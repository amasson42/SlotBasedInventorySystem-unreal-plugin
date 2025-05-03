// Amasson

#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "SlotInventorySystemStructs.generated.h"

USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventorySlotTransactionRule
{
	GENERATED_USTRUCT_BODY()

	/** If true, the transaction must be fully completed or it fails (no partial transfers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAtomic = false;

	/** If true, the transaction can swap items between source and target slots if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAllowSwap = true;

	/** If true, merging is the only allowed behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bOnlyMerge = false;

	/** Maximum number of items allowed to transfer. 0 means no maximum (transfer all possible). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	int32 MaxTransferQuantity = 0;
};

USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FItemModifier
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ItemModifier")
	FName Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ItemModifier")
	FInstancedStruct Data;
};

USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventorySlot // : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "Slot")
	FName Item;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "Slot")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "Slot")
    TArray<FItemModifier> Modifiers;


	/** Is the slot empty */
	bool IsEmpty() const;

	/** Reset the slot to an empty value */
	void Reset();

	/** Receive a stack of item */
	bool ReceiveStack(const FName& InItem, int32& InoutQuantity, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize);

	/** Receive another slot */
	bool ReceiveSlot(FInventorySlot& SourceSlot, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize);

	const FItemModifier* GetConstModifierByType(const FName& ModifierType) const;
	FItemModifier* GetModifierByType(const FName& ModifierType);
	void GetConstModifiersByType(const FName& ModifierType, TArray<const FItemModifier*>& Modifiers) const;
	void GetModifiersByType(const FName& ModifierType, TArray<FItemModifier*>& Modifiers);
};


USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventoryContent // : public FFastArraySerializer
{
    GENERATED_USTRUCT_BODY()


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


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "Content")
	TArray<FInventorySlot> Slots;
};

// template<>
// struct TStructOpsTypeTraits<FInventoryContent> : public TStructOpsTypeTraitsBase2<FInventoryContent>
// {
// 	enum { WithNetDeltaSerializer = true };
// };
