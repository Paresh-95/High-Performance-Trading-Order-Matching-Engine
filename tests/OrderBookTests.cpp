// Lightweight, dependency-free unit tests for OrderBook. Each test
// function exercises one behaviour and reports through check(); main()
// returns the number of failures so ctest can treat non-zero as a
// failing test run.

#include <iostream>
#include <string>
#include <vector>

#include "Logger.h"
#include "OrderBook.h"
#include "Trade.h"
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

Logger& testLogger() {
    static Logger logger("logs/test_trades.log", "logs/test_error.log");
    return logger;
}

void testEmptyBookHasNoBidOrAsk() {
    OrderBook book("AAPL");
    check(!book.bestBidPrice().has_value(), "Empty book has no best bid");
    check(!book.bestAskPrice().has_value(), "Empty book has no best ask");
}

void testPartialExecutionExample() {
    // Matches the example from the spec: BUY 100 @ 150, SELL 40 @ 149
    // should trade 40 shares and leave 60 shares resting on the buy side.
    OrderBook book("AAPL");
    std::vector<Trade> trades;
    int nextTradeId = 1;

    Order buyOrder(1, "AAPL", Side::BUY, 150, 100, Utils::currentTimestampMillis());
    book.addOrder(buyOrder, trades, nextTradeId, testLogger());

    Order sellOrder(2, "AAPL", Side::SELL, 149, 40, Utils::currentTimestampMillis());
    book.addOrder(sellOrder, trades, nextTradeId, testLogger());

    check(trades.size() == 1, "One trade generated for crossing orders");
    check(trades[0].getQuantity() == 40, "Trade quantity is 40 shares");
    check(book.bestBidPrice().has_value() && book.bestBidPrice().value() == 150, "Best bid remains at 150");
    check(book.hasOrder(1), "Buy order remains open with 60 shares remaining");
    check(!book.hasOrder(2), "Sell order is fully filled and removed");
}

void testPriceTimePriority() {
    OrderBook book("AAPL");
    std::vector<Trade> trades;
    int nextTradeId = 1;

    Order firstBuy(1, "AAPL", Side::BUY, 150, 50, 1000);
    Order secondBuy(2, "AAPL", Side::BUY, 150, 50, 2000);
    book.addOrder(firstBuy, trades, nextTradeId, testLogger());
    book.addOrder(secondBuy, trades, nextTradeId, testLogger());

    Order sellOrder(3, "AAPL", Side::SELL, 150, 50, 3000);
    book.addOrder(sellOrder, trades, nextTradeId, testLogger());

    check(trades.size() == 1, "Only the older order at the same price is matched");
    check(trades[0].getBuyOrderId() == 1, "First submitted buy order executes before the newer one");
    check(book.hasOrder(2), "Second buy order is still resting in the book");
}

void testCancelOrder() {
    OrderBook book("AAPL");
    std::vector<Trade> trades;
    int nextTradeId = 1;

    Order order(1, "AAPL", Side::BUY, 150, 100, Utils::currentTimestampMillis());
    book.addOrder(order, trades, nextTradeId, testLogger());
    check(book.hasOrder(1), "Order exists before cancellation");

    book.cancelOrder(1);
    check(!book.hasOrder(1), "Order removed after cancellation");

    bool threw = false;
    try {
        book.cancelOrder(1);
    } catch (const OrderNotFoundException&) {
        threw = true;
    }
    check(threw, "Cancelling a missing order throws OrderNotFoundException");
}

void testModifyOrder() {
    OrderBook book("AAPL");
    std::vector<Trade> trades;
    int nextTradeId = 1;

    Order sellOrder(1, "AAPL", Side::SELL, 155, 100, Utils::currentTimestampMillis());
    book.addOrder(sellOrder, trades, nextTradeId, testLogger());

    book.modifyOrder(1, 150, 60, trades, nextTradeId, testLogger());
    check(book.hasOrder(1), "Modified order is still open");
    check(book.bestAskPrice().has_value() && book.bestAskPrice().value() == 150,
          "Modified order updates the best ask price");

    Order buyOrder(2, "AAPL", Side::BUY, 150, 60, Utils::currentTimestampMillis());
    book.addOrder(buyOrder, trades, nextTradeId, testLogger());
    check(trades.size() == 1, "Modified order can match against a new incoming order");
    check(!book.hasOrder(1), "Modified order is fully filled after matching");
}

void testDuplicateOrderRejected() {
    OrderBook book("AAPL");
    std::vector<Trade> trades;
    int nextTradeId = 1;

    Order order(1, "AAPL", Side::BUY, 150, 10, Utils::currentTimestampMillis());
    book.addOrder(order, trades, nextTradeId, testLogger());

    bool threw = false;
    try {
        Order duplicate(1, "AAPL", Side::SELL, 150, 10, Utils::currentTimestampMillis());
        book.addOrder(duplicate, trades, nextTradeId, testLogger());
    } catch (const DuplicateOrderException&) {
        threw = true;
    }
    check(threw, "Duplicate order ID is rejected");
}

}  // namespace

int main() {
    testEmptyBookHasNoBidOrAsk();
    testPartialExecutionExample();
    testPriceTimePriority();
    testCancelOrder();
    testModifyOrder();
    testDuplicateOrderRejected();

    std::cout << '\n' << failures << " failure(s).\n";
    return failures == 0 ? 0 : 1;
}
