// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainShopWidget.generated.h"

class UHorizontalBox;

UCLASS()
class EC_CLIENT_API UMainShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnShopDataUpdated(bool bIsSuccess, int32 RemainGold, const TArray<int32>& ShopUnits);
	
protected:
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* CardContainer;
};
