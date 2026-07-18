#include "Order.h"

#include <stdexcept>

std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

Order::Order(int orderId, std::string symbol, Side side, int price, int quantity, long long timestamp)
    : orderId_(orderId),
      symbol_(std::move(symbol)),
      side_(side),
      price_(price),
      quantity_(quantity),
      remainingQuantity_(quantity),
      timestamp_(timestamp) {}

void Order::fill(int filledQuantity) {
    if (filledQuantity < 0 || filledQuantity > remainingQuantity_) {
        throw std::logic_error("Fill quantity exceeds remaining quantity for order " +
                                std::to_string(orderId_));
    }
    remainingQuantity_ -= filledQuantity;
}

void Order::setPrice(int newPrice) {
    price_ = newPrice;
}

void Order::setRemainingQuantity(int quantity) {
    remainingQuantity_ = quantity;
}
