// РУБЕЖ: Земля после разлома

#include "Health/RubezhInjuryComponent.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Health/RubezhHealthEffects.h"
#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhInventoryComponent.h"
#include "LyraGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhGameplayTags.h"
#include "UI/RubezhHealthStatusWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInjuryComponent)

using namespace Rubezh::Injuries;

namespace
{
	// Строка состояния под окнами Lyra, как и подсказка подбора.
	constexpr int32 StatusWidgetZOrder = -10;

	FGameplayTag GetInjuryTag(ERubezhInjury Injury)
	{
		switch (Injury)
		{
		case ERubezhInjury::Bleeding:      return RubezhGameplayTags::Status_Injury_Bleeding;
		case ERubezhInjury::HeavyBleeding: return RubezhGameplayTags::Status_Injury_HeavyBleeding;
		case ERubezhInjury::LegFracture:   return RubezhGameplayTags::Status_Injury_LegFracture;
		case ERubezhInjury::Pain:          return RubezhGameplayTags::Status_Injury_Pain;
		default:                           return FGameplayTag();
		}
	}

	constexpr ERubezhInjury AllInjuries[] = { ERubezhInjury::Bleeding, ERubezhInjury::HeavyBleeding, ERubezhInjury::LegFracture, ERubezhInjury::Pain };

	/** Пока идёт лечение, руки заняты: нельзя стрелять, целиться, перезаряжаться, бросать гранату, бить и бегать. */
	const FGameplayTagContainer& GetTreatmentBlockedAbilities()
	{
		static const FGameplayTagContainer Tags = []
		{
			FGameplayTagContainer Result;
			for (const TCHAR* Name : { TEXT("Ability.Type.Action.WeaponFire"), TEXT("Ability.Type.Action.ADS"), TEXT("Ability.Type.Action.Reload"),
				TEXT("Ability.Type.Action.Grenade"), TEXT("Ability.Type.Action.Melee") })
			{
				Result.AddTag(FGameplayTag::RequestGameplayTag(Name, false));
			}
			Result.AddTag(RubezhGameplayTags::Ability_Sprint);
			return Result;
		}();
		return Tags;
	}

	void SetLooseTag(UAbilitySystemComponent& ASC, const FGameplayTag& Tag, bool bPresent)
	{
		// Тег виден и серверу, и клиенту (по нему клиент не даёт начать спринт при переломе).
		ASC.SetLooseGameplayTagCount(Tag, bPresent ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	}
}

URubezhInjuryComponent::URubezhInjuryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

URubezhInjuryComponent* URubezhInjuryComponent::FindForActor(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<URubezhInjuryComponent>() : nullptr;
}

void URubezhInjuryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Свои травмы знает только сам игрок (видимые другим следы — кровь, хромота — позже).
	DOREPLIFETIME_CONDITION(URubezhInjuryComponent, State, COND_OwnerOnly);
}

void URubezhInjuryComponent::BeginPlay()
{
	Super::BeginPlay();
	Random.Initialize(FMath::Rand());
}

void URubezhInjuryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ASC живёт на PlayerState и переживает персонажа — снимаем с него всё, что ставили, иначе новый персонаж родится «раненым».
	ApplyTreatmentBlock(false);
	if (GetOwner()->HasAuthority())
	{
		ClearInjuryTags();
	}

	if (UAbilitySystemComponent* ASC = BoundAbilitySystem.Get())
	{
		if (const ULyraHealthSet* HealthSet = ASC->GetSet<ULyraHealthSet>())
		{
			HealthSet->OnHealthChanged.Remove(HealthChangedHandle);
		}
	}
	BoundAbilitySystem.Reset();

	if (StatusWidget)
	{
		StatusWidget->RemoveFromParent();
		StatusWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* URubezhInjuryComponent::GetAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn<APawn>());
}

double URubezhInjuryComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

bool URubezhInjuryComponent::IsDead() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystem();
	return !ASC || ASC->HasMatchingGameplayTag(LyraGameplayTags::Status_Death);
}

bool URubezhInjuryComponent::IsLocalPlayerPawn() const
{
	const APawn* Pawn = GetPawn<APawn>();
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

bool URubezhInjuryComponent::IsPainRelieved() const
{
	return GetServerTime() < State.PainReliefUntil;
}

float URubezhInjuryComponent::GetTreatmentProgress() const
{
	const double Duration = State.TreatmentEnd - State.TreatmentStart;
	return (IsTreating() && Duration > 0.0) ? FMath::Clamp(static_cast<float>((GetServerTime() - State.TreatmentStart) / Duration), 0.0f, 1.0f) : 0.0f;
}

void URubezhInjuryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		TickServer(DeltaTime);
	}
	if (IsLocalPlayerPawn())
	{
		TickLocal(DeltaTime);
	}
}

