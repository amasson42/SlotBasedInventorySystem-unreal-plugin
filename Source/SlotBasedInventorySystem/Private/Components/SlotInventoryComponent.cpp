// Amasson


#include "Components/SlotInventoryComponent.h"
#include "Net/UnrealNetwork.h"


USlotInventoryComponent::USlotInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    bHasAuthority = GetOwner() ? GetOwner()->HasAuthority() : false;
}

void USlotInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bHasAuthority)
    {
        OnInventoryCapacityChanged.AddDynamic(this, &ThisClass::OnCapacityChanged);
    }
}

void USlotInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);


}

void USlotInventoryComponent::Server_BroadcastFullInventory_Implementation(bool bOwnerOnly)
{
    TArray<int32> AllIndices;
    AllIndices.Reserve(GetContentCapacity() + 1);  // Reserve space for N+1 elements
    for (int32 i = 0; i <= GetContentCapacity(); i++)
        AllIndices.Add(i);

    if (bOwnerOnly)
        Client_UpdateSlotsValues_Implementation(AllIndices, Content.Slots);
    else
        NetMulticast_UpdateSlotsValues_Implementation(AllIndices, Content.Slots);
}


/** Client Request */

void USlotInventoryComponent::Server_RequestSetContentCapacity_Implementation(int32 NewCapacity)
{
    SetContentCapacity(NewCapacity);
}

void USlotInventoryComponent::Server_RequestSetSlotValueAtIndex_Implementation(int32 Index, const FInventorySlot& NewSlotValue)
{
    SetSlotValueAtIndex(Index, NewSlotValue);
}

void USlotInventoryComponent::Server_RequestClearSlotAtIndex_Implementation(int32 Index)
{
    ClearSlotAtIndex(Index);
}

void USlotInventoryComponent::Server_RequestDropSlotTowardOtherInventoryAtIndex_Implementation(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory, int32 DestinationIndex, int32 MaxAmount)
{
    DropSlotTowardOtherInventoryAtIndex(SourceIndex, DestinationInventory, DestinationIndex, MaxAmount);
}

void USlotInventoryComponent::Server_RequestDropSlotTowardOtherInventory_Implementation(int32 SourceIndex, USlotInventoryComponentBase* DestinationInventory)
{
    DropSlotTowardOtherInventory(SourceIndex, DestinationInventory);
}

void USlotInventoryComponent::Server_RequestDropSlotFromOtherInventoryAtIndex_Implementation(int32 DestinationIndex, USlotInventoryComponentBase* SourceInventory, int32 SourceIndex, int32 MaxAmount)
{
    if (IsValid(SourceInventory))
    {
        SourceInventory->DropSlotTowardOtherInventoryAtIndex(SourceIndex, this, DestinationIndex, MaxAmount);
    }
}

void USlotInventoryComponent::Server_RequestDropSlotFromOtherInventory_Implementation(USlotInventoryComponentBase* SourceInventory, int32 SourceIndex)
{
    if (IsValid(SourceInventory))
    {
        SourceInventory->DropSlotTowardOtherInventory(SourceIndex, this);
    }
}

static AActor* GetLastValidOwner(AActor* Actor)
{
    if (IsValid(Actor))
    {
        AActor* Owner = Actor->GetOwner();
        AActor* ValidOwner = GetLastValidOwner(Owner);
        return IsValid(ValidOwner) ? ValidOwner : Actor;
    }
    return nullptr;
}

static bool IsValidAndCanCallRPC(UActorComponent* Component)
{
    if (IsValid(Component))
    {
        AActor* LastOwner = GetLastValidOwner(Component->GetOwner());
        if (IsValid(LastOwner))
        {
            ENetRole NetRole = LastOwner->GetLocalRole();
            bool bCanCallRPC = NetRole >= ENetRole::ROLE_AutonomousProxy;
            return bCanCallRPC;
        }
    }
    return false;
}

void USlotInventoryComponent::DropInventorySlotFromSourceToDestinationAtIndex(USlotInventoryComponent* SourceInventory, int32 SourceIndex, USlotInventoryComponent* DestinationInventory, int32 DestinationIndex, int32 MaxAmount)
{
    if (IsValidAndCanCallRPC(SourceInventory))
    {
        SourceInventory->Server_RequestDropSlotTowardOtherInventoryAtIndex(SourceIndex, DestinationInventory, DestinationIndex, MaxAmount);
    }
    else if (IsValidAndCanCallRPC(DestinationInventory))
    {
        DestinationInventory->Server_RequestDropSlotFromOtherInventoryAtIndex(DestinationIndex, SourceInventory, SourceIndex, MaxAmount);
    }
}

void USlotInventoryComponent::DropInventorySlotFromSourceToDestination(USlotInventoryComponent* SourceInventory, int32 SourceIndex, USlotInventoryComponent* DestinationInventory)
{
    if (IsValidAndCanCallRPC(SourceInventory))
    {
        SourceInventory->Server_RequestDropSlotTowardOtherInventory(SourceIndex, DestinationInventory);
    }
    else if (IsValidAndCanCallRPC(DestinationInventory))
    {
        DestinationInventory->Server_RequestDropSlotFromOtherInventory(SourceInventory, SourceIndex);
    }
}

void USlotInventoryComponent::Server_RequestRegroupSlotAtIndexWithSimilarIds_Implementation(int32 Index)
{
    RegroupSlotAtIndexWithSimilarIds(Index);
}


/** Slot Update */

void USlotInventoryComponent::NetMulticast_UpdateSlotsValues_Implementation(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values)
{
    ReceievedUpdateSlotsValues(Indices, Values);
}

void USlotInventoryComponent::Client_UpdateSlotsValues_Implementation(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values)
{
    ReceievedUpdateSlotsValues(Indices, Values);
}

void USlotInventoryComponent::ReceievedUpdateSlotsValues(const TArray<int32>& Indices, const TArray<FInventorySlot>& Values)
{
    checkf(Indices.Num() == Values.Num(), TEXT("SlotInventoryComponent_Networked::ReceievedUpdateSlotsValues: Received miss matching arrays"));

    if (bHasAuthority)
        return;

    for (int32 i = 0; i < Indices.Num(); i++)
    {
        SetSlotValueAtIndex(Indices[i], Values[i]);
    }
}


/** Capacity Update */

void USlotInventoryComponent::OnCapacityChanged(USlotInventoryComponentBase* SlotInventoryComponent, int32 NewCapacity)
{
    if (SlotInventoryComponent == this)
    {
        NetMulticast_UpdateCapacity(NewCapacity);
    }
}

void USlotInventoryComponent::NetMulticast_UpdateCapacity_Implementation(int32 NewCapacity)
{
    if (bHasAuthority)
        return;

    SetContentCapacity(NewCapacity);
}


/** Content Update */

void USlotInventoryComponent::BroadcastContentUpdate()
{
    if (bHasAuthority)
        BroadcastModifiedSlotsToClients();

    Super::BroadcastContentUpdate();
}

void USlotInventoryComponent::BroadcastModifiedSlotsToClients()
{
    TArray<int32> Indices;
    TArray<FInventorySlot> Values;

    for (int32 DirtySlotIndex : DirtySlots)
    {
        FInventorySlot SlotValue;
        if (GetSlotValueAtIndex(DirtySlotIndex, SlotValue))
        {
            Indices.Add(DirtySlotIndex);
            Values.Add(SlotValue);
        }
    }
    NetMulticast_UpdateSlotsValues(Indices, Values);
}
