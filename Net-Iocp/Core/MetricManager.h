#pragma once

#include <atomic>
#include <string>

class MetricManager
{
private:
    std::atomic<uint64_t> m_uPacketCounter;
    std::atomic<float> m_fLatencyGauge;
    
    MetricManager() : m_uPacketCounter(0), m_fLatencyGauge(0.0f) {}
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

    void SetLatency(float latency)
    {
        m_fLatencyGauge.store(latency, std::memory_order_relaxed);
    }

    std::string SerializeMetrics() const
    {
        return "{\n"
               "  \"packet_count\": " + std::to_string(m_uPacketCounter.load(std::memory_order_relaxed)) + ",\n"
               "  \"latency_ms\": " + std::to_string(m_fLatencyGauge.load(std::memory_order_relaxed)) + "\n"
               "}\n";
    }
};
