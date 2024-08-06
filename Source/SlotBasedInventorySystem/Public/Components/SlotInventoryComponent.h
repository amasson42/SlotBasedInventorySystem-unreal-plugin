// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structures/SlotInventorySystemStructs.h"
#include "SlotInventoryComponent.generated.h"

class USlotModifier;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryCapacityChangedSignature, USlotInventoryComponent*, SlotInventoryComponent, int32, NewCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryContentChangedSignature, USlotInventoryComponent*, SlotInventoryComponent, const TArray<int32>&, ChangedSlots);

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SLOTBASEDINVENTORYSYSTEM_API USlotInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USlotInventoryComponent();

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

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	bool IsEmptySlotAtIndex(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	bool ClearSlotAtIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	int32 GetEmptySlotCounts() const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	bool ContainsOnlyEmptySlots() const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	void ModifySlotCountAtIndex(int32 Index, int32 ModifyAmount, bool bAllOrNothing, int32& Overflow);

    UFUNCTION(BlueprintCallable, Category = "Content|Slot|Modifier", meta = (DeterminesOutputType = "ModifierClass"))
    USlotModifier* AddModifierToSlotAtIndex(int32 Index, TSubclassOf<USlotModifier> ModifierClass);

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	virtual int32 GetMaxStackSizeForID(const FName& ID) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Slot|Count")
	void GetMaxStackSizeForIds(const TSet<FName>& Ids, TMap<FName, int32>& MaxStackSizes) const;


	/** Content Management */

	UFUNCTION(BlueprintCallable, Category = "Content|Count")
	int32 GetContentIdCount(const FName& Id) const;

	UFUNCTION(BlueprintCallable, Category = "Content|Modify")
	bool ModifyContentWithOverflow(const TMap<FName, int32>& IdsAndCounts, TMap<FName, int32>& Overflows);

	UFUNCTION(BlueprintCallable, Category = "Content|Modify")
	bool TryModifyContentWithoutOverflow(const TMap<FName, int32>& IdsAndCounts);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	bool DropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponent* Destination, int32 DestinationIndex, int32 MaxAmount = 255);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	bool DropSlotTowardOtherInventory(int32 SourceIndex, USlotInventoryComponent* Destination);

	UFUNCTION(BlueprintCallable, Category = "Content|Action")
	void RegroupSlotAtIndexWithSimilarIds(int32 Index);


protected:

	/** Content Management */

	const TMap<FName, int32> GetMaxStackSizesFromIds(const TMap<FName, int32>& IdsAndCounts) const;


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
