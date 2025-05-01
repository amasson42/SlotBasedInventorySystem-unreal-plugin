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

bool FInventorySlot::ModifyQuantity(int32& InoutQuantity, bool bAllOrNothing, int32 MaxStackSize)
{
    const int32 NewQuantity = Quantity + InoutQuantity;
    int32 TransferQuantity;

    if (NewQuantity < 0)
        TransferQuantity = -Quantity;
    else if (NewQuantity > MaxStackSize)
        TransferQuantity = MaxStackSize - Quantity;
    else
        TransferQuantity = InoutQuantity;

    if (bAllOrNothing && TransferQuantity != InoutQuantity)
        return false;

    if (TransferQuantity == 0)
        return false;

    Quantity += TransferQuantity;
    InoutQuantity -= TransferQuantity;

    return true;
}

bool FInventorySlot::ReceiveStack(const FName& InItem, int32& InoutQuantity, bool bAllOrNothing, int32 MaxStackSize)
{
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

    return ModifyQuantity(InoutQuantity, bAllOrNothing, MaxStackSize);
}

bool FInventorySlot::ReceiveSlot(FInventorySlot& SourceSlot, int32 MaxTransferAmount, int32 MaxStackSize)
{
    if (this == &SourceSlot)
        return false;

    if (IsEmpty())
        Item = SourceSlot.Item;

    if (Item != SourceSlot.Item || !Modifiers.IsEmpty() || !SourceSlot.Modifiers.IsEmpty())
        return false;

    const int32 TotalQuantity = Quantity + SourceSlot.Quantity;
    const int32 NewQuantity = FMath::Min(TotalQuantity, MaxStackSize);

    const int32 TotalTransferAmount = NewQuantity - Quantity;
    const int32 TransferAmount = FMath::Min(TotalTransferAmount, MaxTransferAmount);

    if (TransferAmount <= 0)
        return false;

    SourceSlot.Quantity -= TransferAmount;
    if (SourceSlot.Quantity == 0)
        SourceSlot.Reset();
    Quantity += TransferAmount;
    return true;
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

        if (Slot.ReceiveStack(Item, InoutQuantity, false, MaxStackSize))
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

    if (LocalSlot->ReceiveSlot(InoutSlot, MaxTransferAmount, MaxStackSize))
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

            bool bMerged = TargetSlot->ReceiveSlot(Slot, Slot.Quantity, MaxStackSize);
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
