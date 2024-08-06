// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SlotInventorySystemStructs.generated.h"

class USlotModifier;


USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventorySlot
{
	GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	FName ID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	int32 Count = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Instanced)
    TArray<TObjectPtr<USlotModifier>> Modifiers;


	bool IsEmpty() const;
	void Reset();
	void ModifyCountWithOverflow(int32 ModifyAmount, int32& Overflow, int32 MaxStackSize = 255);
	bool TryModifyCountByExact(int32 ModifyAmount, int32 MaxStackSize = 255);
	bool AddIdAndCount(const FName& SlotId, int32 ModifyAmount, int32& Overflow, int32 MaxStackSize = 255);
	bool AcceptStackAdditions() const;

	USlotModifier* GetModifierByClass(TSubclassOf<USlotModifier> ModifierClass) const;
	void GetModifiersByClass(TSubclassOf<USlotModifier> ModifierClass, TArray<USlotModifier*>& Modifiers) const;
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

	void ModifyContentWithValues(const TMap<FName, int32>& IdsAndCounts, const TMap<FName, int32>& MaxStackSizes, FContentModificationResult& ModificationResult);

	void ReceiveSlotOverflow(const FName& SlotId, int32& InoutOverflow, int32 MaxStackSize, bool bTargetEmptySlots, FContentModificationResult& ModificationResult);
	bool ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, int32 MaxStackSize = 255, int32 MaxTransferAmount = 255);

	void RegroupSlotsWithSimilarIdsAtIndex(int32 Index, FContentModificationResult& ModificationResult, int32 MaxStackSize = 255, FInventorySlot* CachedSlotPtr = nullptr);

	static bool MergeSlotsWithSimilarIds(FInventorySlot& DestinationSlot, FInventorySlot& SourceSlot, int32 MaxStackSize = 255, int32 MaxTransferAmount = 255);

	static void SwapSlots(FInventorySlot& FirstSlot, FInventorySlot& SecondSlot);

	int32 GetFirstEmptySlotIndex() const;
};
