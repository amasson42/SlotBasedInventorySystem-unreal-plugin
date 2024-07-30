// Amasson


#include "SlotInventoryBlueprintLibrary.h"


bool USlotInventoryBlueprintLibrary::IsValidIndex(const FInventoryContent& Content, int32 Index)
{
    return Content.IsValidIndex(Index);
}

bool USlotInventoryBlueprintLibrary::IsEmptySlot(const FInventorySlot& Slot)
{
    return Slot.IsEmpty();
}