// ---------------------------------------------------------------- сервер

void URubezhInjuryComponent::TickServer(float DeltaTime)
{
	TryBindHealth();

	if (IsDead())
	{
		if (IsTreating())
		{
			// Лечение прервано смертью — предмет не тратится.
			State.TreatmentItem = NAME_None;
			ApplyTreatmentBlock(false);
		}
		State.PendingHeals.Reset();
		return;
	}

	TrackFall();
	ApplyBleeding(DeltaTime);
	UpdatePain();
	ApplyPendingHeals();

	if (IsTreating() && GetServerTime() >= State.TreatmentEnd)
	{
		FinishTreatment();
	}
}

void URubezhInjuryComponent::TryBindHealth()
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (ASC == BoundAbilitySystem.Get())
	{
		return;
	}

	if (UAbilitySystemComponent* OldASC = BoundAbilitySystem.Get())
	{
		if (const ULyraHealthSet* OldSet = OldASC->GetSet<ULyraHealthSet>())
		{
			OldSet->OnHealthChanged.Remove(HealthChangedHandle);
		}
	}
	BoundAbilitySystem = ASC;
	HealthChangedHandle.Reset();

	if (const ULyraHealthSet* HealthSet = ASC ? ASC->GetSet<ULyraHealthSet>() : nullptr)
	{
		HealthChangedHandle = HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	}
}

void URubezhInjuryComponent::HandleHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	if (NewValue >= OldValue || NewValue <= 0.0f)
	{
		return;
	}
	// Урон от самого кровотечения новых травм не даёт.
	if (Spec && Spec->GetDynamicAssetTags().HasTagExact(RubezhGameplayTags::Damage_Injury))
	{
		return;
	}

	FHitInfo Hit;
	Hit.Damage = OldValue - NewValue;
	if (Spec)
	{
		if (const FHitResult* HitResult = Spec->GetEffectContext().GetHitResult())
		{
			Hit.bLegHit = IsLegBone(HitResult->BoneName);
		}
	}
	AddInjuries(RollHitInjuries(Hit, Random));
}

void URubezhInjuryComponent::AddInjuries(FMask NewInjuries)
{
	if (!GetOwner()->HasAuthority() || NewInjuries == 0 || IsDead())
	{
		return;
	}

	State.Injuries |= NewInjuries;
	if (Has(NewInjuries, ERubezhInjury::Pain))
	{
		State.PainUntil = GetServerTime() + PainDurationSeconds;
	}
	if (Has(NewInjuries, ERubezhInjury::LegFracture))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystem())
		{
			const FGameplayTagContainer Sprint(RubezhGameplayTags::Ability_Sprint);
			ASC->CancelAbilities(&Sprint);
		}
	}
	SyncInjuryTags();
	UE_LOG(LogRubezh, Verbose, TEXT("%s: травмы 0x%02x"), *GetNameSafe(GetOwner()), State.Injuries);
}

void URubezhInjuryComponent::TrackFall()
{
	const ACharacter* Character = GetPawn<ACharacter>();
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	const float Z = Character->GetActorLocation().Z;
	const bool bFalling = Movement->IsFalling();
	if (bFalling)
	{
		FallApexZ = bWasFalling ? FMath::Max(FallApexZ, Z) : Z;
	}
	else if (bWasFalling && IsFallFracture(FallApexZ - Z))
	{
		AddInjuries(Bit(ERubezhInjury::LegFracture) | Bit(ERubezhInjury::Pain));
	}
	bWasFalling = bFalling;
}

void URubezhInjuryComponent::ApplyBleeding(float DeltaTime)
{
	const float LossPerSecond = GetHealthLossPerSecond(State.Injuries);
	if (LossPerSecond <= 0.0f)
	{
		BleedAccumulator = 0.0f;
		return;
	}

	BleedAccumulator += DeltaTime;
	while (BleedAccumulator >= BleedTickSeconds)
	{
		BleedAccumulator -= BleedTickSeconds;
		ApplyHealthEffect(URubezhGE_InjuryDamage::StaticClass(), RubezhGameplayTags::SetByCaller_Damage, LossPerSecond * BleedTickSeconds);
	}
}

void URubezhInjuryComponent::UpdatePain()
{
	if (Has(State.Injuries, ERubezhInjury::Pain) && GetServerTime() >= State.PainUntil)
	{
		State.Injuries &= ~Bit(ERubezhInjury::Pain);
		SyncInjuryTags();
	}
}

