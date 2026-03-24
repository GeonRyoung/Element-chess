#include "GridManager.h"
#include "Tile.h"
#include "Engine/World.h"




AGridManager::AGridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

ATile* AGridManager::GetTileAt(int32 X, int32 Y)
{
	FIntPoint Coordinate(X, Y);
	if (GridMap.Contains(Coordinate))
	{
		return GridMap[Coordinate];
	}
	return nullptr;
}

void AGridManager::GenerateGrid()
{
	ClearGrid();

	if (!TileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("TileClass가 비어있습니다! 디테일 패널에서 설정해주세요."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Columns; ++Col)
		{
			float XPos = (Col * TileWidth) + ((Row % 2 == 1) ? OddRowXOffset : 0.0f);
			float YPos = Row * RowHeightOffset;
			FVector TileLocation = GetActorLocation() + FVector(XPos, YPos, 0.0f);

			float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
			FRotator RandomRotation = FRotator(0.0f, RandomYaw, 0.0f);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ATile* NewTile = World->SpawnActor<ATile>(TileClass, TileLocation, RandomRotation, SpawnParams);

			if (NewTile)
			{
				NewTile->SetActorScale3D(TileScale);

				NewTile->GridX = Col;
				NewTile->GridY = Row;

				NewTile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				GridMap.Add(FIntPoint(Col, Row), NewTile);
			}
		}
	}
}

void AGridManager::ClearGrid()
{
	for (auto& Pair : GridMap)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}

	GridMap.Empty();
}


