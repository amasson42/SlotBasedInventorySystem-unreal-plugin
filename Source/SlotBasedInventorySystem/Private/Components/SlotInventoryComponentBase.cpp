// Amasson


#include "Components/SlotInventoryComponentBase.h"

USlotInventoryComponentBase::USlotInventoryComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USlotInventoryComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetComponentTickEnabled(false);
	BroadcastContentUpdate();
}
/** Public Content Management */

const FInventoryContent& USlotInventoryComponentBase::GetContent() const
{
	return Content;
}

void USlotInventoryComponentBase::SetContent(const FInventoryContent& NewContent)
{
	if (NewContent.Slots.Num() != GetContentCapacity())
	{
		SetContentCapacity(NewContent.Slots.Num());
	}
	for (int32 SlotIndex = 0; SlotIndex < NewContent.Slots.Num(); SlotIndex++)
	{
		SetSlotValueAtIndex(SlotIndex, NewContent.Slots[SlotIndex]);
	}
}

int32 USlotInventoryComponentBase::GetContentCapacity() const
{
	return Content.Slots.Num();
}

void USlotInventoryComponentBase::SetContentCapacity(int32 NewCapacity)
{
	if (NewCapacity < 0)
		NewCapacity = 0;

	const int32 OldCapacity = GetContentCapacity();

	Content.Slots.SetNum(NewCapacity, true);

	if (NewCapacity > OldCapacity)
	{
		for (int32 NewSlotIndex = OldCapacity; NewSlotIndex < NewCapacity; NewSlotIndex++)
		{
			ClearSlotAtIndex(NewSlotIndex);
		}
	}
	OnInventoryCapacityChanged.Broadcast(this, NewCapacity);
}


/** Public Slot Management */

bool USlotInventoryComponentBase::GetSlotValueAtIndex(int32 Index, FInventorySlot& SlotValue) const
{
	if (const FInventorySlot* SlotPtr = Content.GetSlotConstPtrAtIndex(Index))
	{
		SlotValue = *SlotPtr;
		return true;
	}

	return false;
}

bool USlotInventoryComponentBase::SetSlotValueAtIndex(int32 Index, const FInventorySlot& NewSlotValue)
{
	if (FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index))
	{
		*SlotPtr = NewSlotValue;
		MarkDirtySlot(Index);
		return true;
	}

	return false;
}

bool USlotInventoryComponentBase::ClearSlotAtIndex(int32 Index)
{
	if (FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index))
	{
		bool bIsEmpty = SlotPtr->IsEmpty();
		SlotPtr->Reset();
		if (!bIsEmpty)
		{
			MarkDirtySlot(Index);
			return true;
		}
	}

	return false;
}

void USlotInventoryComponentBase::ModifySlotQuantityAtIndex(int32 Index, int32 ModifyAmount, bool bAllOrNothing, int32& Overflow)
{
	FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index);

    const bool bCanStack = SlotPtr && !SlotPtr->IsEmpty() && SlotPtr->Modifiers.IsEmpty();
	if (!bCanStack)
	{
		Overflow = ModifyAmount;
		return;
	}

	const int32 MaxStackSize = GetMaxStackSizeForID(SlotPtr->Item);

	if (bAllOrNothing)
	{
		Overflow = ModifyAmount;
		bool bModified = SlotPtr->ReceiveStack(SlotPtr->Item, Overflow, FInventorySlotTransactionRule(true, true, false, 0), MaxStackSize);
		if (bModified)
		{
			Overflow = 0;
			MarkDirtySlot(Index);
		}
		else
			Overflow = ModifyAmount;
	}
	else
	{
		Overflow = ModifyAmount;
		SlotPtr->ReceiveStack(SlotPtr->Item, Overflow, FInventorySlotTransactionRule(), MaxStackSize);
		if (Overflow != ModifyAmount)
			MarkDirtySlot(Index);
	}
}

bool USlotInventoryComponentBase::AddModifierToSlotAtIndex(int32 Index, const FItemModifier& NewModifier)
{
    FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index);
    if (!SlotPtr)
        return false;

    SlotPtr->Modifiers.Add(NewModifier);

    MarkDirtySlot(Index);

    return true;
}

int32 USlotInventoryComponentBase::GetMaxStackSizeForID(const FName& ID) const
{
	return 255;
}

void USlotInventoryComponentBase::GetMaxStackSizeForIds(const TSet<FName>& Ids, TMap<FName, int32>& MaxStackSizes) const
{
	for (const FName& Id : Ids)
	{
		const int32 MaxStackSize = GetMaxStackSizeForID(Id);
		MaxStackSizes.Add(Id, MaxStackSize);
	}
}


/** Content Management */

