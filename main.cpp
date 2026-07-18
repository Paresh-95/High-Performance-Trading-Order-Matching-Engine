#include <cctype>
#include <iostream>
#include <limits>
#include <string>

#include "MatchingEngine.h"
#include "Utils.h"

namespace {

void printMenu() {
    std::cout << "\n====================================================\n";
    std::cout << "   High Performance Trading Order Matching Engine\n";
    std::cout << "====================================================\n";
    std::cout << "1. Place Buy Order\n";
    std::cout << "2. Place Sell Order\n";
    std::cout << "3. Cancel Order\n";
    std::cout << "4. Modify Order\n";
    std::cout << "5. View Order Book\n";
    std::cout << "6. View Trade History\n";
    std::cout << "7. View Statistics\n";
    std::cout << "8. Save Logs\n";
    std::cout << "9. Exit\n";
    std::cout << "----------------------------------------------------\n";
    std::cout << "Choice: ";
}

int readInt() {
    int value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a whole number: ";
    }
    return value;
}

std::string readSymbol() {
    std::string symbol;
    std::cout << "Symbol (AAPL, GOOG, MSFT, TSLA): ";
    std::cin >> symbol;
    for (char& c : symbol) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return symbol;
}

void handlePlaceOrder(MatchingEngine& engine, Side side) {
    try {
        const std::string symbol = readSymbol();
        std::cout << "Price: ";
        const int price = readInt();
        std::cout << "Quantity: ";
        const int quantity = readInt();

        const int orderId = (side == Side::BUY) ? engine.placeBuyOrder(symbol, price, quantity)
                                                  : engine.placeSellOrder(symbol, price, quantity);

        std::cout << "\n[OK] Order accepted. Order ID: " << orderId << '\n';
    } catch (const std::exception& ex) {
        std::cout << "\n[ERROR] " << ex.what() << '\n';
    }
}

void handleCancelOrder(MatchingEngine& engine) {
    try {
        std::cout << "Order ID to cancel: ";
        const int orderId = readInt();
        engine.cancelOrder(orderId);
        std::cout << "\n[OK] Order " << orderId << " cancelled.\n";
    } catch (const std::exception& ex) {
        std::cout << "\n[ERROR] " << ex.what() << '\n';
    }
}

void handleModifyOrder(MatchingEngine& engine) {
    try {
        std::cout << "Order ID to modify: ";
        const int orderId = readInt();
        std::cout << "New Price: ";
        const int price = readInt();
        std::cout << "New Quantity: ";
        const int quantity = readInt();
        engine.modifyOrder(orderId, price, quantity);
        std::cout << "\n[OK] Order " << orderId << " modified.\n";
    } catch (const std::exception& ex) {
        std::cout << "\n[ERROR] " << ex.what() << '\n';
    }
}

void handleViewOrderBook(MatchingEngine& engine) {
    std::cout << "View a single symbol or all? (1 = single, 2 = all): ";
    const int choice = readInt();
    try {
        if (choice == 1) {
            const std::string symbol = readSymbol();
            engine.printOrderBook(symbol, std::cout);
        } else {
            engine.printAllOrderBooks(std::cout);
        }
    } catch (const std::exception& ex) {
        std::cout << "\n[ERROR] " << ex.what() << '\n';
    }
}

}  // namespace

int main() {
    MatchingEngine engine;

    bool running = true;
    while (running) {
        printMenu();
        const int choice = readInt();

        switch (choice) {
            case 1:
                handlePlaceOrder(engine, Side::BUY);
                break;
            case 2:
                handlePlaceOrder(engine, Side::SELL);
                break;
            case 3:
                handleCancelOrder(engine);
                break;
            case 4:
                handleModifyOrder(engine);
                break;
            case 5:
                handleViewOrderBook(engine);
                break;
            case 6:
                engine.printTradeHistory(std::cout);
                break;
            case 7:
                engine.printStatistics(std::cout);
                break;
            case 8:
                engine.saveTradeLogs();
                std::cout << "\n[OK] Trade history snapshot saved to logs/trade_history.log\n";
                break;
            case 9:
                std::cout << "\nShutting down exchange. Goodbye!\n";
                running = false;
                break;
            default:
                std::cout << "\nInvalid choice. Please select an option between 1 and 9.\n";
        }
    }

    return 0;
}
