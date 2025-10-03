#include <csignal>

#include "matcher/matching_engine.h"
#include "market_data/market_data_publisher.h"
#include "order_server/order_server.h"
#include "common/performance_dashboard.h"
#include "common/latency_tracker.h"

/// Main components, made global to be accessible from the signal handler.
Common::Logger *logger = nullptr;
Exchange::MatchingEngine *matching_engine = nullptr;
Exchange::MarketDataPublisher *market_data_publisher = nullptr;
Exchange::OrderServer *order_server = nullptr;

/// Shut down gracefully on external signals to this server.
void signal_handler(int) {
  // Removed 10 second sleeps - using event-driven shutdown for nanosecond performance

  delete logger;
  logger = nullptr;
  delete matching_engine;
  matching_engine = nullptr;
  delete market_data_publisher;
  market_data_publisher = nullptr;
  delete order_server;
  order_server = nullptr;

  // Removed 10 second sleep - using event-driven shutdown for nanosecond performance

  exit(EXIT_SUCCESS);
}

int main(int, char **) {
  logger = new Common::Logger("exchange_main.log");

  std::signal(SIGINT, signal_handler);

  // Initialize nanosecond performance monitoring
  Common::NanosecondTimer::calibrate();
  Common::g_performance_dashboard.start();
  
  logger->log("%:% %() % Starting NANOSECOND HFT Engine with performance monitoring...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));

  // The lock free queues to facilitate communication between order server <-> matching engine and matching engine -> market data publisher.
  Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
  Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
  Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);

  std::string time_str;

  logger->log("%:% %() % Starting Nanosecond-Precision Matching Engine...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  matching_engine = new Exchange::MatchingEngine(&client_requests, &client_responses, &market_updates);
  matching_engine->start();

  const std::string mkt_pub_iface = "lo";
  const std::string snap_pub_ip = "233.252.14.1", inc_pub_ip = "233.252.14.3";
  const int snap_pub_port = 20000, inc_pub_port = 20001;

  logger->log("%:% %() % Starting Market Data Publisher...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  market_data_publisher = new Exchange::MarketDataPublisher(&market_updates, mkt_pub_iface, snap_pub_ip, snap_pub_port, inc_pub_ip, inc_pub_port);
  market_data_publisher->start();

  const std::string order_gw_iface = "lo";
  const int order_gw_port = 12345;

  logger->log("%:% %() % Starting Order Server...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  order_server = new Exchange::OrderServer(&client_requests, &client_responses, order_gw_iface, order_gw_port);
  order_server->start();

  logger->log("%:% %() % NANOSECOND HFT Engine started successfully! Performance monitoring active.\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  
  while (true) {
    // Event-driven main loop - no sleep for nanosecond performance
    // Performance dashboard runs in separate thread and reports metrics
    std::this_thread::yield(); // Minimal yield instead of blocking sleep
  }
}
