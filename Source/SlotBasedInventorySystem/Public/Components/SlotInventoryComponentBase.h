// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structures/SlotInventorySystemStructs.h"
#include "SlotInventoryComponentBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryCapacityChangedSignature, USlotInventoryComponentBase*, SlotInventoryComponent, int32, NewCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryContentChangedSignature, USlotInventoryComponentBase*, SlotInventoryComponent, const TArray<int32>&, ChangedSlots);

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SLOTBASEDINVENTORYSYSTEM_API USlotInventoryComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:

	USlotInventoryComponentBase();

	UPROPERTY(BlueprintAssignable)
	FOnInventoryCapacityChangedSignature OnInventoryCapacityChanged;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryContentChangedSignature OnInventoryContentChanged;


	/** Content Management */

	UFUNCTION(BlueprintCallable, Category = "Content")
	const FInventoryContent& GetContent() const;

	UFUNCTION(BlueprintCallable, Category = "Content")
	void SetContent(const FInventoryContent& NewContent);

	UFUNCTION(BlueprintCallable, Category = "Content|Capacity")
	int32 GetContentCapacity() const;

	UFUNCTION(BlueprintCallable, Category = "Content|Capacity")
	void SetContentCapacity(int32 NewCapacity);


	/** Slot Management */

	UFUNCTION(BlueprintCallable, Category = "Content|Slot")
	bool GetSlotValueAtIndex(int32 Index, FInventorySlot& SlotValue) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot")
	bool SetSlotValueAtIndex(int32 Index, const FInventorySlot& NewSlotValue);

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity")
	bool IsEmptySlotAtIndex(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity")
	bool ClearSlotAtIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity") // TODO: Move to BP lib
	int32 GetEmptySlotCounts() const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity") // TODO: Move to BP lib
	bool ContainsOnlyEmptySlots() const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity")
	void ModifySlotQuantityAtIndex(int32 Index, int32 ModifyAmount, bool bAllOrNothing, int32& Overflow);

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity") // TODO: Move to external interface IInventoryRule
	virtual int32 GetMaxStackSizeForID(const FName& ID) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Quantity") // TODO: Remove when IInventoryRule is ready
	void GetMaxStackSizeForIds(const TSet<FName>& Ids, TMap<FName, int32>& MaxStackSizes) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Modifier")
	bool AddModifierToSlotAtIndex(int32 Index, const FItemModifier& NewModifier);


	/** Content Management */

	UFUNCTION(BlueprintCallable, Category = "Content|Quantity") // TODO: Move to BP lib
	int32 GetItemQuantity(const FName& Item) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Modify")
	bool ModifyContent(const TMap<FName, int32>& Items, TMap<FName, int32>& Overflows);

	UFUNCTION(BlueprintCallable, Category = "Content|Modify")
	bool TryModifyContentWithoutOverflow(const TMap<FName, int32>& Items);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	bool DropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponentBase* Destination, int32 DestinationIndex, int32 MaxAmount = 255);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	bool DropSlotTowardOtherInventory(int32 SourceIndex, USlotInventoryComponentBase* Destination);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	void RegroupSimilarItemsAtIndex(int32 Index);


protected:

	/** Content Management */

	const TMap<FName, int32> GetMaxStackSizesFromIds(const TMap<FName, int32>& IdsAndCounts) const; // TODO: Remove when IInventoryRule


	/** Slot Updating */

	virtual void BroadcastContentUpdate();

	void MarkDirtySlot(int32 SlotIndex);

	void MarkSlotsHaveBeenModified();


public:

	/**
	 * The purpose of the tick function is to trigger an update broadcast only once.
	 * We can use mark modified content multiple times in a same tick but they will
	 * be cached and only broadcast in the next tick all at once.
	*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

		SetComponentTickEnabled(false);
		BroadcastContentUpdate();
	}


protected:

	UPROPERTY(EditAnywhere, Category = "Content", meta = (AllowPrivateAccess = true))
	FInventoryContent Content;

	TSet<int32> DirtySlots;

};
