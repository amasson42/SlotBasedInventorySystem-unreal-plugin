// Amasson


#include "Structures/SlotInventorySystemStructs.h"
#include "Math/UnrealMathUtility.h"
#include "Templates/UnrealTemplate.h"


bool FInventorySlot::IsEmpty() const
{
    return (Item == NAME_None || Quantity == 0) && Modifiers.IsEmpty();
}

void FInventorySlot::Reset()
{
    Item = NAME_None;
    Quantity = 0;
    Modifiers.Reset();
}

bool FInventorySlot::ReceiveStack(const FName& InItem, int32& InoutQuantity, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize)
{
    if (Rule.bOnlyMerge && (Item != InItem || IsEmpty()))
        return false;

    if (Item != InItem)
    {
        if (IsEmpty())
        {
            Reset();
            Item = InItem;
        }
        else
            return false;
    }

    if (!Modifiers.IsEmpty())
        return false;

    const int32 TransferQuantityGoal = Rule.MaxTransferQuantity > 0 ? FMath::Min(Rule.MaxTransferQuantity, InoutQuantity) : InoutQuantity;
    const int32 NewQuantity = Quantity + TransferQuantityGoal;
    int32 TransferQuantity;

    if (NewQuantity < 0)
        TransferQuantity = -Quantity;
    else if (NewQuantity > MaxStackSize)
        TransferQuantity = MaxStackSize - Quantity;
    else
        TransferQuantity = TransferQuantityGoal;

    if (Rule.bAtomic && TransferQuantity != TransferQuantityGoal)
        return false;

    if (TransferQuantity == 0)
        return false;

    InoutQuantity -= TransferQuantity;
    Quantity += TransferQuantity;
    check(Quantity >= 0);

    if (Quantity == 0)
        Reset();

    return true;
}

bool FInventorySlot::ReceiveSlot(FInventorySlot& SourceSlot, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize)
{
    if (this == &SourceSlot)
        return false;

    if (SourceSlot.Modifiers.IsEmpty())
    {
        if (ReceiveStack(SourceSlot.Item, SourceSlot.Quantity, Rule, MaxStackSize))
        {
            if (SourceSlot.Quantity == 0)
                SourceSlot.Reset();
            return true;
        }
    }

    if (Rule.bAllowSwap
        && SourceSlot.Quantity < MaxStackSize
        && !IsEmpty() && !SourceSlot.IsEmpty())
    {
        Swap(*this, SourceSlot);
        return true;
    }

    return false;
}

const FItemModifier* FInventorySlot::GetConstModifierByType(const FName& ModifierType) const
{
    for (const FItemModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            return &Modifier;
    }
    return nullptr;
}

FItemModifier* FInventorySlot::GetModifierByType(const FName& ModifierType)
{
    for (FItemModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            return &Modifier;
    }
    return nullptr;
}

void FInventorySlot::GetConstModifiersByType(const FName& ModifierType, TArray<const FItemModifier*>& OutModifiers) const
{
    for (const FItemModifier& Modifier : Modifiers)
    {
        if (Modifier.Type == ModifierType)
            OutModifiers.Add(&Modifier);
    }
}

