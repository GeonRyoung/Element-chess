// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "UnitData.generated.h"

USTRUCT(BlueprintType)
struct FUnitDataRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Data")
	FText UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Data")
	FText UnitDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Data")
	TSoftObjectPtr<UTexture2D> UnitImage;
};