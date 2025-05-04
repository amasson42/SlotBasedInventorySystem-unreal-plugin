// Amasson

#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "SlotInventorySystemStructs.generated.h"

/** Rules set when moving one specific slow around */
USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventorySlotTransactionRule
{
	GENERATED_USTRUCT_BODY()

	/** If true, the transaction must be fully completed or it fails (no partial transfers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAtomic = false;

	/** If true, the transaction can swap items between source and target slots if needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAllowSwap = false;

	/** If true, merging is the only allowed behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bOnlyMerge = false;

	/** Maximum number of items allowed to transfer. 0 means unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	int32 MaxTransferQuantity = 0;
};

/** Rule set when moving multiple stacks or doing mass transfers (e.g., drag/drop many items). */
USTRUCT(BlueprintType)
struct SLOTBASEDINVENTORYSYSTEM_API FInventoryContentTransactionRule
{
	GENERATED_USTRUCT_BODY()

	/** If true, the entire transaction must succeed or none of it does (no partial stack moves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAtomic = false;

	/** If true, prioritize merging items with existing similar stacks where possible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bPreferMerge = true;

	/** If true, allows the destination inventory to internally reorganize/compact contents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Transaction")
	bool bAllowAutoStacking = true;
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

	/** Does the slot have modifiers */
	bool HasModifiers() const;

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

	using FItemStacks = TMap<FName, int32>;

	struct FContentModifications
	{
		TSet<int32> ModifiedSlots;
		bool bCreatedEmptySlot = false;
	};

	bool ReceiveStacks(FItemStacks& Stacks, const FInventoryContentTransactionRule& Rule, const TMap<FName, int32>& MaxStackSizes, FContentModifications& OutModifications);

	bool ReceiveStack(const FName& Item, int32& InoutQuantity, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize, FContentModifications& OutModifications);
	bool ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize);
	bool ReceiveSlot(FInventorySlot& InoutSlot, const FInventoryContentTransactionRule& Rule, int32 MaxStackSize, FContentModifications& OutModifications);

	bool RegroupSimilarItemsAtIndex(int32 Index, FContentModifications& OutModifications, int32 MaxStackSize);


	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "Content")
	TArray<FInventorySlot> Slots;
};

// template<>
// struct TStructOpsTypeTraits<FInventoryContent> : public TStructOpsTypeTraitsBase2<FInventoryContent>
// {
// 	enum { WithNetDeltaSerializer = true };
// };
