// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SlotInventorySystemStructs.generated.h"


USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	FName ID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	uint8 Count = 0;


	bool IsEmpty() const;
	void Reset();
	void ModifyCountWithOverflow(int32 ModifyAmount, int32& Overflow, uint8 MaxStackSize = 255);
	bool TryModifyCountByExact(int32 ModifyAmount, uint8 MaxStackSize = 255);
	bool AddIdAndCount(const FName& SlotId, int32 ModifyAmount, int32& Overflow, uint8 MaxStackSize = 255);

};


USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventoryContent
{
    GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	TArray<FInventorySlot> Slots;


	bool IsValidIndex(int32 Index) const;

	FInventorySlot* GetSlotPtrAtIndex(int32 Index);
	const FInventorySlot* GetSlotConstPtrAtIndex(int32 Index) const;

	struct FContentModificationResult
	{
		bool bModifiedSomething;
		bool bCreatedEmptySlot;
		TSet<int32>* ModifiedSlots;
		TMap<FName, int32>* Overflows;

		FContentModificationResult(TSet<int32>* InModifiedSlots, TMap<FName, int32>* Overflows);
	};

	void ModifyContentWithValues(const TMap<FName, int32>& IdsAndCounts, const TMap<FName, uint8>& MaxStackSizes, FContentModificationResult& ModificationResult);

	void ReceiveSlotOverflow(const FName& SlotId, int32& InoutOverflow, uint8 MaxStackSize, bool bTargetEmptySlots, FContentModificationResult& ModificationResult);
	bool ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, uint8 MaxStackSize = 255, uint8 MaxTransferAmount = 255);

	void RegroupSlotsWithSimilarIdsAtIndex(int32 Index, FContentModificationResult& ModificationResult, uint8 MaxStackSize = 255, FInventorySlot* CachedSlotPtr = nullptr);

	static bool MergeSlotsWithSimilarIds(FInventorySlot& DestinationSlot, FInventorySlot& SourceSlot, uint8 MaxStackSize = 255, uint8 MaxTransferAmount = 255);

	static void SwapSlots(FInventorySlot& FirstSlot, FInventorySlot& SecondSlot);

};
