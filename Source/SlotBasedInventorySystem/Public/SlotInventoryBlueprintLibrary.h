// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Structures/SlotInventorySystemStructs.h"
#include "SlotInventoryBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SLOTBASEDINVENTORYSYSTEM_API USlotInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Inventory Content */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Content")
	static bool IsValidIndex(const FInventoryContent& Content, int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Slot")
	static bool IsEmptySlot(const FInventorySlot& Slot);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Slot")
    static bool IsSlotAcceptingStackAddition(const FInventorySlot& Slot);    

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Component")
	static class USlotInventoryComponent* GetInventoryComponent(UObject* Holder, FName InventoryTag);

};
