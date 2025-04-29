// Amasson


#include "SlotInventoryBlueprintLibrary.h"

#include "JsonObjectConverter.h"
#include "Components/SlotInventoryComponentBase.h"
#include "Interfaces/InventoryHolderInterface.h"


bool USlotInventoryBlueprintLibrary::IsValidIndex(const FInventoryContent& Content, int32 Index)
{
    return Content.IsValidIndex(Index);
}

bool USlotInventoryBlueprintLibrary::IsEmptySlot(const FInventorySlot& Slot)
{
    return Slot.IsEmpty();
}

USlotInventoryComponentBase* USlotInventoryBlueprintLibrary::GetInventoryComponent(UObject* Holder, FName InventoryTag)
{
    if (Holder->GetClass()->ImplementsInterface(UInventoryHolderInterface::StaticClass()))
        return IInventoryHolderInterface::Execute_GetInventoryComponent(Holder, InventoryTag);

    if (AActor* Actor = Cast<AActor>(Holder); InventoryTag == NAME_None)
        return Actor->FindComponentByClass<USlotInventoryComponentBase>();

    return nullptr;
}

void USlotInventoryBlueprintLibrary::ModifierToString(const FSlotModifier& Modifier, FString& OutString)
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
