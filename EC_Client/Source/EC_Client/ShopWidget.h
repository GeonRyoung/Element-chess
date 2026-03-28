// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Engine/DataTable.h"
#include "UnitData.h"
#include "Components/TextBlock.h"

#include "ShopWidget.generated.h"


UCLASS()
class EC_CLIENT_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
		
public:
	void UpdateShopSlot(int32 ReceiveUnitID);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Data")
	UDataTable* UnitDataTable;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_UnitName;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_UnitDesc;
};
