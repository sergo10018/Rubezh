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
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryComponent)

namespace
{
	constexpr float WeightSyncIntervalSeconds = 0.25f;
}

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
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, Equipped, COND_OwnerOnly);
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
		Equipped.SetNum(Rubezh::Equipment::SlotCount);
		Grid = FRubezhInventoryGrid(Settings->PocketsSize);
		ApplyStartingEquipment();
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
	GetWorld()->GetTimerManager().ClearTimer(WeightSyncTimer);

	Super::EndPlay(EndPlayReason);
}

void URubezhInventoryComponent::OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	RefreshOwnerWalkSpeed();

	// Атрибуты веса появляются на ASC чуть позже персонажа (их выдаёт набор способностей героя) —
	// ждём их и выставляем вес и лимит с учётом рюкзака.
	if (GetOwner()->HasAuthority() && NewPawn)
	{
		GetWorld()->GetTimerManager().SetTimer(WeightSyncTimer, this, &ThisClass::TrySyncWeightAttributes, WeightSyncIntervalSeconds, true);
	}
}

void URubezhInventoryComponent::TrySyncWeightAttributes()
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (ASC && ASC->GetSet<URubezhSurvivalSet>())
	{
		UpdateCarriedWeight();
		GetWorld()->GetTimerManager().ClearTimer(WeightSyncTimer);
	}
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

FIntPoint URubezhInventoryComponent::GetPocketsSize()
{
	return GetDefault<URubezhSettings>()->PocketsSize;
}

void URubezhInventoryComponent::ApplyStartingEquipment()
{
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	for (const FName& ItemId : GetDefault<URubezhSettings>()->StartingEquipment)
	{
		const FRubezhItemDefinition* Item = FindDefinition(ItemId);
		const int32 SlotIndex = Item ? Rubezh::Equipment::GetSlotIndex(Item->EquipSlot) : INDEX_NONE;
		if (SlotIndex == INDEX_NONE || !Equipped[SlotIndex].IsNone())
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh: стартовое снаряжение '%s' не надевается или слот занят"), *ItemId.ToString());
			continue;
		}
		Equipped[SlotIndex] = ItemId;
	}
	// Сетка пока пустая — просто берём размер надетого рюкзака.
	Grid = FRubezhInventoryGrid(Rubezh::Equipment::GetGridSize(Equipped, Lookup, GetPocketsSize()));
	HandleGridChanged();
}

FName URubezhInventoryComponent::GetEquipped(ERubezhEquipSlot Slot) const
{
	const int32 Index = Rubezh::Equipment::GetSlotIndex(Slot);
	return Equipped.IsValidIndex(Index) ? Equipped[Index] : NAME_None;
}

Rubezh::Equipment::FProtection URubezhInventoryComponent::GetProtection(ERubezhBodyZone Zone) const
{
	return Rubezh::Equipment::GetProtection(Equipped, Zone, [this](FName Id) { return FindDefinition(Id); });
}

float URubezhInventoryComponent::GetIncomingDamageMultiplier(const UAbilitySystemComponent* Target, const FHitResult* Hit)
{
	const URubezhInventoryComponent* Inventory = Target ? FindForActor(Target->GetOwnerActor()) : nullptr;
	if (!Inventory)
	{
		return 1.0f;
	}
	const ERubezhBodyZone Zone = Hit ? Rubezh::Equipment::GetZoneForBone(Hit->BoneName) : ERubezhBodyZone::Torso;
	return Inventory->GetProtection(Zone).DamageMultiplier;
}

void URubezhInventoryComponent::LogEquipFailure(Rubezh::Equipment::EResult Result) const
{
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		UE_LOG(LogRubezh, Verbose, TEXT("%s: снаряжение — %s"), *GetNameSafe(GetOwner()), Rubezh::Equipment::Describe(Result));
	}
}

void URubezhInventoryComponent::ServerEquipEntry_Implementation(int32 EntryIndex)
{
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Equip(Grid, Equipped, EntryIndex, [this](FName Id) { return FindDefinition(Id); }, GetPocketsSize());
	LogEquipFailure(Result);
	if (Result == Rubezh::Equipment::EResult::Ok)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerUnequip_Implementation(ERubezhEquipSlot Slot, FIntPoint Position, bool bRotated, bool bAutoPlace)
{
	const TOptional<FIntPoint> Target = bAutoPlace ? TOptional<FIntPoint>() : TOptional<FIntPoint>(Position);
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, Slot, Target, bRotated, [this](FName Id) { return FindDefinition(Id); }, GetPocketsSize());
	LogEquipFailure(Result);
	if (Result == Rubezh::Equipment::EResult::Ok)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerDropEquipped_Implementation(ERubezhEquipSlot Slot)
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	// Снимаем на копиях; на землю кладём до того, как сохранить, — если положить не вышло, вещь останется надетой.
	FRubezhInventoryGrid NewGrid = Grid;
	Rubezh::Equipment::FEquipped NewEquipped = Equipped;
	FName ItemId;
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::TakeOff(NewGrid, NewEquipped, Slot, [this](FName Id) { return FindDefinition(Id); }, GetPocketsSize(), ItemId);
	LogEquipFailure(Result);
	if (Result != Rubezh::Equipment::EResult::Ok || !ARubezhWorldItem::SpawnInFrontOf(*Pawn, ItemId, 1))
	{
		return;
	}

	Grid = MoveTemp(NewGrid);
	Equipped = MoveTemp(NewEquipped);
	HandleGridChanged();
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
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	return Grid.GetTotalWeight(Lookup) + Rubezh::Equipment::GetWeight(Equipped, Lookup);
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
		const float CarryBonus = Rubezh::Equipment::GetCarryBonusKg(Equipped, [this](FName Id) { return FindDefinition(Id); });
		ASC->SetNumericAttributeBase(URubezhSurvivalSet::GetMaxCarryWeightAttribute(), Rubezh::Encumbrance::BaseMaxCarryWeight + CarryBonus);
		ASC->SetNumericAttributeBase(URubezhSurvivalSet::GetCarriedWeightAttribute(), GetTotalWeight());
	}
}
