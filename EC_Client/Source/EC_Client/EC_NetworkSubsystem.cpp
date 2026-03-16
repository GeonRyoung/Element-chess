#include "EC_NetworkSubsystem.h"

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
