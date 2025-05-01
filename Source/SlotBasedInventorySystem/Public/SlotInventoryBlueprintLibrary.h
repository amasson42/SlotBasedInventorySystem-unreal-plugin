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
	static bool SlotHasModifier(const FInventorySlot& Slot, FName Type);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "SlotInventory|Slot")
	static FItemModifier& SlotGetOrMakeModifier(UPARAM(Ref) FInventorySlot& Slot, FName Type, FInstancedStruct Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Component")
	static class USlotInventoryComponentBase* GetInventoryComponent(UObject* Holder, FName InventoryTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SlotInventory|Debug")
	static void ModifierToString(const FItemModifier& Modifier, FString& OutString);

};