void URubezhInjuryComponent::ServerUseItem_Implementation(FName ItemId)
{
	const APawn* Pawn = GetPawn<APawn>();
	URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(Pawn);
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Pawn);
	if (!Inventory || !Health || IsTreating() || IsDead() || Inventory->CountItem(ItemId) < 1)
	{
		return;
	}

	const FRubezhItemDefinition* Item = Inventory->FindDefinition(ItemId);
	if (!Item || !IsUseful(*Item, State.Injuries, IsPainRelieved(), Health->GetHealth(), Health->GetMaxHealth()))
	{
		return;
	}

	State.TreatmentItem = ItemId;
	State.TreatmentStart = GetServerTime();
	State.TreatmentEnd = State.TreatmentStart + Item->UseSeconds;
	ApplyTreatmentBlock(true);
	SyncInjuryTags();
}

void URubezhInjuryComponent::FinishTreatment()
{
	const FName ItemId = State.TreatmentItem;
	State.TreatmentItem = NAME_None;
	ApplyTreatmentBlock(false);

	// Предмет тратится только в конце: прерванное лечение ничего не стоит.
	URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>());
	const FRubezhItemDefinition* Item = Inventory ? Inventory->FindDefinition(ItemId) : nullptr;
	if (!Item || Inventory->RemoveItem(ItemId, 1) != 1)
	{
		SyncInjuryTags();
		return;
	}

	if (Item->Cures != ERubezhInjury::None)
	{
		State.Injuries &= ~Bit(Item->Cures);
	}
	if (Item->PainReliefSeconds > 0.0f)
	{
		State.PainReliefUntil = GetServerTime() + Item->PainReliefSeconds;
	}
	HealPercent(Item->HealPercent);
	if (Item->DelayedHealPercent > 0.0f)
	{
		// Аптечка «действует» ещё какое-то время — вторая часть здоровья придёт позже (урон её не отменяет, смерть — отменяет).
		State.PendingHeals.Add(FRubezhPendingHeal{ GetServerTime() + Item->DelayedHealSeconds, Item->DelayedHealPercent });
	}
	SyncInjuryTags();
}

void URubezhInjuryComponent::ApplyPendingHeals()
{
	const double Now = GetServerTime();
	for (int32 Index = State.PendingHeals.Num() - 1; Index >= 0; --Index)
	{
		if (Now >= State.PendingHeals[Index].At)
		{
			HealPercent(State.PendingHeals[Index].Percent);
			State.PendingHeals.RemoveAt(Index);
		}
	}
}

void URubezhInjuryComponent::HealPercent(float Percent) const
{
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(GetPawn<APawn>());
	if (Health && Percent > 0.0f)
	{
		ApplyHealthEffect(URubezhGE_Heal::StaticClass(), RubezhGameplayTags::SetByCaller_Healing, PercentToHealth(Percent, Health->GetMaxHealth()));
	}
}

float URubezhInjuryComponent::GetPendingHealSecondsLeft() const
{
	double Earliest = TNumericLimits<double>::Max();
	for (const FRubezhPendingHeal& Heal : State.PendingHeals)
	{
		Earliest = FMath::Min(Earliest, Heal.At);
	}
	return State.PendingHeals.IsEmpty() ? 0.0f : FMath::Max(0.0f, static_cast<float>(Earliest - GetServerTime()));
}

void URubezhInjuryComponent::ApplyHealthEffect(TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& MagnitudeTag, float Amount) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC || Amount <= 0.0f)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Amount);
	Spec.Data->AddDynamicAssetTag(RubezhGameplayTags::Damage_Injury);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

void URubezhInjuryComponent::SyncInjuryTags() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC)
	{
		return;
	}
	for (const ERubezhInjury Injury : AllInjuries)
	{
		SetLooseTag(*ASC, GetInjuryTag(Injury), Has(State.Injuries, Injury));
	}
	SetLooseTag(*ASC, RubezhGameplayTags::Status_Treating, IsTreating());
}

void URubezhInjuryComponent::ClearInjuryTags() const
{
	UAbilitySystemComponent* ASC = BoundAbilitySystem.Get() ? BoundAbilitySystem.Get() : GetAbilitySystem();
	if (!ASC)
	{
		return;
	}
	for (const ERubezhInjury Injury : AllInjuries)
	{
		SetLooseTag(*ASC, GetInjuryTag(Injury), false);
	}
	SetLooseTag(*ASC, RubezhGameplayTags::Status_Treating, false);
}

// ---------------------------------------------------------------- все

void URubezhInjuryComponent::OnRep_State()
{
	// Блокировка способностей не реплицируется — клиент ставит её сам, иначе он «предскажет» выстрел, который сервер отвергнет.
	if (IsLocalPlayerPawn())
	{
		ApplyTreatmentBlock(IsTreating());
	}
}

