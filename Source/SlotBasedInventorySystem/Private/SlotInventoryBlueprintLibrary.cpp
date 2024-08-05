// Amasson


#include "SlotInventoryBlueprintLibrary.h"
#include "Components/SlotInventoryComponent.h"
#include "Interfaces/InventoryHolderInterface.h"


bool USlotInventoryBlueprintLibrary::IsValidIndex(const FInventoryContent& Content, int32 Index)
{
    return Content.IsValidIndex(Index);
}

bool USlotInventoryBlueprintLibrary::IsEmptySlot(const FInventorySlot& Slot)
{
    return Slot.IsEmpty();
}

bool USlotInventoryBlueprintLibrary::IsSlotAcceptingStackAddition(const FInventorySlot& Slot)
{
    for (const auto& Modifier : Slot.Modifiers)
    {
        if (!Modifier->AcceptStackAddition())
            return false;
    }
    return true;
}

USlotInventoryComponent* USlotInventoryBlueprintLibrary::GetInventoryComponent(UObject* Holder, FName InventoryTag)
{
    if (Holder->GetClass()->ImplementsInterface(UInventoryHolderInterface::StaticClass()))
    {
        return IInventoryHolderInterface::Execute_GetInventoryComponent(Holder, InventoryTag);
    }
    else if (AActor* Actor = Cast<AActor>(Holder))
    {
        return Actor->FindComponentByClass<USlotInventoryComponent>();
    }
    return nullptr;
}
