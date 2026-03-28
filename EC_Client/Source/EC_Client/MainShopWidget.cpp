#include "MainShopWidget.h"
#include "ShopWidget.h"
#include "EC_NetworkSubsystem.h" 
#include "Components/HorizontalBox.h"
#include "Engine/GameInstance.h"

void UMainShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UEC_NetworkSubsystem* NetSubsystem = GI->GetSubsystem<UEC_NetworkSubsystem>())
		{
			NetSubsystem->OnRefreshShopResponseEvent.AddDynamic(this, &UMainShopWidget::OnShopDataUpdated);
		}
	}
}

void UMainShopWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UEC_NetworkSubsystem* NetSubsystem = GI->GetSubsystem<UEC_NetworkSubsystem>())
		{
			NetSubsystem->OnRefreshShopResponseEvent.RemoveDynamic(this, &UMainShopWidget::OnShopDataUpdated);
		}
	}
	
	Super::NativeDestruct();
}

void UMainShopWidget::OnShopDataUpdated(bool bIsSuccess, int32 RemainGold, const TArray<int32>& ShopUnits)
{
	if (!bIsSuccess || CardContainer == nullptr) return;

	int32 CardCount = CardContainer->GetChildrenCount();

	for (int32 i = 0; i < ShopUnits.Num(); ++i)
	{
		if (i >= CardCount) break;

		if (UShopWidget* ShopCard = Cast<UShopWidget>(CardContainer->GetChildAt(i)))
		{
			ShopCard->UpdateShopSlot(ShopUnits[i]);
		}
	}
}