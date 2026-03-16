// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include "EC_NetworkSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class EC_CLIENT_API UEC_NetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, category = "Network")
	bool ConnectToServer(const FString& IPAddress, int32 Port);

	UFUNCTION(BlueprintCallable, category = "Network")
	void Disconnect();

	UFUNCTION(BlueprintCallable, category = "Network")
	bool SendMessage(const FString& Message);

private:
	FSocket* ClientSocket = nullptr;
};
