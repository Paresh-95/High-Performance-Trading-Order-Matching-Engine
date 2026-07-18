#include "Trade.h"

Trade::Trade(int tradeId, int buyOrderId, int sellOrderId, std::string symbol,
             int price, int quantity, long long timestamp)
    : tradeId_(tradeId),
      buyOrderId_(buyOrderId),
      sellOrderId_(sellOrderId),
      symbol_(std::move(symbol)),
      price_(price),
      quantity_(quantity),
      timestamp_(timestamp) {}
