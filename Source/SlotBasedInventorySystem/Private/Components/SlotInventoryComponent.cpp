// Amasson


#include "Components/SlotInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

USlotInventoryComponent::USlotInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetComponentTickEnabled(false);
}


/** Public Content Management */

const FInventoryContent& USlotInventoryComponent::GetContent() const
{
	return Content;
}

void USlotInventoryComponent::SetContent(const FInventoryContent& NewContent)
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

int32 USlotInventoryComponent::GetContentCapacity() const
{
	return Content.Slots.Num();
}

void USlotInventoryComponent::SetContentCapacity(int32 NewCapacity)
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

bool USlotInventoryComponent::GetSlotValueAtIndex(int32 Index, FInventorySlot& SlotValue) const
{
	if (const FInventorySlot* SlotPtr = Content.GetSlotConstPtrAtIndex(Index))
	{
		SlotValue = *SlotPtr;
		return true;
	}

	return false;
}

bool USlotInventoryComponent::SetSlotValueAtIndex(int32 Index, const FInventorySlot& NewSlotValue)
{
	if (FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index))
	{
		*SlotPtr = NewSlotValue;
		MarkDirtySlot(Index);
		return true;
	}

	return false;
}

bool USlotInventoryComponent::IsEmptySlotAtIndex(int32 Index) const
{
	if (const FInventorySlot* SlotPtr = Content.GetSlotConstPtrAtIndex(Index))
		return SlotPtr->IsEmpty();

	return false;
}

bool USlotInventoryComponent::ClearSlotAtIndex(int32 Index)
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

int32 USlotInventoryComponent::GetEmptySlotCounts() const
{
	int32 Total = 0;

	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (Slot.IsEmpty())
			++Total;
	}
	return Total;
}

bool USlotInventoryComponent::ContainsOnlyEmptySlots() const
{
	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (!Slot.IsEmpty())
			return false;
	}
	return true;
}

void USlotInventoryComponent::ModifySlotCountAtIndex(int32 Index, int32 ModifyAmount, bool bAllOrNothing, int32& Overflow)
{
	FInventorySlot* SlotPtr = Content.GetSlotPtrAtIndex(Index);

	if (!SlotPtr || SlotPtr->IsEmpty())
	{
		Overflow = ModifyAmount;
		return;
	}

	const uint8 MaxStackSize = GetMaxStackSizeForID(SlotPtr->ID);

	if (bAllOrNothing)
	{
		bool bModified = SlotPtr->TryModifyCountByExact(ModifyAmount, MaxStackSize);
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
		SlotPtr->ModifyCountWithOverflow(ModifyAmount, Overflow, MaxStackSize);
		if (Overflow != ModifyAmount)
			MarkDirtySlot(Index);
	}
}

uint8 USlotInventoryComponent::GetMaxStackSizeForID(const FName& ID) const
{
	return 255;
}

void USlotInventoryComponent::GetMaxStackSizeForIds(const TSet<FName>& Ids, TMap<FName, uint8>& MaxStackSizes) const
{
	for (const FName& Id : Ids)
	{
		const uint8 MaxStackSize = GetMaxStackSizeForID(Id);
		MaxStackSizes.Add(Id, MaxStackSize);
	}
}


/** Content Management */

int32 USlotInventoryComponent::GetContentIdCount(FName Id) const
{
	int32 Total = 0;

	for (const FInventorySlot& Slot : Content.Slots)
	{
		if (Slot.IsEmpty()) continue;

		if (Slot.ID == Id)
			Total += Slot.Count;
	}
	return Total;
}

bool USlotInventoryComponent::ModifyContentWithOverflow(const TMap<FName, int32>& IdsAndCounts, TMap<FName, int32>& Overflows)
{
	const TMap<FName, uint8>& MaxStackSizes = GetMaxStackSizesFromIds(IdsAndCounts);

	TSet<int32> ModifiedSlots;
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, &Overflows);
	Content.ModifyContentWithValues(IdsAndCounts, MaxStackSizes, ModificationResult);

	for (int32 ModifiedSlotIndex : ModifiedSlots)
	{
		MarkDirtySlot(ModifiedSlotIndex);
	}

	return true;
}

