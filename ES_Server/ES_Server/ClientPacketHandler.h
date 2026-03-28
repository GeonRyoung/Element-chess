#pragma once

class Session;

class ClientPacketHandler
{
public:
    static void Init();
    
    static void HandlePacket(Session* session, char* packetData, uint16_t id);
    
private:
    static void Handle_LoginReq(Session* session, char* packetData);
    static void Handle_CreateNicknameReq(Session* session, char* packetData);
    static void Handle_EnterGameReq(Session* session, char* packetData);
    static void Handle_RefreshShopReq(Session* session, char* packetData);
    
    static std::unordered_map<uint16_t, std::function<void(Session*,char*)>> _packetHandlers;
};
