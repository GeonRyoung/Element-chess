#include "PacketDispatcher.h"
#include "RudpSession.h"
#include "IExecutionService.h"

void PacketDispatcher::RegisterHandler(uint16_t opcode, HandlerFunc handler)
{
    m_handlerMap[opcode] = handler;
}

void PacketDispatcher::Dispatch(std::shared_ptr<RudpSession> session, PacketPtr packet)
{
    if (!session || !packet)
        return;

    uint16_t opcode = static_cast<uint16_t>(packet->GetOpcode());
    auto it = m_handlerMap.find(opcode);
    if (it != m_handlerMap.end())
    {
        auto handler = it->second;
        if (m_pExecutionService)
        {
            m_pExecutionService->Execute([handler, session, packet]() {
                handler(session, packet);
            });
        }
        else
        {
            handler(session, packet);
        }
    }
}
