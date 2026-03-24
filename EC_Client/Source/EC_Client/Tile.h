// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

UCLASS()
class EC_CLIENT_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	int32 GridX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Info")
	int32 GridY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* TileMesh;

};
