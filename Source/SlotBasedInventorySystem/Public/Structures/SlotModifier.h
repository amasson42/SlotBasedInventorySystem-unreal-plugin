// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SlotModifier.generated.h"

/**
 * Data structure that holds the information of a modifier that can be applied to a slot.
 * This is an abstract class, and should be inherited to create a modifier.
 * 
 * All properties should also add the SaveGame flag, so that they can be saved to disk.
 */
// UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories, DisplayName="Slot Modifier")
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class SLOTBASEDINVENTORYSYSTEM_API USlotModifier : public UObject
{
	GENERATED_BODY()
	

public:

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Slot Modifier")
    bool AcceptStackAddition() const;
    virtual bool AcceptStackAddition_Implementation() const;
};
