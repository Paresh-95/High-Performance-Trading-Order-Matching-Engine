cmake_minimum_required(VERSION 3.15)
project(HighPerformanceTradingOrderMatchingEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(ENGINE_SOURCES
    src/Order.cpp
    src/Trade.cpp
    src/Utils.cpp
    src/Logger.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
)

add_library(trading_engine STATIC ${ENGINE_SOURCES})
target_include_directories(trading_engine PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Older GCC (< 9) ships std::filesystem in a separate static library.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(trading_engine PUBLIC stdc++fs)
endif()

add_executable(trading_engine_app main.cpp)
target_link_libraries(trading_engine_app PRIVATE trading_engine)

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(order_book_tests tests/OrderBookTests.cpp)
    target_link_libraries(order_book_tests PRIVATE trading_engine)
    add_test(NAME OrderBookTests COMMAND order_book_tests)

    add_executable(matching_engine_tests tests/MatchingEngineTests.cpp)
    target_link_libraries(matching_engine_tests PRIVATE trading_engine)
    add_test(NAME MatchingEngineTests COMMAND matching_engine_tests)
endif()
