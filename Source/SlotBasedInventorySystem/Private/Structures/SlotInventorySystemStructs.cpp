// Amasson


#include "Structures/SlotInventorySystemStructs.h"
#include "Math/UnrealMathUtility.h"
#include "Templates/UnrealTemplate.h"


bool FInventorySlot::IsEmpty() const
{
    return (Item == NAME_None || Quantity == 0) && !HasModifiers();
}

bool FInventorySlot::HasModifiers() const
{
    return !Modifiers.IsEmpty();
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

    if (HasModifiers())
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

    if (!SourceSlot.HasModifiers())
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
        && !SourceSlot.IsEmpty())
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

bool FInventoryContent::ReceiveStacks(FItemStacks& Stacks, const FInventoryContentTransactionRule& Rule, const TMap<FName, int32>& MaxStackSizes, FContentModifications& OutModifications)
{
    bool bModified = false;
    bool bHasItemsLeft = false;
    bool bHasNewEmptySlots = false;

    TSet<FName> EmptyStacks;

    for (auto& [Item, Quantity] : Stacks)
    {
        const int32 MaxStackSize = MaxStackSizes[Item];

        OutModifications.bCreatedEmptySlot = false;

        FInventorySlotTransactionRule ReceivingRule;
        ReceivingRule.bAllowSwap = false;
        if (Rule.bPreferMerge)
        {
            ReceivingRule.bOnlyMerge = true;
            bModified |= ReceiveStack(Item, Quantity, ReceivingRule, MaxStackSize, OutModifications);
        }
        ReceivingRule.bOnlyMerge = false;
        bModified |= ReceiveStack(Item, Quantity, ReceivingRule, MaxStackSize, OutModifications);

        bHasNewEmptySlots = OutModifications.bCreatedEmptySlot;

        if (Quantity != 0)
            bHasItemsLeft = true;
        else
            EmptyStacks.Add(Item);
    }

    for (const auto& Item : EmptyStacks)
        Stacks.Remove(Item);

    if (bModified && bHasItemsLeft && bHasNewEmptySlots)
        ReceiveStacks(Stacks, Rule, MaxStackSizes, OutModifications);

    return bModified;
}

bool FInventoryContent::ReceiveStack(const FName& Item, int32& InoutQuantity, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize, FContentModifications& OutModifications)
{
    bool bModified = false;

    for (int32 i = 0; i < Slots.Num() && InoutQuantity != 0; i++)
    {
        FInventorySlot& Slot(Slots[i]);

        if (Slot.ReceiveStack(Item, InoutQuantity, Rule, MaxStackSize))
        {
            bModified = true;
            if (Slot.IsEmpty())
                OutModifications.bCreatedEmptySlot = true;
            OutModifications.ModifiedSlots.Add(i);
        }
    }
    return bModified;
}

bool FInventoryContent::ReceiveSlotAtIndex(FInventorySlot& InoutSlot, int32 Index, const FInventorySlotTransactionRule& Rule, int32 MaxStackSize)
{
    if (FInventorySlot* LocalSlot = GetSlotPtrAtIndex(Index))
        return LocalSlot->ReceiveSlot(InoutSlot, Rule, MaxStackSize);
    return false;
}

bool FInventoryContent::ReceiveSlot(FInventorySlot& InoutSlot, const FInventoryContentTransactionRule& Rule, int32 MaxStackSize, FContentModifications& OutModifications)
{
    bool bModified = false;

    FInventorySlotTransactionRule SlotRule;
    SlotRule.bAllowSwap = false;
    if (Rule.bPreferMerge)
    {
        SlotRule.bOnlyMerge = true;
        for (int32 i = 0; i < Slots.Num() && !InoutSlot.IsEmpty(); i++)
        {
            if (Slots[i].ReceiveSlot(InoutSlot, SlotRule, MaxStackSize))
            {
                bModified = true;
                OutModifications.ModifiedSlots.Add(i);
            }
        }
        if (InoutSlot.IsEmpty())
            return bModified;
    }
    SlotRule.bOnlyMerge = false;
    for (int32 i = 0; i < Slots.Num() && !InoutSlot.IsEmpty(); i++)
    {
        if (Slots[i].ReceiveSlot(InoutSlot, SlotRule, MaxStackSize))
        {
            bModified = true;
            OutModifications.ModifiedSlots.Add(i);
        }
    }
    return bModified;
}

bool FInventoryContent::RegroupSimilarItemsAtIndex(int32 Index, FContentModifications& OutModifications, int32 MaxStackSize)
{
    bool bModified = false;

    FInventorySlot* TargetSlot = GetSlotPtrAtIndex(Index);

    if (TargetSlot == nullptr || TargetSlot->IsEmpty() || TargetSlot->HasModifiers())
        return false;

    FInventorySlotTransactionRule GroupingRule;
    GroupingRule.bAllowSwap = false;
    GroupingRule.bOnlyMerge = true;
    for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && TargetSlot->Quantity < MaxStackSize; SlotIndex++)
    {
        if (SlotIndex == Index)
            continue;

        FInventorySlot& Slot = Slots[SlotIndex];

        if (TargetSlot->ReceiveSlot(Slot, GroupingRule, MaxStackSize))
        {
            bModified = true;
            OutModifications.ModifiedSlots.Add(SlotIndex);
            if (Slot.IsEmpty())
                OutModifications.bCreatedEmptySlot = true;
        }
    }
    if (bModified)
        OutModifications.ModifiedSlots.Add(Index);
    return bModified;
}
