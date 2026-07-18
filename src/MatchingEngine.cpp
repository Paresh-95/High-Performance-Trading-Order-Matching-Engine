#include "MatchingEngine.h"

#include <iomanip>

#include "Utils.h"

MatchingEngine::MatchingEngine()
    : logger_(),
      nextOrderId_(1),
      nextTradeId_(1),
      ordersProcessed_(0),
      ordersCancelled_(0),
      ordersModified_(0) {
    for (const auto& symbol : Utils::supportedSymbols()) {
        orderBooks_.emplace(symbol, OrderBook(symbol));
    }
}

void MatchingEngine::validateNewOrder(const std::string& symbol, int price, int quantity) {
    if (!Utils::isSupportedSymbol(symbol)) {
        throw UnknownSymbolException("Unknown trading symbol: " + symbol);
    }
    if (price <= 0) {
        throw InvalidPriceException("Price must be a positive value.");
    }
    if (quantity <= 0) {
        throw InvalidQuantityException("Quantity must be a positive value.");
    }
}

OrderBook& MatchingEngine::resolveOrderBook(const std::string& symbol) {
    return orderBooks_.at(symbol);
}

const OrderBook& MatchingEngine::resolveOrderBook(const std::string& symbol) const {
    return orderBooks_.at(symbol);
}

int MatchingEngine::submitOrder(const std::string& symbol, Side side, int price, int quantity) {
    validateNewOrder(symbol, price, quantity);

    const int orderId = nextOrderId_++;
    Order order(orderId, symbol, side, price, quantity, Utils::currentTimestampMillis());

    OrderBook& book = resolveOrderBook(symbol);
    book.addOrder(std::move(order), tradeHistory_, nextTradeId_, logger_);

    orderIdToSymbol_[orderId] = symbol;
    ++ordersProcessed_;
    return orderId;
}

int MatchingEngine::placeBuyOrder(const std::string& symbol, int price, int quantity) {
    return submitOrder(symbol, Side::BUY, price, quantity);
}

int MatchingEngine::placeSellOrder(const std::string& symbol, int price, int quantity) {
    return submitOrder(symbol, Side::SELL, price, quantity);
}

void MatchingEngine::cancelOrder(int orderId) {
    auto it = orderIdToSymbol_.find(orderId);
    if (it == orderIdToSymbol_.end()) {
        throw OrderNotFoundException("Order not found: " + std::to_string(orderId));
    }

    OrderBook& book = resolveOrderBook(it->second);
    if (!book.hasOrder(orderId)) {
        throw OrderNotFoundException("Order " + std::to_string(orderId) +
                                      " is no longer open (already filled or cancelled).");
    }

    book.cancelOrder(orderId);
    ++ordersCancelled_;
}

void MatchingEngine::modifyOrder(int orderId, int newPrice, int newQuantity) {
    auto it = orderIdToSymbol_.find(orderId);
    if (it == orderIdToSymbol_.end()) {
        throw OrderNotFoundException("Order not found: " + std::to_string(orderId));
    }

    OrderBook& book = resolveOrderBook(it->second);
    if (!book.hasOrder(orderId)) {
        throw OrderNotFoundException("Order " + std::to_string(orderId) +
                                      " is no longer open (already filled or cancelled).");
    }

    book.modifyOrder(orderId, newPrice, newQuantity, tradeHistory_, nextTradeId_, logger_);
    ++ordersModified_;
}

void MatchingEngine::printOrderBook(const std::string& symbol, std::ostream& os) const {
    if (!Utils::isSupportedSymbol(symbol)) {
        throw UnknownSymbolException("Unknown trading symbol: " + symbol);
    }
    resolveOrderBook(symbol).print(os);
}

void MatchingEngine::printAllOrderBooks(std::ostream& os) const {
    for (const auto& symbol : Utils::supportedSymbols()) {
        resolveOrderBook(symbol).print(os);
    }
}

void MatchingEngine::printTradeHistory(std::ostream& os) const {
    os << "\n----------------------------------------------------\n";
    os << "Trade History\n";
    os << "----------------------------------------------------\n";

    if (tradeHistory_.empty()) {
        os << "(no trades executed yet)\n";
        return;
    }

    os << std::left << std::setw(8) << "TradeID" << std::setw(8) << "BuyID" << std::setw(8) << "SellID"
       << std::setw(8) << "Symbol" << std::setw(8) << "Price" << std::setw(8) << "Qty" << "Timestamp\n";
    os << "----------------------------------------------------\n";

    for (const auto& trade : tradeHistory_) {
        os << std::left << std::setw(8) << trade.getTradeId() << std::setw(8) << trade.getBuyOrderId()
           << std::setw(8) << trade.getSellOrderId() << std::setw(8) << trade.getSymbol() << std::setw(8)
           << trade.getPrice() << std::setw(8) << trade.getQuantity()
           << Utils::formatTimestamp(trade.getTimestamp()) << '\n';
    }
}

ExchangeStatistics MatchingEngine::getStatistics() const {
    ExchangeStatistics stats;
    stats.ordersProcessed = ordersProcessed_;
    stats.tradesExecuted = static_cast<int>(tradeHistory_.size());
    stats.ordersCancelled = ordersCancelled_;
    stats.ordersModified = ordersModified_;

    int openOrders = 0;
    for (const auto& entry : orderBooks_) {
        openOrders += entry.second.openOrderCount();
    }
    stats.openOrders = openOrders;

    return stats;
}

void MatchingEngine::printStatistics(std::ostream& os) const {
    const ExchangeStatistics stats = getStatistics();
    os << "\n====================================================\n";
    os << "Exchange Statistics\n";
    os << "====================================================\n";
    os << "Orders Processed : " << stats.ordersProcessed << '\n';
    os << "Trades Executed  : " << stats.tradesExecuted << '\n';
    os << "Cancelled Orders : " << stats.ordersCancelled << '\n';
    os << "Modified Orders  : " << stats.ordersModified << '\n';
    os << "Open Orders      : " << stats.openOrders << '\n';
    os << "====================================================\n";
}

void MatchingEngine::saveTradeLogs() {
    logger_.saveTradeSnapshot(tradeHistory_, "logs/trade_history.log");
    logger_.logInfo("Trade history snapshot saved (" + std::to_string(tradeHistory_.size()) + " trades).");
}