void URubezhInjuryComponent::ApplyTreatmentBlock(bool bBlock)
{
	if (bBlock == bAbilitiesBlocked)
	{
		return;
	}
	UAbilitySystemComponent* ASC = BoundAbilitySystem.Get() ? BoundAbilitySystem.Get() : GetAbilitySystem();
	if (!ASC)
	{
		return;
	}

	bAbilitiesBlocked = bBlock;
	if (bBlock)
	{
		ASC->BlockAbilitiesWithTags(GetTreatmentBlockedAbilities());
		CancelBlockedAbilities();
	}
	else
	{
		ASC->UnBlockAbilitiesWithTags(GetTreatmentBlockedAbilities());
	}
}

void URubezhInjuryComponent::CancelBlockedAbilities() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystem())
	{
		ASC->CancelAbilities(&GetTreatmentBlockedAbilities());
	}
}

void URubezhInjuryComponent::UseItem(FName ItemId)
{
	if (!ItemId.IsNone())
	{
		ServerUseItem(ItemId);
	}
}

void URubezhInjuryComponent::CollectInventoryMedicine(TArray<TPair<FName, const FRubezhItemDefinition*>>& OutAvailable) const
{
	const URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>());
	if (!Inventory)
	{
		return;
	}
	for (const FRubezhInventoryEntry& Entry : Inventory->GetGrid().GetEntries())
	{
		const FRubezhItemDefinition* Item = Inventory->FindDefinition(Entry.ItemId);
		const bool bAlreadyListed = OutAvailable.ContainsByPredicate([&Entry](const TPair<FName, const FRubezhItemDefinition*>& Pair) { return Pair.Key == Entry.ItemId; });
		if (Item && IsMedical(*Item) && !bAlreadyListed)
		{
			OutAvailable.Emplace(Entry.ItemId, Item);
		}
	}
}

void URubezhInjuryComponent::QuickTreat()
{
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(GetPawn<APawn>());
	if (!Health || IsTreating())
	{
		return;
	}

	TArray<TPair<FName, const FRubezhItemDefinition*>> Available;
	CollectInventoryMedicine(Available);
	const FName Choice = ChooseQuickTreatment(Available, State.Injuries, IsPainRelieved(), Health->GetHealth(), Health->GetMaxHealth());
	if (Choice.IsNone())
	{
		UE_LOG(LogRubezh, Display, TEXT("Быстрое лечение: лечить нечего или нечем"));
		return;
	}
	UseItem(Choice);
}

// ---------------------------------------------------------------- свой клиент

void URubezhInjuryComponent::TickLocal(float DeltaTime)
{
	ApplyPainSway(DeltaTime);
	UpdateStatusWidget();
}

void URubezhInjuryComponent::ApplyPainSway(float DeltaTime)
{
	APlayerController* Controller = GetController<APlayerController>();
	const bool bInPain = Has(State.Injuries, ERubezhInjury::Pain) && !IsPainRelieved() && !IsDead();
	if (!Controller)
	{
		return;
	}

	// Плавное «плавание» прицела: к повороту камеры добавляется медленный шум, а не случайные рывки.
	FRotator Target = FRotator::ZeroRotator;
	if (bInPain)
	{
		PainSwayTime += DeltaTime * PainSwayFrequency;
		Target.Yaw = FMath::PerlinNoise1D(PainSwayTime) * PainSwayDegrees;
		Target.Pitch = FMath::PerlinNoise1D(PainSwayTime + 31.7f) * PainSwayDegrees * 0.6f;
	}
	const FRotator Delta = Target - PainSwayOffset;
	if (!Delta.IsNearlyZero())
	{
		Controller->SetControlRotation(Controller->GetControlRotation() + Delta);
		PainSwayOffset = Target;
	}
}

void URubezhInjuryComponent::UpdateStatusWidget()
{
	const float PendingHealSeconds = GetPendingHealSecondsLeft();
	const bool bHasSomething = State.Injuries != 0 || IsTreating() || PendingHealSeconds > 0.0f;
	if (!bHasSomething)
	{
		if (StatusWidget)
		{
			StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!StatusWidget)
	{
		APlayerController* Controller = GetController<APlayerController>();
		if (!Controller)
		{
			return;
		}
		StatusWidget = CreateWidget<URubezhHealthStatusWidget>(Controller);
		StatusWidget->AddToPlayerScreen(StatusWidgetZOrder);
	}

	FText TreatmentName;
	if (IsTreating())
	{
		const URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>());
		const FRubezhItemDefinition* Item = Inventory ? Inventory->FindDefinition(State.TreatmentItem) : nullptr;
		TreatmentName = Item ? Item->DisplayName : FText::FromName(State.TreatmentItem);
	}
	StatusWidget->SetStatus(State.Injuries, IsPainRelieved(), TreatmentName, GetTreatmentProgress(), PendingHealSeconds);
	StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}
