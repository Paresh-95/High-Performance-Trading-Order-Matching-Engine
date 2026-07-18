#pragma once

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Logger.h"
#include "Order.h"
#include "Trade.h"

// Maintains the buy and sell books for a single trading symbol and
// applies price-time priority matching.
//
// Buy book:  highest price first, FIFO within a price level.
// Sell book: lowest price first,  FIFO within a price level.
class OrderBook {
public:
    explicit OrderBook(std::string symbol = "");

    // Inserts a new order and immediately attempts to match it against
    // the opposite book. Any resulting trades are appended to
    // tradeHistory and logged. Throws DuplicateOrderException if
    // order's id is already present in this book.
    void addOrder(Order order, std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger);

    // Removes an open order. Throws OrderNotFoundException if the id
    // is unknown to this book.
    void cancelOrder(int orderId);

    // Changes price and/or quantity of an existing order. Implemented
    // as cancel + re-insert, so a modified order loses its place in
    // time priority and is re-matched from scratch -- the same rule
    // most real exchanges apply. Throws OrderNotFoundException,
    // InvalidPriceException or InvalidQuantityException as appropriate.
    void modifyOrder(int orderId, int newPrice, int newQuantity,
                      std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger);

    bool hasOrder(int orderId) const;
    std::optional<int> bestBidPrice() const;
    std::optional<int> bestAskPrice() const;
    int openOrderCount() const;

    void print(std::ostream& os) const;

private:
    struct OrderLocation {
        Side side;
        int price;
    };

    std::string symbol_;
    std::map<int, std::deque<Order>, std::greater<int>> buyBook_;
    std::map<int, std::deque<Order>> sellBook_;
    std::unordered_map<int, OrderLocation> orderLocations_;

    void insertOrder(Order order);
    void removeFromLocation(int orderId);
    void match(std::vector<Trade>& tradeHistory, int& nextTradeId, Logger& logger);
};
