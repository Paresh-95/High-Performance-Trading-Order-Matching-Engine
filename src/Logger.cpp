#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

namespace fs = std::filesystem;

namespace {

void ensureDirectoryExists(const std::string& filePath) {
    const fs::path parent = fs::path(filePath).parent_path();
    if (!parent.empty() && !fs::exists(parent)) {
        fs::create_directories(parent);
    }
}

}  // namespace

Logger::Logger(std::string tradeLogPath, std::string errorLogPath)
    : tradeLogPath_(std::move(tradeLogPath)), errorLogPath_(std::move(errorLogPath)) {
    ensureDirectoryExists(tradeLogPath_);
    ensureDirectoryExists(errorLogPath_);
}

void Logger::appendLine(const std::string& path, const std::string& line) const {
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Warning: unable to open log file: " << path << '\n';
        return;
    }
    out << line << '\n';
}

void Logger::logTrade(const Trade& trade) const {
    std::ostringstream oss;
    oss << "--------------------------------\n"
        << "Trade ID   : " << trade.getTradeId() << '\n'
        << "Buyer      : " << trade.getBuyOrderId() << '\n'
        << "Seller     : " << trade.getSellOrderId() << '\n'
        << "Symbol     : " << trade.getSymbol() << '\n'
        << "Price      : " << trade.getPrice() << '\n'
        << "Quantity   : " << trade.getQuantity() << '\n'
        << "Timestamp  : " << Utils::formatTimestamp(trade.getTimestamp()) << '\n'
        << "--------------------------------";
    appendLine(tradeLogPath_, oss.str());
}

void Logger::logError(const std::string& message) const {
    appendLine(errorLogPath_, "[ERROR] " + Utils::formatTimestamp(Utils::currentTimestampMillis()) + " - " + message);
}

void Logger::logInfo(const std::string& message) const {
    appendLine(errorLogPath_, "[INFO] " + Utils::formatTimestamp(Utils::currentTimestampMillis()) + " - " + message);
}

void Logger::saveTradeSnapshot(const std::vector<Trade>& trades, const std::string& path) const {
    ensureDirectoryExists(path);
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Warning: unable to open log file: " << path << '\n';
        return;
    }

    out << "Trade History Snapshot (" << trades.size() << " trades)\n";
    out << "================================\n";
    for (const auto& trade : trades) {
        out << "--------------------------------\n"
            << "Trade ID   : " << trade.getTradeId() << '\n'
            << "Buyer      : " << trade.getBuyOrderId() << '\n'
            << "Seller     : " << trade.getSellOrderId() << '\n'
            << "Symbol     : " << trade.getSymbol() << '\n'
            << "Price      : " << trade.getPrice() << '\n'
            << "Quantity   : " << trade.getQuantity() << '\n'
            << "Timestamp  : " << Utils::formatTimestamp(trade.getTimestamp()) << '\n';
    }
    out << "--------------------------------\n";
}
