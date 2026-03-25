#include "EC_NetworkSubsystem.h"
#include "ECPacket.h"

void UEC_NetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("[NetworkSubsystem] Initialized."));
}

void UEC_NetworkSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

void UEC_NetworkSubsystem::Tick(float DeltaTime) 
{
	ReceivePacket();
}

bool UEC_NetworkSubsystem::ConnectToServer(const FString& IPAddress, int32 Port)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem) return false;

	ClientSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("EchoClientSocket"), false);
	if (!ClientSocket) return false;

	FIPv4Address IPv4Addr;
	FIPv4Address::Parse(IPAddress, IPv4Addr);

	TSharedRef<FInternetAddr> ServerAddr = SocketSubsystem->CreateInternetAddr();
	ServerAddr->SetIp(IPv4Addr.Value);
	ServerAddr->SetPort(Port);

	bool bConnected = ClientSocket->Connect(*ServerAddr);
	if (bConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] Successfully connected to server %s:%d"), *IPAddress, Port);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Network] Failed to connect to server."));
		Disconnect();
	}

	return bConnected;
}

void UEC_NetworkSubsystem::Disconnect()
{
	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[Network] Disconnected from server."));
	}
}

bool UEC_NetworkSubsystem::SendMessage(const FString& Message)
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		return false;
	}

	FTCHARToUTF8 Convert(*Message);
	int32 BytesSent = 0;

	bool bSuccessful = ClientSocket->Send((uint8*)Convert.Get(), Convert.Length(), BytesSent);

	if (bSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] Sent message: %s"), *Message);
	}

	return bSuccessful;
}

/*======================
		로그인
======================*/

bool UEC_NetworkSubsystem::SendLoginRequest(const FString& ID, const FString& Password)
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		UE_LOG(LogTemp, Error, TEXT("[Network] 서버에 연결되어 있지 않습니다."));
		return false;
	}

	FPKT_C2S_LoginReq Packet;
	Packet.Header.Size = sizeof(FPKT_C2S_LoginReq);
	Packet.Header.ID = (uint16)EPacketID::LoginReq;

	FTCHARToUTF8 ConvertedID(*ID);
	FTCHARToUTF8 ConvertedPW(*Password);

	FMemory::Memzero(Packet.Username, 32);
	FMemory::Memzero(Packet.Password, 32);

	strncpy_s(Packet.Username, ConvertedID.Get(), 31);
	strncpy_s(Packet.Password, ConvertedPW.Get(), 31);

	int32 BytesSent = 0;
	bool bSuccessful = ClientSocket->Send((uint8*)&Packet, Packet.Header.Size, BytesSent);

	if (bSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] 로그인 요청 패킷 전송 완료! ID: %s"), *ID);
	}

	return bSuccessful;
}

bool UEC_NetworkSubsystem::SendCreateNicknameRequest(const FString& Nickname)
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		UE_LOG(LogTemp, Error, TEXT("[Network] 서버에 연결되어 있지 않습니다."));
		return false;
	}

	FPKT_C2S_CreateNicknameReq Packet;
	Packet.Header.Size = sizeof(FPKT_C2S_CreateNicknameReq);
	Packet.Header.ID = (uint16)EPacketID::CreateNicknameReq;
	 
	FTCHARToUTF8 ConvertedNickname(*Nickname);
	FMemory::Memzero(Packet.Nickname, 32);
	strncpy(Packet.Nickname, ConvertedNickname.Get(), 31);

	int32 BytesSent = 0;
	bool bSuccessful = ClientSocket->Send((uint8*)&Packet, Packet.Header.Size, BytesSent);

	if (bSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] 닉네임 생성 요청 패킷 전송 완료! 닉네임: %s"), *Nickname);
	}

	return bSuccessful;
}

bool UEC_NetworkSubsystem::EnterGameRequest()
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected)
	{
		UE_LOG(LogTemp, Error, TEXT("[Network] 서버에 연결되어 있지 않습니다."));
		return false;
	}

	FPKT_C2S_CreateNicknameReq Packet;
	Packet.Header.Size = sizeof(FPKT_C2S_CreateNicknameReq);
	Packet.Header.ID = (uint16)EPacketID::CreateNicknameReq;
	int32 ByteSent = 0;
	bool bSuccessful = ClientSocket->Send((uint8*)&Packet, Packet.Header.Size, ByteSent);

	if (bSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] 게임 진입 요청"));
	}

	return bSuccessful;
}

bool UEC_NetworkSubsystem::SendRefreshShopRequest()
{
	FPKT_C2S_RefreshShopReq Packet;
	Packet.header.Size = sizeof(FPKT_C2S_RefreshShopReq);
	Packet.header.ID = (uint16_t)EPacketID::RefreshShopReq;
	int32 BytesSent = 0;

	bool bSuccessful = ClientSocket->Send((uint8*)&Packet, Packet.header.Size, BytesSent);

	if (bSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network] 상점 리롤 패킷(2골드 소모) 서버로 전송 완료!"));
	}

	return bSuccessful;
}

void UEC_NetworkSubsystem::ReceivePacket()
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected) return;

	uint32 PendingDataSize = 0;

	if (ClientSocket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
	{
		TArray<uint8> ReceiveBuffer;
		ReceiveBuffer.SetNumUninitialized(PendingDataSize);

		int32 BytesRead = 0;
		if (ClientSocket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead))
		{
			FPacketHeader* Header = (FPacketHeader*)ReceiveBuffer.GetData();

			switch (Header->ID)
			{
			case (uint16)EPacketID::LoginRes:
			{
				FPKT_S2C_LoginRes* ResPacket = (FPKT_S2C_LoginRes*)ReceiveBuffer.GetData();
				FString ReceivedNickname = UTF8_TO_TCHAR(ResPacket->Nickname);

				OnLoginResponseEvent.Broadcast(ResPacket->bSuccess, ResPacket->bHasProfile, ReceivedNickname);

				UE_LOG(LogTemp, Warning, TEXT("[Network] 로그인 응답! 성공: %s, 프로필: %s, 닉네임: %s"),
					ResPacket->bSuccess ? TEXT("True") : TEXT("False"),
					ResPacket->bHasProfile ? TEXT("True") : TEXT("False"),
					*ReceivedNickname);
				break;
			}
			case (uint16)EPacketID::CreateNicknameRes:
			{
				FPKT_S2C_CreateNicknameRes* ResPacket = (FPKT_S2C_CreateNicknameRes*)ReceiveBuffer.GetData();

				OnCreateNicknameResponseEvent.Broadcast(ResPacket->bSuccess);
				break;
			}
			case (uint16)EPacketID::EnterGameRes:
			{
				FPKT_S2C_EnterGameRes * ResPacket = (FPKT_S2C_EnterGameRes*)ReceiveBuffer.GetData();
				//[TODO]
				break;
			}
			default:
			{
				UE_LOG(LogTemp, Warning, TEXT("[Network] 알 수 없는 패킷 수신 (ID: %d)"), Header->ID);
				break;
			}
			}
		}
	}
}
