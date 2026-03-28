#include "ShopWidget.h"

void UShopWidget::UpdateShopSlot(int32 ReceivedUnitID)
{
	if (UnitDataTable == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitDataTable이 연결되지 않았습니다!"));
		return;
	}

	FName RowName = FName(*FString::FromInt(ReceivedUnitID));

	FUnitDataRow* FoundUnitData = UnitDataTable->FindRow<FUnitDataRow>(RowName, TEXT("ShopSlotUpdateContext"));

	if (FoundUnitData != nullptr)
	{
		if (Text_UnitName)
		{
			Text_UnitName->SetText(FoundUnitData->UnitName);
		}
        
		if (Text_UnitDesc)
		{
			Text_UnitDesc->SetText(FoundUnitData->UnitDescription);
		}

		UE_LOG(LogTemp, Log, TEXT("유닛 정보 로드 성공: %s"), *FoundUnitData->UnitName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ID %d에 해당하는 유닛 데이터를 찾을 수 없습니다!"), ReceivedUnitID);
	}
}

