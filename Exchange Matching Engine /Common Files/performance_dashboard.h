#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include "latency_tracker.h"
#include "nanosecond_timer.h"

namespace Common {
  /// Real-time performance monitoring dashboard
  class PerformanceDashboard {
  private:
    struct alignas(64) PerformanceMetrics {
      std::atomic<uint64_t> orders_per_second_{0};
      std::atomic<uint64_t> trades_per_second_{0};
      std::atomic<uint64_t> avg_latency_ns_{0};
      std::atomic<uint64_t> p99_latency_ns_{0};
      std::atomic<uint64_t> p99_9_latency_ns_{0};
      std::atomic<uint64_t> memory_usage_bytes_{0};
      std::atomic<uint64_t> cpu_usage_percent_{0};
    };
    
    PerformanceMetrics metrics_;
    std::thread reporter_thread_;
    std::atomic<bool> running_{false};
    
    // Track throughput
    std::atomic<uint64_t> last_orders_count_{0};
    std::atomic<uint64_t> last_trades_count_{0};
    uint64_t last_report_time_{0};
    
  public:
    PerformanceDashboard() = default;
    
    ~PerformanceDashboard() {
      stop();
    }
    
    /// Start the performance monitoring dashboard
    void start() {
      running_.store(true, std::memory_order_release);
      reporter_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
          update_metrics();
          publish_metrics();
          
          std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Update every 100ms
        }
      });
    }
    
    /// Stop the performance monitoring dashboard
    void stop() {
      running_.store(false, std::memory_order_release);
      if (reporter_thread_.joinable()) {
        reporter_thread_.join();
      }
    }
    
    /// Record an order processed
    void record_order() noexcept {
      // This would be called from the matching engine
      // Implementation depends on your order tracking mechanism
    }
    
    /// Record a trade executed
    void record_trade() noexcept {
      // This would be called from the matching engine when a trade occurs
      // Implementation depends on your trade tracking mechanism
    }
    
    /// Get current metrics
    PerformanceMetrics get_metrics() const noexcept {
      PerformanceMetrics result;
      result.orders_per_second_ = metrics_.orders_per_second_.load(std::memory_order_relaxed);
      result.trades_per_second_ = metrics_.trades_per_second_.load(std::memory_order_relaxed);
      result.avg_latency_ns_ = metrics_.avg_latency_ns_.load(std::memory_order_relaxed);
      result.p99_latency_ns_ = metrics_.p99_latency_ns_.load(std::memory_order_relaxed);
      result.p99_9_latency_ns_ = metrics_.p99_9_latency_ns_.load(std::memory_order_relaxed);
      result.memory_usage_bytes_ = metrics_.memory_usage_bytes_.load(std::memory_order_relaxed);
      result.cpu_usage_percent_ = metrics_.cpu_usage_percent_.load(std::memory_order_relaxed);
      return result;
    }
    
    /// Get performance summary as string
    std::string get_performance_summary() const {
      auto m = get_metrics();
      
      std::string summary = "=== NANOSECOND HFT PERFORMANCE DASHBOARD ===\n";
      summary += "Orders/sec: " + std::to_string(m.orders_per_second_) + "\n";
      summary += "Trades/sec: " + std::to_string(m.trades_per_second_) + "\n";
      summary += "Avg Latency: " + std::to_string(m.avg_latency_ns_) + " ns\n";
      summary += "P99 Latency: " + std::to_string(m.p99_latency_ns_) + " ns\n";
      summary += "P99.9 Latency: " + std::to_string(m.p99_9_latency_ns_) + " ns\n";
      summary += "Memory Usage: " + std::to_string(m.memory_usage_bytes_ / 1024 / 1024) + " MB\n";
      summary += "CPU Usage: " + std::to_string(m.cpu_usage_percent_) + "%\n";
      summary += "Latency Stats: " + g_latency_tracker.get_stats_string() + "\n";
      summary += "===============================================\n";
      
      return summary;
    }
    
  private:
    void update_metrics() noexcept {
      // Update latency metrics from global tracker
      metrics_.avg_latency_ns_.store(g_latency_tracker.get_average_latency(), std::memory_order_relaxed);
      metrics_.p99_latency_ns_.store(g_latency_tracker.get_percentile_latency(99.0), std::memory_order_relaxed);
      metrics_.p99_9_latency_ns_.store(g_latency_tracker.get_percentile_latency(99.9), std::memory_order_relaxed);
      
      // Calculate throughput (this is a simplified version)
      auto current_time = NanosecondTimer::now_ns();
      if (last_report_time_ == 0) {
        last_report_time_ = current_time;
        return;
      }
      
      auto time_diff_ns = current_time - last_report_time_;
      if (time_diff_ns > 1000000000) { // Update every second
        // Calculate orders per second (simplified)
        auto current_orders = g_latency_tracker.get_total_operations();
        auto orders_diff = current_orders - last_orders_count_.load(std::memory_order_relaxed);
        auto orders_per_sec = (orders_diff * 1000000000) / time_diff_ns;
        
        metrics_.orders_per_second_.store(orders_per_sec, std::memory_order_relaxed);
        last_orders_count_.store(current_orders, std::memory_order_relaxed);
        last_report_time_ = current_time;
      }
      
      // Update memory usage (simplified)
      // In a real implementation, you would read from /proc/self/status or similar
      metrics_.memory_usage_bytes_.store(1024 * 1024 * 512, std::memory_order_relaxed); // Placeholder
      
      // Update CPU usage (simplified)
      // In a real implementation, you would read from /proc/stat or similar
      metrics_.cpu_usage_percent_.store(75, std::memory_order_relaxed); // Placeholder
    }
    
    void publish_metrics() noexcept {
      // In a real implementation, this would publish to:
      // - Prometheus metrics
      // - Grafana dashboards
      // - InfluxDB
      // - Custom monitoring systems
      
      // For now, just log the summary periodically
      static uint64_t log_counter = 0;
      if (++log_counter % 100 == 0) { // Log every 10 seconds (100 * 100ms)
        std::cout << get_performance_summary() << std::endl;
      }
    }
  };
  
  /// Global performance dashboard instance
  extern PerformanceDashboard g_performance_dashboard;
}

/// Convenience macros for performance tracking
#define RECORD_ORDER() Common::g_performance_dashboard.record_order()
#define RECORD_TRADE() Common::g_performance_dashboard.record_trade()
