// Amasson


#include "SlotInventoryBlueprintLibrary.h"

#include "JsonObjectConverter.h"
#include "Components/SlotInventoryComponentBase.h"
#include "Interfaces/InventoryHolderInterface.h"


bool USlotInventoryBlueprintLibrary::IsEmptySlot(const FInventorySlot& Slot)
{
    return Slot.IsEmpty();
}

bool USlotInventoryBlueprintLibrary::SlotHasModifier(const FInventorySlot& Slot, FName Type)
{
    return Slot.GetConstModifierByType(Type) != nullptr;
}

void USlotInventoryBlueprintLibrary::ModifierToString(const FItemModifier& Modifier, FString& OutString)
{
    OutString = "";
    OutString += Modifier.Type.ToString();

    if (Modifier.Data.IsValid())
    {
        const UScriptStruct* ScriptStruct = Modifier.Data.GetScriptStruct();
        const void* StructData = Modifier.Data.GetMemory();

        OutString += " (" + ScriptStruct->GetName() + ")";

        if (!ScriptStruct || !StructData)
            return;

        FString JsonString;
        auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
        if (FJsonObjectConverter::UStructToJsonObjectString(ScriptStruct, StructData, JsonString, 0, 0))
            OutString += JsonString;
    }
}

FItemModifier& USlotInventoryBlueprintLibrary::SlotGetOrMakeModifier(FInventorySlot& Slot, FName Type, const FInstancedStruct& Value, bool& bAdded)
{
    if (FItemModifier* Modifier = Slot.GetModifierByType(Type))
    {
        bAdded = false;
        return *Modifier;
    }
    bAdded = true;
    return Slot.Modifiers.Emplace_GetRef(Type, Value);
}


/** Inventory Content */

bool USlotInventoryBlueprintLibrary::IsValidIndex(const FInventoryContent& Content, int32 Index)
{
    return Content.IsValidIndex(Index);
}

bool USlotInventoryBlueprintLibrary::IsEmptySlotAtIndex(const FInventoryContent& Content, int32 Index)
{
    if (const FInventorySlot* SlotPtr = Content.GetSlotConstPtrAtIndex(Index))
        return SlotPtr->IsEmpty();

    return false;
}

int32 USlotInventoryBlueprintLibrary::GetEmptySlotCounts(const FInventoryContent& Content)
{
    int32 Total = 0;

    for (const FInventorySlot& Slot : Content.Slots)
    {
        if (Slot.IsEmpty())
            ++Total;
    }
    return Total;
}

bool USlotInventoryBlueprintLibrary::ContainsOnlyEmptySlots(const FInventoryContent& Content)
{
    for (const FInventorySlot& Slot : Content.Slots)
    {
        if (!Slot.IsEmpty())
            return false;
    }
    return true;
}

int32 USlotInventoryBlueprintLibrary::GetItemQuantity(const FInventoryContent& Content, FName Item)
{
    int32 Total = 0;

    for (const FInventorySlot& Slot : Content.Slots)
    {
        if (Slot.IsEmpty()) continue;

        if (Slot.Item == Item)
            Total += Slot.Quantity;
    }
    return Total;
}


/** Inventory Component */

USlotInventoryComponentBase* USlotInventoryBlueprintLibrary::GetInventoryComponent(UObject* Holder, FName InventoryTag)
{
    if (Holder->GetClass()->ImplementsInterface(UInventoryHolderInterface::StaticClass()))
        return IInventoryHolderInterface::Execute_GetInventoryComponent(Holder, InventoryTag);

    if (AActor* Actor = Cast<AActor>(Holder); InventoryTag == NAME_None)
        return Actor->FindComponentByClass<USlotInventoryComponentBase>();

    return nullptr;
}
