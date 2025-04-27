// Amasson


#include "Structures/SlotInventorySystemStructs.h"
#include "Math/UnrealMathUtility.h"
#include "Templates/UnrealTemplate.h"


bool FInventorySlot::IsEmpty() const
{
    return (ID == NAME_None || Count == 0) && Modifiers.IsEmpty();
}

void FInventorySlot::Reset()
{
    ID = NAME_None;
    Count = 0;
    Modifiers.Reset();
}

void FInventorySlot::ModifyCountWithOverflow(int32 ModifyAmount, int32& Overflow, int32 MaxStackSize)
{
    int32 NewCount = Count + ModifyAmount;

    if (NewCount < 0)
    {
        Overflow = Count + ModifyAmount;
        Count = 0;
    }
    else if (NewCount > MaxStackSize)
    {
        Overflow = Count + ModifyAmount - MaxStackSize;
        Count = MaxStackSize;
    }
    else
    {
        Overflow = 0;
        Count = NewCount;
    }
}

bool FInventorySlot::TryModifyCountByExact(int32 ModifyAmount, int32 MaxStackSize)
{
    int32 NewCount = Count + ModifyAmount;

    if (NewCount < 0 || NewCount > MaxStackSize)
        return false;

    Count = NewCount;
    return true;
}

bool FInventorySlot::AddIdAndCount(const FName& SlotId, int32 ModifyAmount, int32& Overflow, int32 MaxStackSize)
{
    if (IsEmpty())
    {
        if (ModifyAmount < 0)
        {
            Overflow = ModifyAmount;
            return false;
        }
        ID = SlotId;
        Count = FMath::Min(ModifyAmount, int32(MaxStackSize));
        Overflow = ModifyAmount - Count;
        return Overflow != ModifyAmount;
    }

    if (ID != SlotId)
    {
        Overflow = ModifyAmount;
        return false;
    }

    ModifyCountWithOverflow(ModifyAmount, Overflow, MaxStackSize);
    return Overflow != ModifyAmount;
}

const FSlotModifier* FInventorySlot::GetConstModifierByType(const FName& ModifierType) const
{
    for (const FSlotModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            return &Modifier;
    }
    return nullptr;
}

FSlotModifier* FInventorySlot::GetModifierByType(const FName& ModifierType)
{
    for (FSlotModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            return &Modifier;
    }
    return nullptr;
}

void FInventorySlot::GetConstModifiersByType(const FName& ModifierType, TArray<const FSlotModifier*>& OutModifiers) const
{
    for (const FSlotModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            OutModifiers.Add(&Modifier);
    }
}

void FInventorySlot::GetModifiersByType(const FName& ModifierType, TArray<FSlotModifier*>& OutModifiers)
{
    for (FSlotModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            OutModifiers.Add(&Modifier);
    }
}


/** Inventory Content */


bool FInventoryContent::IsValidIndex(int32 Index) const
{
	return Index >= 0 && Index < Slots.Num();
}

FInventorySlot* FInventoryContent::GetSlotPtrAtIndex(int32 Index)
{
	if (!IsValidIndex(Index))
		return nullptr;

	return &(Slots[Index]);
}

const FInventorySlot* FInventoryContent::GetSlotConstPtrAtIndex(int32 Index) const
{
	if (!IsValidIndex(Index))
		return nullptr;

	return &(Slots[Index]);
}

FInventoryContent::FContentModificationResult::FContentModificationResult(TSet<int32>* InModifiedSlots, TMap<FName, int32>* InOverflows)
: bModifiedSomething(false), bCreatedEmptySlot(false),
    ModifiedSlots(InModifiedSlots), Overflows(InOverflows)
{}

void FInventoryContent::ModifyContentWithValues(const TMap<FName, int32>& IdsAndCounts, const TMap<FName, int32>& MaxStackSizes, FContentModificationResult& ModificationResult)
{
    bool bModified = false;
    bool bHasPositiveOverflow = false;
    bool bHasNewEmptySlots = false;

    for (const TPair<FName, int32>& IdAndCount : IdsAndCounts)
    {
        const FName& SlotId = IdAndCount.Key;
        int32 ModifyAmount = IdAndCount.Value;
        const int32 MaxStackSize = MaxStackSizes[SlotId];

        FContentModificationResult Result(ModificationResult.ModifiedSlots, nullptr);
        ReceiveSlotOverflow(SlotId, ModifyAmount, MaxStackSize, false, Result);
        ReceiveSlotOverflow(SlotId, ModifyAmount, MaxStackSize, true, Result);
        bModified = Result.bModifiedSomething;
        bHasNewEmptySlots = Result.bCreatedEmptySlot;

        if (ModifyAmount != 0)
        {
            if (ModificationResult.Overflows)
                ModificationResult.Overflows->Add(SlotId, ModifyAmount);
            if (ModifyAmount > 0)
                bHasPositiveOverflow = true;
        }
    }

    if (bModified && bHasPositiveOverflow && bHasNewEmptySlots && ModificationResult.Overflows)
    {
        if (ModificationResult.Overflows)
        {
            const TMap<FName, int32> NewModifications = *ModificationResult.Overflows;
            ModificationResult.Overflows->Reset();
            ModifyContentWithValues(NewModifications, MaxStackSizes, ModificationResult);
        }
    }
}

