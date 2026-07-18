// Lightweight, dependency-free unit tests for MatchingEngine. See
// OrderBookTests.cpp for the same small check()-based test harness.

#include <iostream>
#include <string>

#include "MatchingEngine.h"
#include "Utils.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& description) {
    if (condition) {
        std::cout << "[PASS] " << description << '\n';
    } else {
        std::cout << "[FAIL] " << description << '\n';
        ++failures;
    }
}

void testUnknownSymbolRejected() {
    MatchingEngine engine;
    bool threw = false;
    try {
        engine.placeBuyOrder("XXXX", 100, 10);
    } catch (const UnknownSymbolException&) {
        threw = true;
    }
    check(threw, "Unknown symbol is rejected");
}

void testInvalidPriceAndQuantityRejected() {
    MatchingEngine engine;

    bool priceThrew = false;
    try {
        engine.placeBuyOrder("AAPL", -5, 10);
    } catch (const InvalidPriceException&) {
        priceThrew = true;
    }
    check(priceThrew, "Negative price is rejected");

    bool quantityThrew = false;
    try {
        engine.placeBuyOrder("AAPL", 100, -10);
    } catch (const InvalidQuantityException&) {
        quantityThrew = true;
    }
    check(quantityThrew, "Negative quantity is rejected");
}

void testFullTradeLifecycle() {
    MatchingEngine engine;
    engine.placeBuyOrder("AAPL", 150, 100);
    engine.placeSellOrder("AAPL", 149, 40);

    check(engine.getTradeHistory().size() == 1, "One trade recorded after crossing orders");
    check(engine.getTradeHistory()[0].getQuantity() == 40, "Trade quantity matches the partial execution example");

    const ExchangeStatistics stats = engine.getStatistics();
    check(stats.ordersProcessed == 2, "Two orders processed");
    check(stats.tradesExecuted == 1, "One trade executed");
    check(stats.openOrders == 1, "One order remains open (partially filled buy order)");
}

void testCancelAndModifyThroughEngine() {
    MatchingEngine engine;
    const int orderId = engine.placeSellOrder("MSFT", 300, 50);

    engine.modifyOrder(orderId, 295, 30);
    check(engine.getStatistics().ordersModified == 1, "Modify count is tracked correctly");

    engine.cancelOrder(orderId);
    const ExchangeStatistics statsAfterCancel = engine.getStatistics();
    check(statsAfterCancel.ordersCancelled == 1, "Cancel count is tracked correctly");
    check(statsAfterCancel.openOrders == 0, "No open orders remain after cancellation");

    bool threw = false;
    try {
        engine.cancelOrder(orderId);
    } catch (const OrderNotFoundException&) {
        threw = true;
    }
    check(threw, "Cancelling an already cancelled order throws OrderNotFoundException");
}

void testMultiSymbolIsolation() {
    MatchingEngine engine;
    engine.placeBuyOrder("AAPL", 150, 10);
    engine.placeBuyOrder("TSLA", 700, 5);

    // A GOOG sell should not match against AAPL or TSLA resting orders.
    engine.placeSellOrder("GOOG", 100, 5);
    check(engine.getTradeHistory().empty(), "Orders on different symbols never cross");
    check(engine.getStatistics().openOrders == 3, "All three orders remain open across separate books");
}

}  // namespace

int main() {
    testUnknownSymbolRejected();
    testInvalidPriceAndQuantityRejected();
    testFullTradeLifecycle();
    testCancelAndModifyThroughEngine();
    testMultiSymbolIsolation();

    std::cout << '\n' << failures << " failure(s).\n";
    return failures == 0 ? 0 : 1;
}
