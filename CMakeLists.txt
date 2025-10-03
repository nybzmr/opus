cmake_minimum_required(VERSION 3.10)

project(NanosecondHFT)

set(CMAKE_CXX_STANDARD 17)

# Production-grade optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")

# Include directories
include_directories("Exchange Matching Engine /Common Files")
include_directories("Exchange Matching Engine /EXCHANGE")
include_directories("Exchange Matching Engine /EXCHANGE/matcher")
include_directories("Exchange Matching Engine /EXCHANGE/order_server")
include_directories("Exchange Matching Engine /EXCHANGE/market_data")
include_directories("trading")
include_directories("trading/strategy")
include_directories("trading/market_data")
include_directories("trading/order_gw")

# Common sources
set(COMMON_SOURCES
    "Exchange Matching Engine /Common Files/latency_tracker.cpp"
    "Exchange Matching Engine /Common Files/performance_dashboard.cpp"
    "Exchange Matching Engine /Common Files/tcp_socket.cpp"
    "Exchange Matching Engine /Common Files/tcp_server.cpp"
    "Exchange Matching Engine /Common Files/mcast_socket.cpp"
)

# Exchange executable
add_executable(exchange_main
    "Exchange Matching Engine /EXCHANGE/exchange_main.cpp"
    "Exchange Matching Engine /EXCHANGE/matcher/matching_engine.cpp"
    "Exchange Matching Engine /EXCHANGE/matcher/me_order_book.cpp"
    "Exchange Matching Engine /EXCHANGE/matcher/me_order.cpp"
    "Exchange Matching Engine /EXCHANGE/market_data/market_data_publisher.cpp"
    "Exchange Matching Engine /EXCHANGE/market_data/snapshot_synthesizer.cpp"
    "Exchange Matching Engine /EXCHANGE/order_server/order_server.cpp"
    ${COMMON_SOURCES}
)

target_link_libraries(exchange_main pthread)

# Trading client
add_executable(trading_main
    "trading/trading_main.cpp"
    "trading/strategy/trade_engine.cpp"
    "trading/strategy/market_order_book.cpp"
    "trading/strategy/market_order.cpp"
    "trading/strategy/market_maker.cpp"
    "trading/strategy/liquidity_taker.cpp"
    "trading/strategy/order_manager.cpp"
    "trading/strategy/risk_manager.cpp"
    "trading/market_data/market_data_consumer.cpp"
    "trading/order_gw/order_gateway.cpp"
    ${COMMON_SOURCES}
)

target_link_libraries(trading_main pthread)