void FInventoryContent::ReceiveSlotOverflow(const FName& SlotId, int32& InoutOverflow, int32 MaxStackSize, bool bTargetEmptySlots, FContentModificationResult& ModificationResult)
{
    for (int32 i = 0; i < Slots.Num() && InoutOverflow != 0; i++)
    {
        FInventorySlot& Slot(Slots[i]);

        if (Slot.IsEmpty() != bTargetEmptySlots)
            continue;

        if (!Slot.Modifiers.IsEmpty())
            continue;

        if (Slot.AddIdAndCount(SlotId, InoutOverflow, InoutOverflow, MaxStackSize))
        {
            ModificationResult.bModifiedSomething = true;
            if (Slot.IsEmpty())
                ModificationResult.bCreatedEmptySlot = true;
            if (ModificationResult.ModifiedSlots)
                ModificationResult.ModifiedSlots->Add(i);
        }
    }
}

bool FInventoryContent::ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, int32 MaxStackSize, int32 MaxTransferAmount)
{
    FInventorySlot* LocalSlot = GetSlotPtrAtIndex(Index);
    if (!LocalSlot)
        return false;

    if (LocalSlot->IsEmpty())
    {
        if (InoutSlot.IsEmpty())
            return false;

        LocalSlot->ID = InoutSlot.ID;
        LocalSlot->Count = 0;
    }

    const bool bMergeable = LocalSlot->ID == InoutSlot.ID
        && LocalSlot->Modifiers.IsEmpty()
        && InoutSlot.Modifiers.IsEmpty();
    if (bMergeable)
        return MergeSlotsWithSimilarIds(*LocalSlot, InoutSlot, MaxStackSize, MaxTransferAmount);

    bool bCanReceiveSlot = InoutSlot.Count <= MaxTransferAmount && InoutSlot.Count <= MaxStackSize;
    if (!bCanReceiveSlot)
        return false;

    SwapSlots(InoutSlot, *LocalSlot);

    return true;
}

void FInventoryContent::RegroupSlotsWithSimilarIdsAtIndex(int32 Index, FContentModificationResult& ModificationResult, int32 MaxStackSize, FInventorySlot* CachedSlotPtr)
{
    FInventorySlot* TargetSlot = CachedSlotPtr ? CachedSlotPtr : GetSlotPtrAtIndex(Index);

    if (!TargetSlot)
        return;

    if (TargetSlot->IsEmpty())
        return;

    for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && TargetSlot->Count < MaxStackSize; SlotIndex++)
    {
        if (TargetSlot->Count >= MaxStackSize)
            break;

        if (SlotIndex == Index)
            continue;

        FInventorySlot& Slot = Slots[SlotIndex];
        
        if (!Slot.IsEmpty() && Slot.ID == TargetSlot->ID)
        {
            if (!Slot.Modifiers.IsEmpty())
                continue;

            bool bMerged = MergeSlotsWithSimilarIds(*TargetSlot, Slot, MaxStackSize);
            if (bMerged)
            {
                ModificationResult.bModifiedSomething = true;
                if (ModificationResult.ModifiedSlots)
                    ModificationResult.ModifiedSlots->Add(SlotIndex);
                if (Slot.IsEmpty())
                    ModificationResult.bCreatedEmptySlot = true;
            }
        }
    }
    if (ModificationResult.bModifiedSomething)
        if (ModificationResult.ModifiedSlots)
            ModificationResult.ModifiedSlots->Add(Index);
}

bool FInventoryContent::MergeSlotsWithSimilarIds(FInventorySlot& DestinationSlot, FInventorySlot& SourceSlot, int32 MaxStackSize, int32 MaxTransferAmount)
{
    checkf(SourceSlot.ID == DestinationSlot.ID, TEXT("Tried to merge slots with different Ids (%s!=%s)"), *SourceSlot.ID.ToString(), *DestinationSlot.ID.ToString());

    const int32 LocalCount = DestinationSlot.Count;
    const int32 ReceiveCount = SourceSlot.Count;

    const int32 TotalCount = LocalCount + ReceiveCount;
    const int32 StackCount = FMath::Min(TotalCount, int32(MaxStackSize));

    const int32 TotalTransferAmount = StackCount - LocalCount;
    const int32 TransferAmount = FMath::Min(TotalTransferAmount, int32(MaxTransferAmount));

    if (TransferAmount == 0)
        return false;

    SourceSlot.Count -= TransferAmount;
    if (SourceSlot.Count == 0)
		SourceSlot.Reset();
    DestinationSlot.Count += TransferAmount;
    return true;
}

void FInventoryContent::SwapSlots(FInventorySlot& FirstSlot, FInventorySlot& SecondSlot)
{
    Swap(FirstSlot, SecondSlot);
}

int32 FInventoryContent::GetFirstEmptySlotIndex() const
{
    for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].IsEmpty())
			return i;
	}
	return -1;
}
