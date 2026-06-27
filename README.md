# High-Performance Trading Order Matching Engine

A console-based stock exchange simulator written in modern C++17. It accepts
limit buy/sell orders across four trading symbols, matches them using
**price-time priority**, supports partial fills, order cancellation and
modification, and logs every trade to disk.

Built as a focused systems-programming project: clean OOP design, STL
containers chosen deliberately for their complexity guarantees, RAII-based
logging, and a small unit test suite — without multithreading or lock-free
tricks, since the goal is to demonstrate solid fundamentals rather than
concurrency engineering.

## Project Overview

The engine models a simplified single-exchange order book:

- Traders submit **limit orders** (buy or sell a quantity at a specified
  price) for one of four symbols: `AAPL`, `GOOG`, `MSFT`, `TSLA`.
- Each symbol has its own independent order book.
- Orders rest in the book until they are matched, cancelled, or modified.
- Whenever an order is added or modified, the engine immediately tries to
  match it against the opposite side of the book, executing trades at the
  resting order's price and supporting partial fills.
- Every trade is appended to an in-memory trade history and to
  `logs/trades.log`; exchange-wide statistics are tracked throughout the
  session.

## Architecture

```
                    ┌─────────────┐
   CLI (main.cpp)   │             │
   ───────────────► │ MatchingEngine │  owns one OrderBook per symbol,
                    │             │  assigns order/trade IDs, tracks stats
                    └──────┬──────┘
                           │ routes by symbol
                 ┌─────────┼─────────┬─────────┬─────────┐
                 ▼         ▼         ▼         ▼         │
             OrderBook OrderBook OrderBook OrderBook     │
              (AAPL)    (GOOG)    (MSFT)    (TSLA)       │
                 │                                       │
                 ▼                                       ▼
         buy/sell price levels                       Logger
        (std::map<int, deque<Order>>)          (logs/trades.log,
                                                  logs/error.log)
```

- **`Order`** / **`Trade`** — small immutable-by-default value types.
- **`OrderBook`** — owns the buy/sell books for one symbol and implements
  matching, cancellation and modification.
- **`MatchingEngine`** — the facade the CLI talks to; owns all order books,
  assigns IDs, routes requests, and aggregates statistics.
- **`Logger`** — writes trade confirmations and error/info messages to disk.
- **`Utils`** — timestamps, the supported symbol list, and the exception
  hierarchy used across the engine.

## Folder Structure

```
High-Performance-Trading-Order-Matching-Engine/
├── include/
│   ├── Order.h
│   ├── Trade.h
│   ├── OrderBook.h
│   ├── MatchingEngine.h
│   ├── Logger.h
│   └── Utils.h
├── src/
│   ├── Order.cpp
│   ├── Trade.cpp
│   ├── Utils.cpp
│   ├── OrderBook.cpp
│   ├── MatchingEngine.cpp
│   └── Logger.cpp
├── tests/
│   ├── OrderBookTests.cpp
│   └── MatchingEngineTests.cpp
├── logs/
├── README.md
├── CMakeLists.txt
└── main.cpp
```

## Features

- Place limit buy and sell orders across four symbols (`AAPL`, `GOOG`,
  `MSFT`, `TSLA`)
- Price-time priority matching with partial execution
- Order cancellation
- Order modification (price and/or quantity)
- Order book view (per symbol or all symbols), aggregated by price level
- Full trade history view
- Live exchange statistics
- On-demand trade log snapshot export, plus automatic append-on-trade
  logging
- Input validation and a typed exception hierarchy for error handling

## Data Structures Used

| Structure | Where | Why |
|---|---|---|
| `std::map<int, std::deque<Order>, std::greater<int>>` | Buy book | Keeps price levels sorted highest-first; `std::deque` preserves FIFO (time priority) within a level |
| `std::map<int, std::deque<Order>>` | Sell book | Keeps price levels sorted lowest-first, same FIFO guarantee per level |
| `std::unordered_map<int, OrderLocation>` | `OrderBook` | O(1) average lookup from order ID to its side/price bucket, needed for cancel/modify |
| `std::unordered_map<std::string, OrderBook>` | `MatchingEngine` | One book per symbol, O(1) average routing |
| `std::unordered_map<int, std::string>` | `MatchingEngine` | Order ID → symbol routing table for cancel/modify |
| `std::vector<Trade>` | `MatchingEngine` | Append-only, chronological trade history |
| `std::ofstream` (via `Logger`) | `Logger` | Append-mode writes for trade/error logs; `std::filesystem` ensures `logs/` exists |

Prices are modeled as `int` (whole currency units) rather than `double`,
which sidesteps floating-point equality issues when using price as a map
key — a deliberate simplification appropriate for a simulator.

## Matching Algorithm — Price-Time Priority

1. A new or modified order is inserted into its book at its price level
   (back of the FIFO queue for that price).
2. The engine repeatedly compares the best bid (`buyBook_.begin()`) against
   the best ask (`sellBook_.begin()`):
   - **Buy book** is sorted descending, so `begin()` is the *highest* bid.
   - **Sell book** is sorted ascending, so `begin()` is the *lowest* ask.
