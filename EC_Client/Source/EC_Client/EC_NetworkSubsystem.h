// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include "ECPacket.h"

#include "EC_NetworkSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLoginResponse, bool, bIsSuccess, bool, bHasProfile, FString, Nickname);

UCLASS()
class EC_CLIENT_API UEC_NetworkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	virtual bool IsTickable() const override { return true; }

	UFUNCTION(BlueprintCallable, category = "Network")
	bool ConnectToServer(const FString& IPAddress, int32 Port);

	UFUNCTION(BlueprintCallable, category = "Network")
	void Disconnect();

	UFUNCTION(BlueprintCallable, category = "Network")
	bool SendMessage(const FString& Message);

	/*======================
			로그인
	======================*/

	UFUNCTION(BlueprintCallable, category = "Network")
	bool SendLoginRequest(const FString& ID, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void ReceivePacket();

	UPROPERTY(BlueprintAssignable, Category = "Network|Event")
	FOnLoginResponse OnLoginResponseEvent;

private:
	FSocket* ClientSocket = nullptr;
};
