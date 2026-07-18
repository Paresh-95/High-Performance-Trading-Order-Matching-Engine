#pragma once

#include <stdexcept>
#include <string>
#include <vector>

// Shared helpers: clock access, timestamp formatting and the set of
// symbols the exchange is willing to trade.
namespace Utils {

long long currentTimestampMillis();
std::string formatTimestamp(long long timestampMillis);

const std::vector<std::string>& supportedSymbols();
bool isSupportedSymbol(const std::string& symbol);

}  // namespace Utils

// Base class for every domain-level error the engine can raise.
// Deriving distinct exception types (rather than throwing plain
// std::runtime_error everywhere) lets callers catch narrowly when they
// need to and broadly (via OrderException) otherwise.
class OrderException : public std::runtime_error {
public:
    explicit OrderException(const std::string& message) : std::runtime_error(message) {}
};

class InvalidPriceException : public OrderException {
public:
    explicit InvalidPriceException(const std::string& message) : OrderException(message) {}
};

class InvalidQuantityException : public OrderException {
public:
    explicit InvalidQuantityException(const std::string& message) : OrderException(message) {}
};

class UnknownSymbolException : public OrderException {
public:
    explicit UnknownSymbolException(const std::string& message) : OrderException(message) {}
};

class DuplicateOrderException : public OrderException {
public:
    explicit DuplicateOrderException(const std::string& message) : OrderException(message) {}
};

class OrderNotFoundException : public OrderException {
public:
    explicit OrderNotFoundException(const std::string& message) : OrderException(message) {}
};