3. While `bestBid >= bestAsk`, the two front orders (oldest at that price
   level) are matched:
   - Traded quantity = `min(remaining buy qty, remaining sell qty)`.
   - Trade price = the price of whichever order has been resting longer
     (the passive/maker side) — the incoming order simply takes liquidity
     at the book's price.
   - Both orders' remaining quantities are reduced; a fully filled order is
     popped from its queue and removed from the lookup index.
   - Partially filled orders remain at the front of their queue, keeping
     their time priority for the next match.
4. Matching stops as soon as the best bid no longer crosses the best ask.

**Modification** is implemented as cancel + re-insert with a fresh
timestamp: changing price or quantity forfeits the order's existing time
priority and re-triggers matching, mirroring how most real exchanges treat
amendments.

### Worked Example

```
BUY  100 @ 150   (Order #1, resting)
SELL  40 @ 149   (Order #2, incoming, crosses the book)

-> Trade: 40 shares @ 150 (maker's price)
-> Order #1 remaining: 60 shares (still resting, keeps time priority)
-> Order #2: fully filled, removed from the book
```

## Time Complexity

| Operation | Complexity | Notes |
|---|---|---|
| Insert order | O(log P) | `P` = number of distinct price levels; `map::operator[]` plus `deque::push_back` (amortized O(1)) |
| Cancel order | O(log P + k) | O(1) average ID lookup via `unordered_map`, then O(k) scan to erase from its price level's deque (`k` = orders resting at that price) |
| Modify order | O(log P + k) | Cancel + re-insert, so the cost is the sum of both |
| Best bid / best ask | O(1) | `map::begin()` on an already-sorted container |
| Trade matching (single cross) | O(log P) amortized | Each fully-filled order is removed once; a run of `m` matches costs O(m log P) total, not O(m) per match |

## Error Handling

A small exception hierarchy (in `Utils.h`) rooted at `OrderException`
covers every domain error the engine raises:

- `InvalidPriceException` — non-positive price
- `InvalidQuantityException` — non-positive quantity
- `UnknownSymbolException` — symbol outside the supported list
- `DuplicateOrderException` — an order ID that already exists in a book
- `OrderNotFoundException` — cancel/modify targeting an unknown or
  already-closed order

The CLI catches `std::exception` around every menu action, so any of these
(plus unexpected standard library exceptions) are reported to the user
without crashing the session.

## Unit Tests

`tests/OrderBookTests.cpp` and `tests/MatchingEngineTests.cpp` use a tiny
self-contained `check()` harness (no external test framework required) and
cover:

- Matching logic, including the partial-execution worked example above
- Price-time priority ordering between two orders at the same price
- Order cancellation, including the not-found error path
- Order modification and subsequent re-matching
- Duplicate order rejection
- Input validation (price, quantity, unknown symbol)
- End-to-end statistics tracking through `MatchingEngine`
- Isolation between order books of different symbols

## Future Improvements

- Persist and reload order books across sessions (currently in-memory only)
- Support additional order types (market, stop, IOC/FOK)
- Replace `int` prices with a fixed-point decimal type for finer tick sizes
- Expose the engine over a socket/REST API instead of a CLI
- Add a proper testing framework (GoogleTest/Catch2) and CI pipeline

## Sample Output

```
====================================================
   High Performance Trading Order Matching Engine
====================================================
1. Place Buy Order
2. Place Sell Order
3. Cancel Order
4. Modify Order
5. View Order Book
6. View Trade History
7. View Statistics
8. Save Logs
9. Exit
----------------------------------------------------
Choice: 1
Symbol (AAPL, GOOG, MSFT, TSLA): AAPL
Price: 150
Quantity: 100

[OK] Order accepted. Order ID: 1

Choice: 2
Symbol (AAPL, GOOG, MSFT, TSLA): AAPL
Price: 149
Quantity: 40

[OK] Order accepted. Order ID: 2

Choice: 5
View a single symbol or all? (1 = single, 2 = all): 1
Symbol (AAPL, GOOG, MSFT, TSLA): AAPL

----------------------------------------------------
Order Book: AAPL
----------------------------------------------------
BUY Qty     Price      | Price     SELL Qty
----------------------------------------------------
60          150        |
----------------------------------------------------

Choice: 6

----------------------------------------------------
Trade History
----------------------------------------------------
TradeID BuyID   SellID  Symbol  Price   Qty     Timestamp
----------------------------------------------------
1       1       2       AAPL    150     40      2026-07-18 21:50:29.278

Choice: 7

====================================================
Exchange Statistics
====================================================
Orders Processed : 2
Trades Executed  : 1
Cancelled Orders : 0
Modified Orders  : 0
Open Orders      : 1
====================================================
```

`logs/trades.log` after the run above:

```
--------------------------------
Trade ID   : 1
Buyer      : 1
Seller     : 2
Symbol     : AAPL
Price      : 150
Quantity   : 40
Timestamp  : 2026-07-18 21:50:29.278
--------------------------------
```

## Build Instructions

Requires CMake 3.15+ and a C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+).

```bash
# Configure and build (library + CLI app + tests)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run the CLI
./build/trading_engine_app

# Run the unit tests
cd build && ctest --output-on-failure
```

To build without tests, configure with `-DBUILD_TESTS=OFF`.
