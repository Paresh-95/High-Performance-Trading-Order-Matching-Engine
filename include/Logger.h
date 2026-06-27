#pragma once

#include <string>
#include <vector>

#include "Trade.h"

// Writes trade confirmations and diagnostic messages to disk. Each
// call opens, appends to, and closes the target file, which keeps the
// class simple and crash-safe at the cost of some throughput -- an
// acceptable trade-off for a console simulator rather than a
// production feed handler.
class Logger {
public:
    explicit Logger(std::string tradeLogPath = "logs/trades.log",
                     std::string errorLogPath = "logs/error.log");

    void logTrade(const Trade& trade) const;
    void logError(const std::string& message) const;
    void logInfo(const std::string& message) const;

    // Overwrites path with a formatted snapshot of the full trade
    // history. Used by the "Save Logs" menu action, distinct from the
    // automatic per-trade append performed by logTrade().
    void saveTradeSnapshot(const std::vector<Trade>& trades, const std::string& path) const;

private:
    std::string tradeLogPath_;
    std::string errorLogPath_;

    void appendLine(const std::string& path, const std::string& line) const;
};
