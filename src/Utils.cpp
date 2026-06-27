#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Utils {

long long currentTimestampMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string formatTimestamp(long long timestampMillis) {
    const std::time_t seconds = static_cast<std::time_t>(timestampMillis / 1000);
    const int milliseconds = static_cast<int>(timestampMillis % 1000);

    std::tm tmBuffer{};
#if defined(_WIN32)
    localtime_s(&tmBuffer, &seconds);
#else
    localtime_r(&seconds, &tmBuffer);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << milliseconds;
    return oss.str();
}

const std::vector<std::string>& supportedSymbols() {
    static const std::vector<std::string> symbols = {"AAPL", "GOOG", "MSFT", "TSLA"};
    return symbols;
}

bool isSupportedSymbol(const std::string& symbol) {
    const auto& symbols = supportedSymbols();
    return std::find(symbols.begin(), symbols.end(), symbol) != symbols.end();
}

}  // namespace Utils
