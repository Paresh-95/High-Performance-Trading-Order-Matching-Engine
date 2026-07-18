#pragma once

#include <string>

// Side of the market an order belongs to.
enum class Side {
    BUY,
    SELL
};

std::string sideToString(Side side);

// Represents a single limit order submitted to the exchange.
// An Order is mutable only with respect to its price and remaining
// quantity; identity fields (id, symbol, side) never change after
// construction.
class Order {
public:
    Order(int orderId, std::string symbol, Side side, int price, int quantity, long long timestamp);

    int getOrderId() const { return orderId_; }
    const std::string& getSymbol() const { return symbol_; }
    Side getSide() const { return side_; }
    int getPrice() const { return price_; }
    int getQuantity() const { return quantity_; }
    int getRemainingQuantity() const { return remainingQuantity_; }
    long long getTimestamp() const { return timestamp_; }
    bool isFullyFilled() const { return remainingQuantity_ == 0; }

    // Reduces the remaining quantity by filledQuantity. Throws
    // std::logic_error if filledQuantity is invalid.
    void fill(int filledQuantity);

    void setPrice(int newPrice);
    void setRemainingQuantity(int quantity);

private:
    int orderId_;
    std::string symbol_;
    Side side_;
    int price_;
    int quantity_;
    int remainingQuantity_;
    long long timestamp_;
};
