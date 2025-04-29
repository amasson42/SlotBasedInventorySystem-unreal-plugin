// Amasson

#pragma once

#include "CoreMinimal.h"
#include "Components/SlotInventoryComponentBase.h"
#include "SlotInventoryComponent.generated.h"

/**
 * 
 */
UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SLOTBASEDINVENTORYSYSTEM_API USlotInventoryComponent : public USlotInventoryComponentBase
{
	GENERATED_BODY()

public:

	USlotInventoryComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Update")
	void Server_BroadcastFullInventory(bool bOwnerOnly = true);


	/** Client Request */

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Content|Capacity")
	void Server_RequestSetContentCapacity(int32 NewCapacity);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Content|Slot")
	void Server_RequestSetSlotValueAtIndex(int32 Index, const FInventorySlot& NewSlotValue);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Content|Slot")
	void Server_RequestClearSlotAtIndex(int32 Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Action|Drop")
	void Server_RequestDropSlotTowardOtherInventoryAtIndex(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory, int32 DestinationIndex, int32 MaxAmount = 255);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Action|Drop")
	void Server_RequestDropSlotTowardOtherInventory(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Action|Drop")
	void Server_RequestDropSlotFromOtherInventoryAtIndex(int32 DestinationIndex, USlotInventoryComponentBase* SourceInventory, int32 SourceIndex, int32 MaxAmount = 255);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Action|Drop")
	void Server_RequestDropSlotFromOtherInventory(USlotInventoryComponentBase* SourceInventory, int32 SourceIndex);

	UFUNCTION(BlueprintCallable, Category = "ClientRequest|Action|Drop")
	static void DropInventorySlotFromSourceToDestinationAtIndex(USlotInventoryComponent* SourceInventory, int32 SourceIndex, USlotInventoryComponent* DestinationInventory, int32 DestinationIndex, int32 MaxAmount = 255);

	UFUNCTION(BlueprintCallable, Category = "ClientRequest|Action|Drop")
	static void DropInventorySlotFromSourceToDestination(USlotInventoryComponent* SourceInventory, int32 SourceIndex, USlotInventoryComponent* DestinationInventory);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ClientRequest|Action")
	void Server_RequestRegroupSlotAtIndexWithSimilarIds(int32 Index);


protected:


	/** Slot Update */

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_UpdateSlotsValues(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values);

	UFUNCTION(Client, Reliable)
	void Client_UpdateSlotsValues(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values);

	void ReceievedUpdateSlotsValues(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values);


	/** Capacity Update */

	UFUNCTION()
	void OnCapacityChanged(USlotInventoryComponentBase* SlotInventoryComponent, int32 NewCapacity);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_UpdateCapacity(int32 NewCapacity);


	/** Content Update */

	virtual void BroadcastContentUpdate() override;

	void BroadcastModifiedSlotsToClients();

	bool bHasAuthority;

};
