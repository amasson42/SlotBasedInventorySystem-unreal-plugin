// Amasson

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryHolderInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInventoryHolderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SLOTBASEDINVENTORYSYSTEM_API IInventoryHolderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "InventoryHolderInterface")
	class USlotInventoryComponent* GetInventoryComponent(FName InventoryTag);

};