void FInventorySlot::GetModifiersByType(const FName& ModifierType, TArray<FItemModifier*>& OutModifiers)
{
    for (FItemModifier& Modifier : Modifiers)
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

FInventoryContent::FContentModificationResult::FContentModificationResult(TSet<int32>* InModifiedSlots, FItemsPack* InOverflows)
: ModifiedSlots(InModifiedSlots), Overflows(InOverflows)
{}

void FInventoryContent::ModifyContent(const FItemsPack& Items, const TMap<FName, int32>& MaxStackSizes, FContentModificationResult& ModificationResult)
{
    bool bModified = false;
    bool bHasPositiveOverflow = false;
    bool bHasNewEmptySlots = false;

    for (auto& ItemAndQuantity : Items)
    {
        const FName& Item = ItemAndQuantity.Key;
        int32 ModifyQuantity = ItemAndQuantity.Value;
        const int32 MaxStackSize = MaxStackSizes[Item];

        FContentModificationResult Result(ModificationResult.ModifiedSlots, nullptr);
        ReceiveStack(Item, ModifyQuantity, MaxStackSize, true, Result);
        ReceiveStack(Item, ModifyQuantity, MaxStackSize, false, Result);
        bModified = Result.bModifiedSomething;
        bHasNewEmptySlots = Result.bCreatedEmptySlot;

        if (ModifyQuantity != 0)
        {
            if (ModificationResult.Overflows != nullptr)
                ModificationResult.Overflows->Add(Item, ModifyQuantity);
            if (ModifyQuantity > 0)
                bHasPositiveOverflow = true;
        }
    }

    if (bModified && bHasPositiveOverflow && bHasNewEmptySlots && ModificationResult.Overflows != nullptr)
    {
        if (ModificationResult.Overflows != nullptr)
        {
            const FItemsPack NewModifications = *ModificationResult.Overflows;
            ModificationResult.Overflows->Reset();
            ModifyContent(NewModifications, MaxStackSizes, ModificationResult);
        }
    }
}

void FInventoryContent::ReceiveStack(const FName& Item, int32& InoutQuantity, int32 MaxStackSize, bool bTargetOccupiedSlots, FContentModificationResult& ModificationResult)
{
    for (int32 i = 0; i < Slots.Num() && InoutQuantity != 0; i++)
    {
        FInventorySlot& Slot(Slots[i]);

        if (bTargetOccupiedSlots && Slot.IsEmpty())
            continue;

        if (Slot.ReceiveStack(Item, InoutQuantity, FInventorySlotTransactionRule(), MaxStackSize))
        {
            ModificationResult.bModifiedSomething = true;
            if (Slot.IsEmpty())
                ModificationResult.bCreatedEmptySlot = true;
            if (ModificationResult.ModifiedSlots != nullptr)
                ModificationResult.ModifiedSlots->Add(i);
        }
    }
}

bool FInventoryContent::ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, int32 MaxStackSize, int32 MaxTransferAmount)
{
    FInventorySlot* LocalSlot = GetSlotPtrAtIndex(Index);
    if (!LocalSlot || LocalSlot == &InoutSlot)
        return false;

    FInventorySlotTransactionRule Rule;
    Rule.MaxTransferQuantity = MaxTransferAmount;
    if (LocalSlot->ReceiveSlot(InoutSlot, Rule, MaxStackSize))
        return true;

    if (!InoutSlot.IsEmpty() && LocalSlot->Item != InoutSlot.Item && LocalSlot->Quantity <= MaxStackSize)
    {
        Swap(InoutSlot, *LocalSlot);
        return true;
    }

    return false;
}

void FInventoryContent::RegroupSimilarItemsAtIndex(int32 Index, FContentModificationResult& ModificationResult, int32 MaxStackSize, FInventorySlot* CachedSlotPtr)
{
    FInventorySlot* TargetSlot = CachedSlotPtr ? CachedSlotPtr : GetSlotPtrAtIndex(Index);

    if (!TargetSlot)
        return;

    if (TargetSlot->IsEmpty())
        return;

    for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && TargetSlot->Quantity < MaxStackSize; SlotIndex++)
    {
        if (TargetSlot->Quantity >= MaxStackSize)
            break;

        if (SlotIndex == Index)
            continue;

        FInventorySlot& Slot = Slots[SlotIndex];
        
        if (!Slot.IsEmpty() && Slot.Item == TargetSlot->Item)
        {
            if (!Slot.Modifiers.IsEmpty())
                continue;

            bool bMerged = TargetSlot->ReceiveSlot(Slot, FInventorySlotTransactionRule(), MaxStackSize);
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