bool USlotInventoryComponent::TryModifyContentWithoutOverflow(const TMap<FName, int32>& IdsAndCounts)
{
	const TMap<FName, uint8>& MaxStackSizes = GetMaxStackSizesFromIds(IdsAndCounts);

	TSet<int32> ModifiedSlots;

	FInventoryContent TmpContent = Content;

	TMap<FName, int32> TmpOverflows;
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, &TmpOverflows);

	TmpContent.ModifyContentWithValues(IdsAndCounts, MaxStackSizes, ModificationResult);

	if (!TmpOverflows.IsEmpty())
		return false;

	Content = TmpContent;

	for (int32 ModifiedSlotIndex : ModifiedSlots)
		MarkDirtySlot(ModifiedSlotIndex);

	return true;
}

bool USlotInventoryComponent::DropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponent* DestinationInventory, int32 DestinationIndex, uint8 MaxAmount)
{
	if (!IsValid(DestinationInventory)) return false;

	FInventorySlot* SourceSlot = Content.GetSlotPtrAtIndex(SourceIndex);
	if (!SourceSlot)
		return false;

	const uint8 MaxStackSize = DestinationInventory->GetMaxStackSizeForID(SourceSlot->ID);

	if (DestinationInventory->Content.ReceiveSlotAtIndex(*SourceSlot, DestinationIndex, MaxStackSize, MaxAmount))
	{
		MarkDirtySlot(SourceIndex);
		DestinationInventory->MarkDirtySlot(DestinationIndex);
		return true;
	}

	return false;
}

bool USlotInventoryComponent::DropSlotTowardOtherInventory(int32 SourceIndex, USlotInventoryComponent* Destination)
{
	if (!IsValid(Destination)) return false;

	FInventorySlot SourceSlot;
	if (!GetSlotValueAtIndex(SourceIndex, SourceSlot))
		return false;

	if (SourceSlot.IsEmpty())
		return false;

	TMap<FName, int32> Modifications;
	Modifications.Add(SourceSlot.ID, SourceSlot.Count);

	TMap<FName, int32> Overflows;
	if (!Destination->ModifyContentWithOverflow(Modifications, Overflows))
		return false;

	if (Overflows.IsEmpty())
	{
		ClearSlotAtIndex(SourceIndex);
		return true;
	}

	checkf(Overflows.Contains(SourceSlot.ID), TEXT("Overflow is not empty but does not contains SourceSlot.ID"));
	const int32 NewCount = Overflows[SourceSlot.ID];

	if (SourceSlot.Count == NewCount)
		return false;

	SourceSlot.Count = NewCount;
	return SetSlotValueAtIndex(SourceIndex, SourceSlot);
}

void USlotInventoryComponent::RegroupSlotAtIndexWithSimilarIds(int32 Index)
{
	TSet<int32> ModifiedSlots;
	FInventoryContent::FContentModificationResult ModificationResult(&ModifiedSlots, nullptr);

	FInventorySlot* Slot = Content.GetSlotPtrAtIndex(Index);
	if (!Slot) return;

	const uint8 MaxStackSize = GetMaxStackSizeForID(Slot->ID);

	Content.RegroupSlotsWithSimilarIdsAtIndex(Index, ModificationResult, MaxStackSize, Slot);

	for (const int32 ModifiedSlotIndex : ModifiedSlots)
		MarkDirtySlot(ModifiedSlotIndex);
}


/** Private Content Management */

const TMap<FName, uint8> USlotInventoryComponent::GetMaxStackSizesFromIds(const TMap<FName, int32>& IdsAndCounts) const
{
	TSet<FName> Ids;
	IdsAndCounts.GetKeys(Ids);
	TMap<FName, uint8> MaxStackSizes;
	GetMaxStackSizeForIds(Ids, MaxStackSizes);
	return MaxStackSizes;
}

/** Slot Updating */

void USlotInventoryComponent::BroadcastContentUpdate()
{
	OnInventoryContentChanged.Broadcast(this, DirtySlots.Array());
	DirtySlots.Reset();
}

void USlotInventoryComponent::MarkDirtySlot(int32 SlotIndex)
{
	DirtySlots.Add(SlotIndex);
	MarkSlotsHaveBeenModified();
}

void USlotInventoryComponent::MarkSlotsHaveBeenModified()
{
	SetComponentTickEnabled(true);
}