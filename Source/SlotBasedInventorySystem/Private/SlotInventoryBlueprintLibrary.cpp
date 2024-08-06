// Amasson


#include "SlotInventoryBlueprintLibrary.h"
#include "Components/SlotInventoryComponent.h"
#include "Interfaces/InventoryHolderInterface.h"
#include "Structures/SlotModifier.h"


bool USlotInventoryBlueprintLibrary::IsValidIndex(const FInventoryContent& Content, int32 Index)
{
    return Content.IsValidIndex(Index);
}

bool USlotInventoryBlueprintLibrary::IsEmptySlot(const FInventorySlot& Slot)
{
    return Slot.IsEmpty();
}

bool USlotInventoryBlueprintLibrary::IsSlotAcceptingStackAdditions(const FInventorySlot& Slot)
{
    return Slot.AcceptStackAdditions();
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
