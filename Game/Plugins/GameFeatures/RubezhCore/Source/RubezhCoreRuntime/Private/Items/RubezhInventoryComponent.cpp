// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Items/RubezhWorldItem.h"
#include "Movement/RubezhMovementSpeed.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "Survival/RubezhSurvivalSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryComponent)

URubezhInventoryComponent::URubezhInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

URubezhInventoryComponent* URubezhInventoryComponent::FindForActor(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		Actor = Pawn->GetPlayerState();
	}
	else if (const AController* Controller = Cast<AController>(Actor))
	{
		Actor = Controller->PlayerState;
	}
	return Actor ? Actor->FindComponentByClass<URubezhInventoryComponent>() : nullptr;
}

void URubezhInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Содержимое чужого рюкзака — тайна: видит только владелец (и тот, кто обыщет тело — позже).
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, Grid, COND_OwnerOnly);
}

void URubezhInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	const URubezhSettings* Settings = GetDefault<URubezhSettings>();
	ItemTable = Settings->ItemTable.LoadSynchronous();
	if (!ItemTable)
	{
		UE_LOG(LogRubezh, Error, TEXT("Rubezh: не задана таблица предметов (Project Settings → Game → Rubezh → Item Table)"));
	}

	if (GetOwner()->HasAuthority())
	{
		Grid = FRubezhInventoryGrid(Settings->DefaultBackpackSize);
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		CarriedWeightChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetCarriedWeightAttribute())
			.AddUObject(this, &ThisClass::OnCarriedWeightChanged);
	}
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.AddDynamic(this, &ThisClass::OnPawnSet);
	}
}

void URubezhInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetCarriedWeightAttribute()).Remove(CarriedWeightChangedHandle);
	}
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::OnPawnSet);
	}

	Super::EndPlay(EndPlayReason);
}

void URubezhInventoryComponent::OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	RefreshOwnerWalkSpeed();
}

void URubezhInventoryComponent::OnCarriedWeightChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshOwnerWalkSpeed();
}

void URubezhInventoryComponent::RefreshOwnerWalkSpeed() const
{
	if (const APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		Rubezh::Movement::RefreshWalkSpeed(Cast<ACharacter>(PlayerState->GetPawn()));
	}
}

const FRubezhItemDefinition* URubezhInventoryComponent::FindDefinition(FName ItemId) const
{
	return ItemTable ? ItemTable->FindRow<FRubezhItemDefinition>(ItemId, TEXT("RubezhInventory"), false) : nullptr;
}

int32 URubezhInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return Count;
	}

	const FRubezhItemDefinition* Definition = FindDefinition(ItemId);
	if (!Definition)
	{
		UE_LOG(LogRubezh, Warning, TEXT("Rubezh: неизвестный предмет '%s'"), *ItemId.ToString());
		return Count;
	}

	const int32 NotAdded = Grid.AddItem(ItemId, *Definition, Count, [this](FName Id) { return FindDefinition(Id); });
	if (NotAdded < Count)
	{
		HandleGridChanged();
	}
	return NotAdded;
}

int32 URubezhInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return 0;
	}

	const int32 Removed = Grid.RemoveItem(ItemId, Count);
	if (Removed > 0)
	{
		HandleGridChanged();
	}
	return Removed;
}

void URubezhInventoryComponent::ServerMoveEntry_Implementation(int32 EntryIndex, FIntPoint NewPosition, bool bRotated)
{
	// Сетка сама проверяет индекс, границы и пересечения — недопустимый запрос просто ничего не меняет.
	if (Grid.MoveEntry(EntryIndex, NewPosition, bRotated, [this](FName Id) { return FindDefinition(Id); }))
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerDropEntry_Implementation(int32 EntryIndex)
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (!Pawn || !Grid.GetEntries().IsValidIndex(EntryIndex))
	{
		return;
	}

	// Сначала кладём на землю, потом убираем из рюкзака — если положить не вышло, вещь не пропадёт.
	const FRubezhInventoryEntry Entry = Grid.GetEntries()[EntryIndex];
	if (!ARubezhWorldItem::SpawnInFrontOf(*Pawn, Entry.ItemId, Entry.Count))
	{
		return;
	}

	FRubezhInventoryEntry Removed;
	Grid.RemoveEntry(EntryIndex, Removed);
	HandleGridChanged();
}

float URubezhInventoryComponent::GetTotalWeight() const
{
	return Grid.GetTotalWeight([this](FName Id) { return FindDefinition(Id); });
}

void URubezhInventoryComponent::OnRep_Grid()
{
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::HandleGridChanged()
{
	UpdateCarriedWeight();
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::UpdateCarriedWeight() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (ASC && ASC->GetSet<URubezhSurvivalSet>())
	{
		ASC->SetNumericAttributeBase(URubezhSurvivalSet::GetCarriedWeightAttribute(), GetTotalWeight());
	}
}
