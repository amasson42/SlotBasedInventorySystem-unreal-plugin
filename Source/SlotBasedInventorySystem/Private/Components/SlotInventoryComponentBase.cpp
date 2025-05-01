// Amasson


#include "Components/SlotInventoryComponentBase.h"

USlotInventoryComponentBase::USlotInventoryComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetComponentTickEnabled(false);
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

bool USlotInventoryComponentBase::IsEmptySlotAtIndex(int32 Index) const
{
	if (const FInventorySlot* SlotPtr = Content.GetSlotConstPtrAtIndex(Index))
		return SlotPtr->IsEmpty();

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

int32 USlotInventoryComponentBase::GetEmptySlotCounts() const
{
	int32 Total = 0;

	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (Slot.IsEmpty())
			++Total;
	}
	return Total;
}

bool USlotInventoryComponentBase::ContainsOnlyEmptySlots() const
{
	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (!Slot.IsEmpty())
			return false;
	}
	return true;
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
		bool bModified = SlotPtr->ModifyQuantity(Overflow, true, MaxStackSize);
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
		SlotPtr->ModifyQuantity(Overflow, false, MaxStackSize);
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

int32 USlotInventoryComponentBase::GetItemQuantity(const FName& Name) const
{
	int32 Total = 0;

	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (Slot.IsEmpty()) continue;

		if (Slot.Item == Name)
			Total += Slot.Quantity;
	}
	return Total;
}

bool USlotInventoryComponentBase::ModifyContent(const TMap<FName, int32>& Items, TMap<FName, int32>& Overflows)
{
	const TMap<FName, int32>& MaxStackSizes = GetMaxStackSizesFromIds(Items);

	TSet<int32> ModifiedSlots;
	Overflows.Reset();
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, &Overflows);
	Content.ModifyContent(Items, MaxStackSizes, ModificationResult);

	for (int32 ModifiedSlotIndex : ModifiedSlots)
	{
		MarkDirtySlot(ModifiedSlotIndex);
	}

	return true;
}

bool USlotInventoryComponentBase::TryModifyContentWithoutOverflow(const TMap<FName, int32>& Items)
{
	const TMap<FName, int32>& MaxStackSizes = GetMaxStackSizesFromIds(Items);

	TSet<int32> ModifiedSlots;

	FInventoryContent TmpContent = Content;

	TMap<FName, int32> TmpOverflows;
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, &TmpOverflows);

	TmpContent.ModifyContent(Items, MaxStackSizes, ModificationResult);

	if (!TmpOverflows.IsEmpty())
		return false;

	Content = TmpContent;

	for (int32 ModifiedSlotIndex : ModifiedSlots)
		MarkDirtySlot(ModifiedSlotIndex);

	return true;
}

bool USlotInventoryComponentBase::DropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory, int32 DestinationIndex, int32 MaxAmount)
{
	if (!IsValid(DestinationInventory))
		return false;

	FInventorySlot* SourceSlot = Content.GetSlotPtrAtIndex(SourceIndex);
	if (!SourceSlot)
		return false;

	const int32 MaxStackSize = DestinationInventory->GetMaxStackSizeForID(SourceSlot->Item);

	if (DestinationInventory->Content.ReceiveSlotAtIndex(*SourceSlot, DestinationIndex, MaxStackSize, MaxAmount))
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
	if (SourceSlotPtr == nullptr)
		return false;

	if (SourceSlotPtr->IsEmpty())
		return false;

	if (!SourceSlotPtr->Modifiers.IsEmpty())
	{
		int32 FirstEmpty = Destination->Content.GetFirstEmptySlotIndex();
		if (FirstEmpty < 0)
			return false;
		
		return DropSlotTowardOtherInventoryAtIndex(SourceIndex, Destination, FirstEmpty, SourceSlotPtr->Quantity);
	}

	TMap<FName, int32> Modifications;
	Modifications.Add(SourceSlotPtr->Item, SourceSlotPtr->Quantity);

	TMap<FName, int32> Overflows;
	if (!Destination->ModifyContent(Modifications, Overflows))
		return false;

	if (Overflows.IsEmpty())
	{
		ClearSlotAtIndex(SourceIndex);
		return true;
	}

	checkf(Overflows.Contains(SourceSlotPtr->Item), TEXT("Overflow is not empty but does not contains SourceSlotPtr->Item"));
	const int32 NewQuantity = Overflows[SourceSlotPtr->Item];

	if (SourceSlotPtr->Quantity == NewQuantity)
		return false;

	SourceSlotPtr->Quantity = NewQuantity;
	return SetSlotValueAtIndex(SourceIndex, *SourceSlotPtr);
}

void USlotInventoryComponentBase::RegroupSimilarItemsAtIndex(int32 Index)
{
	TSet<int32> ModifiedSlots;
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, nullptr);

	FInventorySlot* Slot = Content.GetSlotPtrAtIndex(Index);
	if (!Slot) return;

	const int32 MaxStackSize = GetMaxStackSizeForID(Slot->Item);

	Content.RegroupSimilarItemsAtIndex(Index, ModificationResult, MaxStackSize, Slot);

	for (const int32 ModifiedSlotIndex : ModifiedSlots)
		MarkDirtySlot(ModifiedSlotIndex);
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
