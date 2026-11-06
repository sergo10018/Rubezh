// РУБЕЖ: Земля после разлома

#include "Animals/RubezhAnimalSpawner.h"

#include "Animals/RubezhAnimal.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RubezhCoreRuntimeModule.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhAnimalSpawner)

namespace
{
	constexpr float PackSpreadCm = 300.0f;
	constexpr float GroundProbeCm = 5000.0f;
	constexpr float SpawnLiftCm = 120.0f;
}

ARubezhAnimalSpawner::ARubezhAnimalSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false;
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Root"));
#if WITH_EDITORONLY_DATA
	if (UBillboardComponent* Icon = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Icon")))
	{
		Icon->SetupAttachment(RootComponent);
	}
#endif
}

void ARubezhAnimalSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SpawnPack();
	}
}

ARubezhAnimalSpawner* ARubezhAnimalSpawner::SpawnAt(UWorld& World, const FVector& Location, FName AnimalId, int32 Count)
{
	ARubezhAnimalSpawner* Spawner = World.SpawnActorDeferred<ARubezhAnimalSpawner>(StaticClass(), FTransform(Location));
	if (Spawner)
	{
		Spawner->AnimalId = AnimalId;
		Spawner->Count = FMath::Max(Count, 1);
		Spawner->RespawnSeconds = 0.0f;
		UGameplayStatics::FinishSpawningActor(Spawner, FTransform(Location));
	}
	return Spawner;
}

void ARubezhAnimalSpawner::SpawnPack()
{
	Members.Reset();
	UWorld* World = GetWorld();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector2D Offset = FMath::RandPointInCircle(PackSpreadCm);
		FVector Location = GetActorLocation() + FVector(Offset, 0.0f);

		// На холмах логово и зверь могут оказаться на разной высоте: ставим на землю.
		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RubezhAnimalSpawn), false, this);
		if (World->LineTraceSingleByChannel(Ground, Location + FVector(0.0f, 0.0f, GroundProbeCm), Location - FVector(0.0f, 0.0f, GroundProbeCm), ECC_WorldStatic, Params))
		{
			Location = Ground.ImpactPoint + FVector(0.0f, 0.0f, SpawnLiftCm);
		}

		const FTransform Transform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Location);
		ARubezhAnimal* Animal = World->SpawnActorDeferred<ARubezhAnimal>(ARubezhAnimal::StaticClass(), Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Animal)
		{
			Animal->Setup(AnimalId, this, Index);
			UGameplayStatics::FinishSpawningActor(Animal, Transform);
			Members.Add(Animal);
		}
	}
	UE_LOG(LogRubezh, Log, TEXT("%s: появилось %s × %d"), *GetName(), *AnimalId.ToString(), Members.Num());
}

int32 ARubezhAnimalSpawner::CountAlive() const
{
	int32 Alive = 0;
	for (const ARubezhAnimal* Member : Members)
	{
		Alive += (IsValid(Member) && Member->IsAlive()) ? 1 : 0;
	}
	return Alive;
}

void ARubezhAnimalSpawner::AlertPack(APawn* Target)
{
	for (ARubezhAnimal* Member : Members)
	{
		if (IsValid(Member))
		{
			Member->Alert(Target);
		}
	}
}

void ARubezhAnimalSpawner::HandleMemberDied()
{
	if (CountAlive() == 0 && RespawnSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::SpawnPack, RespawnSeconds, false);
	}
}
