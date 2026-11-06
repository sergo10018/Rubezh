// РУБЕЖ: Земля после разлома

#include "Animals/RubezhAnimal.h"

#include "AIController.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animals/RubezhAnimalSpawner.h"
#include "Animation/AnimationAsset.h"
#include "Character/LyraCharacter.h"
#include "Character/LyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Items/RubezhItemCatalog.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Physics/LyraCollisionChannels.h"
#include "RubezhCoreRuntimeModule.h"
#include "Settlement/RubezhSafeZone.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhAnimal)

namespace
{
	constexpr float BrainPeriodSeconds = 0.25f;
	constexpr float AnimationPeriodSeconds = 0.2f;
	constexpr float BodyLifetimeSeconds = 60.0f;
	constexpr float IdleBelowSpeed = 20.0f;
	constexpr float WanderPauseMinSeconds = 3.0f;
	constexpr float WanderPauseMaxSeconds = 9.0f;
	constexpr float FleeDistanceCm = 4000.0f;
	constexpr float EyeHeightCm = 60.0f;
	const TCHAR* const DamageEffectPath = TEXT("/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C");
}

ARubezhAnimal::ARubezhAnimal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	SetReplicates(true);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	// Оружие Lyra ищет цель каналом Weapon: капсула и модель должны его останавливать.
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("LyraPawnCapsule"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(Lyra_TraceChannel_Weapon, ECR_Block);
	GetMesh()->SetCollisionProfileName(TEXT("LyraPawnMesh"));
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	AbilitySystem = ObjectInitializer.CreateDefaultSubobject<ULyraAbilitySystemComponent>(this, TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	HealthSet = ObjectInitializer.CreateDefaultSubobject<ULyraHealthSet>(this, TEXT("HealthSet"));

	PlaceholderBody = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlaceholderBody->SetStaticMesh(CubeMesh.Object);
	}
}

void ARubezhAnimal::Setup(FName InAnimalId, ARubezhAnimalSpawner* InSpawner, int32 InPackIndex)
{
	AnimalId = InAnimalId;
	Spawner = InSpawner;
	PackIndex = InPackIndex;
	// Размер капсулы нужен до проверки места при появлении: иначе крупный зверь застрянет в земле.
	ApplyDefinition();
}

UAbilitySystemComponent* ARubezhAnimal::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void ARubezhAnimal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARubezhAnimal, AnimalId);
	DOREPLIFETIME(ARubezhAnimal, State);
}

void ARubezhAnimal::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	ApplyDefinition();
	AbilitySystem->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		if (!Rubezh::Items::FindAnimal(AnimalId))
		{
			// Вид неизвестен: без чисел зверь был бы живым трупом без здоровья.
			Destroy();
			return;
		}
		AbilitySystem->SetNumericAttributeBase(ULyraHealthSet::GetMaxHealthAttribute(), Definition.MaxHealth);
		AbilitySystem->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), Definition.MaxHealth);
		HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
		HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
		GetWorldTimerManager().SetTimer(BrainTimer, this, &ThisClass::UpdateBrain, BrainPeriodSeconds, true, FMath::FRand() * BrainPeriodSeconds);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetWorldTimerManager().SetTimer(AnimationTimer, this, &ThisClass::UpdateAnimation, AnimationPeriodSeconds, true);
	}
}

void ARubezhAnimal::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(BrainTimer);
	GetWorldTimerManager().ClearTimer(AnimationTimer);
	Super::EndPlay(Reason);
}

void ARubezhAnimal::OnRep_AnimalId()
{
	ApplyDefinition();
}

void ARubezhAnimal::ApplyDefinition()
{
	const FRubezhAnimalDefinition* Found = Rubezh::Items::FindAnimal(AnimalId);
	if (!Found)
	{
		if (!AnimalId.IsNone())
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh: нет животного '%s' в таблице"), *AnimalId.ToString());
		}
		return;
	}
	Definition = *Found;

	GetCapsuleComponent()->SetCapsuleSize(Definition.CapsuleRadiusCm, Definition.CapsuleHalfHeightCm);
	GetCharacterMovement()->MaxWalkSpeed = Definition.WalkSpeed;

	USkeletalMesh* Model = Definition.Mesh.LoadSynchronous();
	if (Model)
	{
		GetMesh()->SetSkeletalMesh(Model);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -Definition.CapsuleHalfHeightCm));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, Definition.MeshYaw, 0.0f));
		GetMesh()->SetRelativeScale3D(FVector(Definition.MeshScale));
		PlaceholderBody->SetVisibility(false);
	}
	else
	{
		// Куб движка — 100 см: длиннее вперёд, чтобы было видно, куда зверь смотрит.
		const float Width = Definition.CapsuleRadiusCm * 2.0f / 100.0f;
		PlaceholderBody->SetRelativeScale3D(FVector(Width * 1.8f, Width, Definition.CapsuleHalfHeightCm * 2.0f / 100.0f));
	}
}

float ARubezhAnimal::GetHealthFraction() const
{
	const float Max = HealthSet->GetMaxHealth();
	return Max > 0.0f ? HealthSet->GetHealth() / Max : 0.0f;
}

