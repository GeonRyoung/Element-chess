#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include "Packet.h"

class RudpSession;

using HandlerFunc = std::function<void(std::shared_ptr<RudpSession>, PacketPtr)>;

class IExecutionService;

class PacketDispatcher
{
private:
    std::unordered_map<uint16_t, HandlerFunc> m_handlerMap;
    IExecutionService* m_pExecutionService = nullptr;

public:
    PacketDispatcher() = default;
    ~PacketDispatcher() = default;

    void SetExecutionService(IExecutionService* service) { m_pExecutionService = service; }

    void RegisterHandler(uint16_t opcode, HandlerFunc handler);
    void Dispatch(std::shared_ptr<RudpSession> session, PacketPtr packet);
};
