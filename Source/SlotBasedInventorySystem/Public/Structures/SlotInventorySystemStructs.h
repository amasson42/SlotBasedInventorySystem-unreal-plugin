// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "InstancedStruct.h"
#include "SlotInventorySystemStructs.generated.h"


USTRUCT(BlueprintType)
struct FItemModifier
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FInstancedStruct Data;
};

USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventorySlot // : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	FName Item;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
    TArray<FItemModifier> Modifiers;


	bool IsEmpty() const;
	void Reset();
	bool ModifyQuantity(int32& InoutQuantity, bool bAllOrNothing = false, int32 MaxStackSize = 255);
	bool ReceiveStack(const FName& InItem, int32& InoutQuantity, bool bAllOrNothing = false, int32 MaxStackSize = 255);
	bool ReceiveSlot(FInventorySlot& SourceSlot, int32 MaxTransfertAmount, int32 MaxStackSize = 255);

	const FItemModifier* GetConstModifierByType(const FName& ModifierType) const;
	FItemModifier* GetModifierByType(const FName& ModifierType);
	void GetConstModifiersByType(const FName& ModifierType, TArray<const FItemModifier*>& Modifiers) const;
	void GetModifiersByType(const FName& ModifierType, TArray<FItemModifier*>& Modifiers);
};


USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventoryContent // : public FFastArraySerializer
{
    GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame)
	TArray<FInventorySlot> Slots;


	bool IsValidIndex(int32 Index) const;

	FInventorySlot* GetSlotPtrAtIndex(int32 Index);
	const FInventorySlot* GetSlotConstPtrAtIndex(int32 Index) const;

	using FItemsPack = TMap<FName, int32>;

	struct FContentModificationResult
	{
		bool bModifiedSomething = false;
		bool bCreatedEmptySlot = false;
		TSet<int32>* ModifiedSlots = nullptr;
		FItemsPack* Overflows = nullptr;

		FContentModificationResult(TSet<int32>* InModifiedSlots, FItemsPack* Overflows);
	};

	void ModifyContent(const FItemsPack& Items, const TMap<FName, int32>& MaxStackSizes, FContentModificationResult& ModificationResult);

	void ReceiveStack(const FName& Item, int32& InoutQuantity, int32 MaxStackSize, bool bTargetOccupiedSlots, FContentModificationResult& ModificationResult);
	bool ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, int32 MaxStackSize = 255, int32 MaxTransferAmount = 255);

	void RegroupSimilarItemsAtIndex(int32 Index, FContentModificationResult& ModificationResult, int32 MaxStackSize = 255, FInventorySlot* CachedSlotPtr = nullptr);

};

// template<>
// struct TStructOpsTypeTraits<FInventoryContent> : public TStructOpsTypeTraitsBase2<FInventoryContent>
// {
// 	enum { WithNetDeltaSerializer = true };
// };