bool ARubezhAnimal::CanSee(const APawn& Candidate) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RubezhAnimalSight), false, this);
	Params.AddIgnoredActor(&Candidate);
	const FVector Eye = GetActorLocation() + FVector(0.0f, 0.0f, EyeHeightCm);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Candidate.GetActorLocation(), ECC_Visibility, Params);
}

APawn* ARubezhAnimal::FindNearestTarget() const
{
	APawn* Nearest = nullptr;
	double NearestDistance = FMath::Square<double>(Definition.AggroRadiusCm);
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		ALyraCharacter* Candidate = *It;
		const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Candidate);
		const double Distance = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (Distance < NearestDistance && Health && Health->GetDeathState() == ELyraDeathState::NotDead
			&& !ARubezhSafeZone::IsInsideAny(GetWorld(), Candidate->GetActorLocation()) && CanSee(*Candidate))
		{
			Nearest = Candidate;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void ARubezhAnimal::Alert(APawn* NewTarget)
{
	if (IsAlive() && NewTarget && !Target)
	{
		Target = NewTarget;
	}
}

void ARubezhAnimal::UpdateBrain()
{
	if (!IsAlive())
	{
		return;
	}

	const ULyraHealthComponent* TargetHealth = Target ? ULyraHealthComponent::FindHealthComponent(Target) : nullptr;
	if (!IsValid(Target) || !TargetHealth || TargetHealth->GetDeathState() != ELyraDeathState::NotDead)
	{
		Target = nullptr;
	}
	if (!Target)
	{
		Target = FindNearestTarget();
		if (Target && Spawner.IsValid())
		{
			Spawner->AlertPack(Target);
		}
	}

	Rubezh::Animals::FSenses Senses;
	Senses.bHasTarget = Target != nullptr;
	Senses.TargetDistanceCm = Target ? FVector::Dist(Target->GetActorLocation(), GetActorLocation()) : 0.0f;
	Senses.bTargetInSafeZone = Target && ARubezhSafeZone::IsInsideAny(GetWorld(), Target->GetActorLocation());
	Senses.HealthFraction = GetHealthFraction();
	// Разбитая стая бежит один раз, потом уцелевшие снова опасны: иначе логово никогда не восстановится.
	Senses.bPackRouted = !bFledFromRout && Spawner.IsValid() && Rubezh::Animals::IsPackRouted(Definition, Spawner->CountAlive(), Spawner->GetPackSize());
	bFledFromRout = bFledFromRout || Senses.bPackRouted;
	Senses.bStillFleeing = GetWorld()->GetTimeSeconds() < FleeUntil;

	const ERubezhAnimalState NewState = Rubezh::Animals::Decide(Definition, Senses);
	if (NewState == ERubezhAnimalState::Wander && Target && (Senses.bTargetInSafeZone || Senses.TargetDistanceCm > Definition.LoseRadiusCm))
	{
		Target = nullptr;
	}
	if (NewState == ERubezhAnimalState::Flee && State != ERubezhAnimalState::Flee)
	{
		FleeUntil = GetWorld()->GetTimeSeconds() + Rubezh::Animals::FleeSeconds;
		NextWanderTime = 0.0f;
	}
	State = NewState;
	MoveForState(NewState, Senses.TargetDistanceCm);
}

void ARubezhAnimal::MoveForState(ERubezhAnimalState NewState, float TargetDistance)
{
	const float Now = GetWorld()->GetTimeSeconds();
	switch (NewState)
	{
	case ERubezhAnimalState::Wander:
		if (Now >= NextWanderTime)
		{
			NextWanderTime = Now + FMath::FRandRange(WanderPauseMinSeconds, WanderPauseMaxSeconds);
			const FVector2D Offset = FMath::RandPointInCircle(Rubezh::Animals::WanderRadiusCm);
			MoveTo(HomeLocation + FVector(Offset, 0.0f), Definition.WalkSpeed);
		}
		break;

	case ERubezhAnimalState::Chase:
	{
		FVector Destination = Target->GetActorLocation();
		// Стая не бежит гуськом: издалека каждый идёт на своё место вокруг цели, вблизи — прямо к ней.
		if (Spawner.IsValid() && Spawner->GetPackSize() > 1 && TargetDistance > Rubezh::Animals::SurroundRadiusCm)
		{
			Destination += FVector(Rubezh::Animals::GetSurroundOffset(PackIndex, Spawner->GetPackSize(), Rubezh::Animals::SurroundRadiusCm), 0.0f);
		}
		MoveTo(Destination, Rubezh::Animals::GetChaseSpeed(Definition, TargetDistance));
		break;
	}

	case ERubezhAnimalState::Bite:
		if (AAIController* AI = GetController<AAIController>())
		{
			AI->StopMovement();
		}
		if (Now >= NextBiteTime)
		{
			NextBiteTime = Now + Definition.BiteSeconds;
			Bite();
		}
		break;

	case ERubezhAnimalState::Flee:
		if (Now >= NextWanderTime)
		{
			NextWanderTime = Now + 2.0f;
			// Ушёл от угрозы — возвращается к логову, а не бежит по карте без конца.
			if (Target && TargetDistance <= Definition.LoseRadiusCm)
			{
				const FVector Away = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
				MoveTo(GetActorLocation() + Away * FleeDistanceCm, Definition.RunSpeed);
			}
			else
			{
				MoveTo(HomeLocation, Definition.WalkSpeed);
			}
		}
		break;

	default:
		break;
	}
}

void ARubezhAnimal::MoveTo(const FVector& Destination, float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (AAIController* AI = GetController<AAIController>())
	{
		const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Destination, 50.0f, false);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			UE_LOG(LogRubezh, Verbose, TEXT("%s: нет пути к %s"), *GetName(), *Destination.ToCompactString());
		}
	}
}

