#include "OrderBook.h"

#include <algorithm>
#include <iomanip>
#include <utility>

#include "Utils.h"

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

void OrderBook::insertOrder(Order order) {
    const int orderId = order.getOrderId();
    const Side side = order.getSide();
    const int price = order.getPrice();

    orderLocations_[orderId] = OrderLocation{side, price};
    if (side == Side::BUY) {
        buyBook_[price].push_back(std::move(order));
    } else {
        sellBook_[price].push_back(std::move(order));
    }
}

void OrderBook::removeFromLocation(int orderId) {
    auto locationIt = orderLocations_.find(orderId);
    if (locationIt == orderLocations_.end()) {
        return;
    }

    const OrderLocation location = locationIt->second;
    auto eraseFrom = [orderId](std::deque<Order>& bucket) {
        bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                                     [orderId](const Order& o) { return o.getOrderId() == orderId; }),
                     bucket.end());
    };

    if (location.side == Side::BUY) {
        auto bucketIt = buyBook_.find(location.price);
        if (bucketIt != buyBook_.end()) {
            eraseFrom(bucketIt->second);
            if (bucketIt->second.empty()) {
                buyBook_.erase(bucketIt);
            }
        }
    } else {
        auto bucketIt = sellBook_.find(location.price);
        if (bucketIt != sellBook_.end()) {
            eraseFrom(bucketIt->second);
            if (bucketIt->second.empty()) {
                sellBook_.erase(bucketIt);
            }
        }
    }

    orderLocations_.erase(locationIt);
}

void OrderBook::addOrder(Order order, std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger) {
    if (orderLocations_.find(order.getOrderId()) != orderLocations_.end()) {
        throw DuplicateOrderException("Duplicate order ID: " + std::to_string(order.getOrderId()));
    }

    insertOrder(std::move(order));
    match(tradeHistory, nextTradeId, logger);
}

void OrderBook::cancelOrder(int orderId) {
    if (orderLocations_.find(orderId) == orderLocations_.end()) {
        throw OrderNotFoundException("Order not found: " + std::to_string(orderId));
    }
    removeFromLocation(orderId);
}

void OrderBook::modifyOrder(int orderId, int newPrice, int newQuantity,
                             std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger) {
    auto locationIt = orderLocations_.find(orderId);
    if (locationIt == orderLocations_.end()) {
        throw OrderNotFoundException("Cannot modify. Order not found: " + std::to_string(orderId));
    }
    if (newPrice <= 0) {
        throw InvalidPriceException("Price must be a positive value.");
    }
    if (newQuantity <= 0) {
        throw InvalidQuantityException("Quantity must be a positive value.");
    }

    const Side side = locationIt->second.side;
    removeFromLocation(orderId);

    // Any modification is treated as cancel + re-submit at the back of
    // the new price level's queue, matching how most exchanges handle
    // amendments that change price or increase quantity.
    Order updatedOrder(orderId, symbol_, side, newPrice, newQuantity, Utils::currentTimestampMillis());
    insertOrder(std::move(updatedOrder));
    match(tradeHistory, nextTradeId, logger);
}

void OrderBook::match(std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger) {
    while (!buyBook_.empty() && !sellBook_.empty()) {
        auto buyIt = buyBook_.begin();
        auto sellIt = sellBook_.begin();

        if (buyIt->first < sellIt->first) {
            break;  // Best bid no longer crosses best ask.
        }

        std::deque<Order>& buyBucket = buyIt->second;
        std::deque<Order>& sellBucket = sellIt->second;
        Order& buyOrder = buyBucket.front();
        Order& sellOrder = sellBucket.front();

        const int tradedQuantity = std::min(buyOrder.getRemainingQuantity(), sellOrder.getRemainingQuantity());

        // The order that has been resting in the book longer sets the
        // trade price -- the incoming order simply "takes" liquidity
        // at the passive side's price.
        const int tradePrice =
            (buyOrder.getTimestamp() <= sellOrder.getTimestamp()) ? buyOrder.getPrice() : sellOrder.getPrice();

        Trade trade(nextTradeId++, buyOrder.getOrderId(), sellOrder.getOrderId(), symbol_, tradePrice,
                    tradedQuantity, Utils::currentTimestampMillis());
        tradeHistory.push_back(trade);
        logger.logTrade(trade);

        buyOrder.fill(tradedQuantity);
        sellOrder.fill(tradedQuantity);

        const int buyOrderId = buyOrder.getOrderId();
        const int sellOrderId = sellOrder.getOrderId();
        const bool buyFilled = buyOrder.isFullyFilled();
        const bool sellFilled = sellOrder.isFullyFilled();

        if (buyFilled) {
            buyBucket.pop_front();
            orderLocations_.erase(buyOrderId);
            if (buyBucket.empty()) {
                buyBook_.erase(buyIt);
            }
        }
        if (sellFilled) {
            sellBucket.pop_front();
            orderLocations_.erase(sellOrderId);
            if (sellBucket.empty()) {
                sellBook_.erase(sellIt);
            }
        }
    }
}

bool OrderBook::hasOrder(int orderId) const {
    return orderLocations_.find(orderId) != orderLocations_.end();
}

std::optional<int> OrderBook::bestBidPrice() const {
    if (buyBook_.empty()) {
        return std::nullopt;
    }
    return buyBook_.begin()->first;
}

std::optional<int> OrderBook::bestAskPrice() const {
    if (sellBook_.empty()) {
        return std::nullopt;
    }
    return sellBook_.begin()->first;
}

int OrderBook::openOrderCount() const {
    return static_cast<int>(orderLocations_.size());
}

void OrderBook::print(std::ostream& os) const {
    os << "\n----------------------------------------------------\n";
    os << "Order Book: " << symbol_ << '\n';
    os << "----------------------------------------------------\n";

    if (buyBook_.empty() && sellBook_.empty()) {
        os << "(order book is empty)\n";
        return;
    }

    os << std::left << std::setw(12) << "BUY Qty" << std::setw(10) << "Price" << " | " << std::setw(10)
       << "Price" << std::setw(12) << "SELL Qty" << '\n';
    os << "----------------------------------------------------\n";

    auto buyIt = buyBook_.begin();
    auto sellIt = sellBook_.begin();

    while (buyIt != buyBook_.end() || sellIt != sellBook_.end()) {
        os << std::left;
        if (buyIt != buyBook_.end()) {
            int totalQty = 0;
            for (const auto& order : buyIt->second) {
                totalQty += order.getRemainingQuantity();
            }
            os << std::setw(12) << totalQty << std::setw(10) << buyIt->first;
            ++buyIt;
        } else {
            os << std::setw(12) << "" << std::setw(10) << "";
        }

        os << " | ";

        if (sellIt != sellBook_.end()) {
            int totalQty = 0;
            for (const auto& order : sellIt->second) {
                totalQty += order.getRemainingQuantity();
            }
            os << std::setw(10) << sellIt->first << std::setw(12) << totalQty;
            ++sellIt;
        } else {
            os << std::setw(10) << "" << std::setw(12) << "";
        }
        os << '\n';
    }
    os << "----------------------------------------------------\n";
}
