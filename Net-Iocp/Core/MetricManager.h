#pragma once

#include <atomic>
#include <string>
#include <chrono>

class MetricManager
{
private:
    std::atomic<uint64_t> m_uPacketCounter;
    std::atomic<uint64_t> m_uRetransmitCounter;
    std::atomic<float> m_fLatencyGauge;
    
    // System Resource Metrics
    std::atomic<uint64_t> m_uActiveSessions;
    std::atomic<uint64_t> m_uPoolAvailable;
    std::atomic<float> m_fWorkerCpuUsage;
    
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    
    MetricManager() 
        : m_uPacketCounter(0), 
          m_uRetransmitCounter(0),
          m_fLatencyGauge(0.0f),
          m_uActiveSessions(0),
          m_uPoolAvailable(0),
          m_fWorkerCpuUsage(15.0f) // Simulated baseline CPU usage
    {
        m_startTime = std::chrono::steady_clock::now();
    }
    
    ~MetricManager() = default;
    MetricManager(const MetricManager&) = delete;
    MetricManager& operator=(const MetricManager&) = delete;

public:
    static MetricManager& GetInstance()
    {
        static MetricManager instance;
        return instance;
    }

    void RecordPacket()
    {
        m_uPacketCounter.fetch_add(1, std::memory_order_relaxed);
    }
    
    void RecordRetransmit()
    {
        m_uRetransmitCounter.fetch_add(1, std::memory_order_relaxed);
    }

    void SetLatency(float latency)
    {
        m_fLatencyGauge.store(latency, std::memory_order_relaxed);
    }
    
    void SetSystemMetrics(uint64_t activeSessions, uint64_t poolAvailable)
    {
        m_uActiveSessions.store(activeSessions, std::memory_order_relaxed);
        m_uPoolAvailable.store(poolAvailable, std::memory_order_relaxed);
        
        // Simulate CPU fluctuation between 15% and 30% for demonstration
        float newCpu = 15.0f + static_cast<float>(rand() % 150) / 10.0f;
        m_fWorkerCpuUsage.store(newCpu, std::memory_order_relaxed);
    }

    std::string SerializeMetrics() const
    {
        auto now = std::chrono::steady_clock::now();
        auto uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
        
        std::string out;
        
        // Global Metrics
        out += "# HELP netiocp_uptime_seconds Server uptime in seconds\n";
        out += "# TYPE netiocp_uptime_seconds gauge\n";
        out += "netiocp_uptime_seconds " + std::to_string(uptimeSec) + "\n";
        
        out += "# HELP netiocp_active_sessions Currently active client sessions\n";
        out += "# TYPE netiocp_active_sessions gauge\n";
        out += "netiocp_active_sessions " + std::to_string(m_uActiveSessions.load(std::memory_order_relaxed)) + "\n";
        
        // Network Performance
        out += "# HELP netiocp_packet_count Total number of packets processed\n";
        out += "# TYPE netiocp_packet_count counter\n";
        out += "netiocp_packet_count " + std::to_string(m_uPacketCounter.load(std::memory_order_relaxed)) + "\n";
        
        out += "# HELP netiocp_retransmit_count Total number of RUDP packets retransmitted\n";
        out += "# TYPE netiocp_retransmit_count counter\n";
        out += "netiocp_retransmit_count " + std::to_string(m_uRetransmitCounter.load(std::memory_order_relaxed)) + "\n";
        
        out += "# HELP netiocp_latency_ms Current server latency in milliseconds\n";
        out += "# TYPE netiocp_latency_ms gauge\n";
        out += "netiocp_latency_ms " + std::to_string(m_fLatencyGauge.load(std::memory_order_relaxed)) + "\n";
        
        // System Resource
        out += "# HELP netiocp_pool_available Available objects in LockFreeObjectPool\n";
        out += "# TYPE netiocp_pool_available gauge\n";
        out += "netiocp_pool_available " + std::to_string(m_uPoolAvailable.load(std::memory_order_relaxed)) + "\n";
        
        out += "# HELP netiocp_worker_cpu_usage CPU usage of IOCP worker threads (%)\n";
        out += "# TYPE netiocp_worker_cpu_usage gauge\n";
        out += "netiocp_worker_cpu_usage " + std::to_string(m_fWorkerCpuUsage.load(std::memory_order_relaxed)) + "\n";
        
        return out;
    }
};
