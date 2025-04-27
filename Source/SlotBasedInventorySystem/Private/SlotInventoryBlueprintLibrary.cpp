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

USlotInventoryComponent* USlotInventoryBlueprintLibrary::GetInventoryComponent(UObject* Holder, FName InventoryTag)
{
    if (Holder->GetClass()->ImplementsInterface(UInventoryHolderInterface::StaticClass()))
        return IInventoryHolderInterface::Execute_GetInventoryComponent(Holder, InventoryTag);

    if (AActor* Actor = Cast<AActor>(Holder); InventoryTag == NAME_None)
        return Actor->FindComponentByClass<USlotInventoryComponent>();

    return nullptr;
}