void ARubezhAnimal::Bite()
{
	UAbilitySystemComponent* TargetAbilities = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	const TSubclassOf<UGameplayEffect> DamageEffect = LoadClass<UGameplayEffect>(nullptr, DamageEffectPath);
	if (!TargetAbilities || !DamageEffect)
	{
		return;
	}

	// Тот же путь, что у попадания пули: броня уменьшает урон и изнашивается, укус вызывает кровотечение и переломы.
	// Источник — сама цель: система команд Lyra не даёт бить тем, у кого нет команды, а животное вне команд.
	const bool bLeg = FMath::FRand() * 100.0f < Definition.LegBitePercent;
	FHitResult Hit;
	Hit.BoneName = bLeg ? FName(TEXT("calf_l")) : FName(TEXT("spine_03"));
	Hit.HitObjectHandle = FActorInstanceHandle(Target);
	Hit.ImpactPoint = Target->GetActorLocation();
	Hit.Location = Hit.ImpactPoint;

	FGameplayEffectContextHandle Context = TargetAbilities->MakeEffectContext();
	Context.AddHitResult(Hit, true);
	const FGameplayEffectSpecHandle Spec = TargetAbilities->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
	Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Damage")), Definition.BiteDamage);
	TargetAbilities->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	UE_LOG(LogRubezh, Verbose, TEXT("%s кусает %s: %s"), *AnimalId.ToString(), *GetNameSafe(Target), *Hit.BoneName.ToString());
}

void ARubezhAnimal::HandleHealthChanged(AActor* DamageInstigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	if (NewValue >= OldValue || !IsAlive())
	{
		return;
	}
	// Подстрелили — вся стая знает, кто.
	APawn* Attacker = Cast<APawn>(Causer);
	if (!Attacker)
	{
		Attacker = Cast<APawn>(DamageInstigator);
	}
	if (const AController* AttackerController = Cast<AController>(DamageInstigator))
	{
		Attacker = AttackerController->GetPawn() ? ToRawPtr(AttackerController->GetPawn()) : Attacker;
	}
	else if (const APlayerState* AttackerState = Cast<APlayerState>(DamageInstigator))
	{
		Attacker = AttackerState->GetPawn() ? ToRawPtr(AttackerState->GetPawn()) : Attacker;
	}
	if (Attacker && Attacker != this)
	{
		Target = Attacker;
		if (Spawner.IsValid())
		{
			Spawner->AlertPack(Attacker);
		}
	}
}

void ARubezhAnimal::HandleOutOfHealth(AActor* DamageInstigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	Die();
}

void ARubezhAnimal::Die()
{
	if (!IsAlive())
	{
		return;
	}
	State = ERubezhAnimalState::Dead;
	GetWorldTimerManager().ClearTimer(BrainTimer);
	if (AAIController* AI = GetController<AAIController>())
	{
		AI->StopMovement();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(BodyLifetimeSeconds);
	if (Spawner.IsValid())
	{
		Spawner->HandleMemberDied();
	}
	UE_LOG(LogRubezh, Log, TEXT("%s: %s погиб"), *GetName(), *AnimalId.ToString());
}

void ARubezhAnimal::UpdateAnimation()
{
	if (PlaceholderBody->GetVisibleFlag())
	{
		// Коробка падает на бок, чтобы мёртвого зверя было видно без анимаций.
		if (State == ERubezhAnimalState::Dead)
		{
			PlaceholderBody->SetRelativeRotation(FRotator(0.0f, 0.0f, 80.0f));
			GetWorldTimerManager().ClearTimer(AnimationTimer);
		}
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const TSoftObjectPtr<UAnimationAsset>& Wanted = State == ERubezhAnimalState::Dead ? Definition.DeathAnim
		: State == ERubezhAnimalState::Bite ? Definition.BiteAnim
		: Speed < IdleBelowSpeed ? Definition.IdleAnim
		: Speed <= Definition.WalkSpeed + 50.0f ? Definition.WalkAnim
		: Definition.RunAnim;
	UAnimationAsset* Anim = Wanted.LoadSynchronous();
	if (Anim && Anim != PlayingAnim)
	{
		PlayingAnim = Anim;
		GetMesh()->PlayAnimation(Anim, State != ERubezhAnimalState::Dead);
	}
	if (State == ERubezhAnimalState::Dead)
	{
		GetWorldTimerManager().ClearTimer(AnimationTimer);
	}
}
