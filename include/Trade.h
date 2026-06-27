#pragma once

#include <string>

// Represents a completed execution between a resting order and an
// incoming order. Trades are immutable once created.
class Trade {
public:
    Trade(int tradeId, int buyOrderId, int sellOrderId, std::string symbol,
          int price, int quantity, long long timestamp);

    int getTradeId() const { return tradeId_; }
    int getBuyOrderId() const { return buyOrderId_; }
    int getSellOrderId() const { return sellOrderId_; }
    const std::string& getSymbol() const { return symbol_; }
    int getPrice() const { return price_; }
    int getQuantity() const { return quantity_; }
    long long getTimestamp() const { return timestamp_; }

private:
    int tradeId_;
    int buyOrderId_;
    int sellOrderId_;
    std::string symbol_;
    int price_;
    int quantity_;
    long long timestamp_;
};
