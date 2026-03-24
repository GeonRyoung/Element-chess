#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

class ATile;

UCLASS()
class EC_CLIENT_API AGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridManager();

	UFUNCTION(BlueprintCallable, Category = "Grid")
	ATile* GetTileAt(int32 X, int32 Y);


protected:
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	TSubclassOf<ATile> TileClass;

	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	int32 Rows = 8;
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	int32 Columns = 8;

	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	float TileWidth = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	float RowHeightOffset = 85.0f;
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	float OddRowXOffset = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	FVector TileScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(VisibleAnywhere, Category = "Grid Data")
	TMap<FIntPoint, ATile*> GridMap;

	UFUNCTION(CallInEditor, Category = "Grid Actions")
	void GenerateGrid();

	UFUNCTION(CallInEditor, Category = "Grid Actions")
	void ClearGrid();
};