bool USlotInventoryComponentBase::ModifyContent(const TMap<FName, int32>& Items, TMap<FName, int32>& Overflows)
{
	const TMap<FName, int32>& MaxStackSizes = GetMaxStackSizesFromIds(Items);
	
	Overflows = Items;
	FInventoryContentTransactionRule Rule;
	FInventoryContent::FContentModifications ModificationResult;
	if (Content.ReceiveStacks(Overflows, Rule, MaxStackSizes, ModificationResult))
	{
		for (int32 ModifiedSlotIndex : ModificationResult.ModifiedSlots)
			MarkDirtySlot(ModifiedSlotIndex);
		return true;
	}
	return false;
}

bool USlotInventoryComponentBase::TryModifyContentWithoutOverflow(const TMap<FName, int32>& Items)
{
	const TMap<FName, int32>& MaxStackSizes = GetMaxStackSizesFromIds(Items);

	FInventoryContent TmpContent = Content;

	FInventoryContent::FItemStacks Stacks = Items;
	FInventoryContent::FContentModifications Modifications;

	FInventoryContentTransactionRule Rule;
	Rule.bAtomic = true;
	if (!TmpContent.ReceiveStacks(Stacks, Rule, MaxStackSizes, Modifications))
		return false;

	for (const auto& QuantityLeft : Stacks)
		if (QuantityLeft.Value != 0)
			return false;

	Content = TmpContent;

	for (int32 ModifiedSlotIndex : Modifications.ModifiedSlots)
		MarkDirtySlot(ModifiedSlotIndex);

	return true;
}

bool USlotInventoryComponentBase::DropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory, int32 DestinationIndex, int32 MaxAmount)
{
	if (!IsValid(DestinationInventory))
		return false;

	FInventorySlot* SourceSlot = Content.GetSlotPtrAtIndex(SourceIndex);
	if (SourceSlot == nullptr)
		return false;

	const int32 MaxStackSize = DestinationInventory->GetMaxStackSizeForID(SourceSlot->Item);

	FInventorySlotTransactionRule Rule;
	Rule.bAllowSwap = true;
	Rule.MaxTransferQuantity = MaxAmount;
	if (DestinationInventory->Content.ReceiveSlotAtIndex(*SourceSlot, DestinationIndex, Rule, MaxStackSize))
	{
		MarkDirtySlot(SourceIndex);
		DestinationInventory->MarkDirtySlot(DestinationIndex);
		return true;
	}

	return false;
}

bool USlotInventoryComponentBase::DropSlotTowardOtherInventory(int32 SourceIndex, USlotInventoryComponentBase* Destination)
{
	if (!IsValid(Destination))
		return false;

	FInventorySlot* SourceSlotPtr = Content.GetSlotPtrAtIndex(SourceIndex);
	if (SourceSlotPtr == nullptr || SourceSlotPtr->IsEmpty())
		return false;

	const int32 MaxStackSize = Destination->GetMaxStackSizeForID(SourceSlotPtr->Item);

	FInventoryContentTransactionRule Rule;
	FInventoryContent::FContentModifications Modifications;
	if (Destination->Content.ReceiveSlot(*SourceSlotPtr, Rule, MaxStackSize, Modifications))
	{
		for (int32 ModifiedSlotIndex : Modifications.ModifiedSlots)
			Destination->MarkDirtySlot(ModifiedSlotIndex);
		MarkDirtySlot(SourceIndex);
		return true;
	}

	return false;
}

bool USlotInventoryComponentBase::RegroupSimilarItemsAtIndex(int32 Index)
{
	FInventorySlot* Slot = Content.GetSlotPtrAtIndex(Index);

	if (Slot == nullptr)
		return false;

	const int32 MaxStackSize = GetMaxStackSizeForID(Slot->Item);

	FInventoryContent::FContentModifications Modifications;
	if (Content.RegroupSimilarItemsAtIndex(Index, Modifications, MaxStackSize))
	{
		for (int32 ModifiedSlotIndex : Modifications.ModifiedSlots)
			MarkDirtySlot(ModifiedSlotIndex);
		return true;
	}

	return false;
}


/** Private Content Management */

const TMap<FName, int32> USlotInventoryComponentBase::GetMaxStackSizesFromIds(const TMap<FName, int32>& IdsAndCounts) const
{
	TSet<FName> Ids;
	IdsAndCounts.GetKeys(Ids);
	TMap<FName, int32> MaxStackSizes;
	GetMaxStackSizeForIds(Ids, MaxStackSizes);
	return MaxStackSizes;
}

/** Slot Updating */

void USlotInventoryComponentBase::BroadcastContentUpdate()
{
	OnInventoryContentChanged.Broadcast(this, DirtySlots.Array());
	DirtySlots.Reset();
}

void USlotInventoryComponentBase::MarkDirtySlot(int32 SlotIndex)
{
	checkf(Content.IsValidIndex(SlotIndex), TEXT("MarkDirtySlot recieve invalid SlotIndex"));
	DirtySlots.Add(SlotIndex);
	MarkSlotsHaveBeenModified();
}

void USlotInventoryComponentBase::MarkSlotsHaveBeenModified()
{
	SetComponentTickEnabled(true);
}
