#include <csignal>
#include <vector>

#include "types.h"
#include "time_utils.h"
#include "trade_engine.h"
#include "order_gateway.h"
#include "market_data_consumer.h"
#include "logging.h"

/// Main components.
Common::Logger *logger = nullptr;
Trading::TradeEngine *trade_engine = nullptr;
Trading::MarketDataConsumer *market_data_consumer = nullptr;
Trading::OrderGateway *order_gateway = nullptr;

/// ./trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
int main(int argc, char **argv) {
  if(argc < 3) {
    FATAL("USAGE trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...");
  }

  const Common::ClientId client_id = atoi(argv[1]);
  srand(client_id);

  const auto algo_type = stringToAlgoType(argv[2]);

  logger = new Common::Logger("trading_main_" + std::to_string(client_id) + ".log");

  // Removed sleep_time - using event-driven architecture for nanosecond performance

  // The lock free queues to facilitate communication between order gateway <-> trade engine and market data consumer -> trade engine.
  Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
  Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
  Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);

  std::string time_str;

  TradeEngineCfgHashMap ticker_cfg;

  // Parse and initialize the TradeEngineCfgHashMap above from the command line arguments.
  // [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
  size_t next_ticker_id = 0;
  for (int i = 3; i < argc; i += 5, ++next_ticker_id) {
    ticker_cfg.at(next_ticker_id) = {static_cast<Qty>(std::atoi(argv[i])), std::atof(argv[i + 1]),
                                     {static_cast<Qty>(std::atoi(argv[i + 2])),
                                      static_cast<Qty>(std::atoi(argv[i + 3])),
                                      std::atof(argv[i + 4])}};
  }

  logger->log("%:% %() % Starting Trade Engine...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  trade_engine = new Trading::TradeEngine(client_id, algo_type,
                                          ticker_cfg,
                                          &client_requests,
                                          &client_responses,
                                          &market_updates);
  trade_engine->start();

  const std::string order_gw_ip = "127.0.0.1";
  const std::string order_gw_iface = "lo";
  const int order_gw_port = 12345;

  logger->log("%:% %() % Starting Order Gateway...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  order_gateway = new Trading::OrderGateway(client_id, &client_requests, &client_responses, order_gw_ip, order_gw_iface, order_gw_port);
  order_gateway->start();

  const std::string mkt_data_iface = "lo";
  const std::string snapshot_ip = "233.252.14.1";
  const int snapshot_port = 20000;
  const std::string incremental_ip = "233.252.14.3";
  const int incremental_port = 20001;

  logger->log("%:% %() % Starting Market Data Consumer...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  market_data_consumer = new Trading::MarketDataConsumer(client_id, &market_updates, mkt_data_iface, snapshot_ip, snapshot_port, incremental_ip, incremental_port);
  market_data_consumer->start();

  // Removed 10 second sleep - using event-driven initialization

  trade_engine->initLastEventTime();

  if (algo_type == AlgoType::RANDOM) {
    Common::OrderId order_id = client_id * 1000;
    std::vector<Exchange::MEClientRequest> client_requests_vec;
    std::array<Price, ME_MAX_TICKERS> ticker_base_price;
    for (size_t i = 0; i < ME_MAX_TICKERS; ++i)
      ticker_base_price[i] = (rand() % 100) + 100;
    const size_t ORDERS_PER_CLIENT = 100000000;
    
    auto start_time = Common::getCurrentNanos();
    
    for (size_t i = 0; i < ORDERS_PER_CLIENT; ++i) {
      const Common::TickerId ticker_id = rand() % Common::ME_MAX_TICKERS;
      const Price price = ticker_base_price[ticker_id] + (rand() % 10) + 1;
      const Qty qty = 1 + (rand() % 100) + 1;
      const Side side = (rand() % 2 ? Common::Side::BUY : Common::Side::SELL);

      auto order_start = Common::getCurrentNanos();
      
      Exchange::MEClientRequest new_request{Exchange::ClientRequestType::NEW, client_id, ticker_id, order_id++, side,
                                            price, qty};
      trade_engine->sendClientRequest(&new_request);
      
      auto order_latency = Common::getCurrentNanos() - order_start;

      client_requests_vec.push_back(new_request);
      const auto cxl_index = rand() % client_requests_vec.size();
      auto cxl_request = client_requests_vec[cxl_index];
      cxl_request.type_ = Exchange::ClientRequestType::CANCEL;
      trade_engine->sendClientRequest(&cxl_request);
      
      if (i > 0 && i % 10000000 == 0) {
        logger->log("%:% %() % Client % processed % orders...\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str), client_id, i);
      }
    }
    
    auto end_time = Common::getCurrentNanos();
    auto total_time_ns = end_time - start_time;
    auto avg_latency_ns = total_time_ns / (ORDERS_PER_CLIENT * 2);
    
    logger->log("%:% %() % Client % COMPLETED: % orders in % ns (avg latency: % ns)\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                client_id, ORDERS_PER_CLIENT * 2, total_time_ns, avg_latency_ns);
  }

  logger->log("%:% %() % All orders processed, shutting down...\n", __FILE__, __LINE__, __FUNCTION__,
              Common::getCurrentTimeStr(&time_str));

  trade_engine->stop();
  market_data_consumer->stop();
  order_gateway->stop();

  delete logger;
  logger = nullptr;
  delete trade_engine;
  trade_engine = nullptr;
  delete market_data_consumer;
  market_data_consumer = nullptr;
  delete order_gateway;
  order_gateway = nullptr;

  exit(EXIT_SUCCESS);
}
