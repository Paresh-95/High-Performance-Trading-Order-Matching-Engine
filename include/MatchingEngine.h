#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Logger.h"
#include "Order.h"
#include "OrderBook.h"
#include "Trade.h"

// Snapshot of exchange-wide counters, returned by value so callers
// cannot mutate engine state through it.
struct ExchangeStatistics {
    int ordersProcessed = 0;
    int tradesExecuted = 0;
    int ordersCancelled = 0;
    int ordersModified = 0;
    int openOrders = 0;
};

// Top-level facade the CLI (and tests) talk to. Owns one OrderBook per
// supported symbol, assigns order/trade ids, routes cancel/modify
// requests to the right book and tracks exchange-wide statistics.
class MatchingEngine {
public:
    MatchingEngine();

    int placeBuyOrder(const std::string& symbol, int price, int quantity);
    int placeSellOrder(const std::string& symbol, int price, int quantity);

    void cancelOrder(int orderId);
    void modifyOrder(int orderId, int newPrice, int newQuantity);

    void printOrderBook(const std::string& symbol, std::ostream& os) const;
    void printAllOrderBooks(std::ostream& os) const;
    void printTradeHistory(std::ostream& os) const;
    void printStatistics(std::ostream& os) const;

    const std::vector<Trade>& getTradeHistory() const { return tradeHistory_; }
    ExchangeStatistics getStatistics() const;

    // Persists a full snapshot of the trade history to disk on demand.
    void saveTradeLogs();

private:
    std::unordered_map<std::string, OrderBook> orderBooks_;
    std::unordered_map<int, std::string> orderIdToSymbol_;

    std::vector<Trade> tradeHistory_;
    Logger logger_;

    int nextOrderId_;
    int nextTradeId_;

    int ordersProcessed_;
    int ordersCancelled_;
    int ordersModified_;

    int submitOrder(const std::string& symbol, Side side, int price, int quantity);
    static void validateNewOrder(const std::string& symbol, int price, int quantity);
    OrderBook& resolveOrderBook(const std::string& symbol);
    const OrderBook& resolveOrderBook(const std::string& symbol) const;
};
