cmake_minimum_required(VERSION 3.10)  # Set a valid minimum required version

project(HFT_Developer_Codes)

set(CMAKE_CXX_STANDARD 17)  # Updated to C++17 for better performance features

# Enable maximum optimization for nanosecond performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g -march=native")

include_directories(.)

add_executable(HFT_Developer_Codes
        "macros.h"
        "main.cpp"
        "thread_utils.h"
        "thread.cpp"
        "memory_pool.h"
        "logging.h"
        "logger.cpp"
        "Exchange Matching Engine /Common Files/nanosecond_timer.h"
        "Exchange Matching Engine /Common Files/latency_tracker.h"
        "Exchange Matching Engine /Common Files/latency_tracker.cpp"
        "Exchange Matching Engine /Common Files/performance_dashboard.h"
        "Exchange Matching Engine /Common Files/performance_dashboard.cpp"
        "Exchange Matching Engine /EXCHANGE/order_server/client_response.h"
        "Exchange Matching Engine /EXCHANGE/order_server/client_request.h"
        "Exchange Matching Engine /EXCHANGE/market_data/market_update.h"
        "Exchange Matching Engine /EXCHANGE/matcher/me_order.h"
        "Exchange Matching Engine /EXCHANGE/matcher/me_order.cpp"
        "Exchange Matching Engine /EXCHANGE/matcher/matching_engine.h"
        "Exchange Matching Engine /EXCHANGE/matcher/matching_engine.cpp"
        "Exchange Matching Engine /EXCHANGE/matcher/me_order_book.h"
        "Exchange Matching Engine /EXCHANGE/matcher/me_order_book.cpp")
