// РУБЕЖ: Земля после разлома

#include "Settlement/RubezhSafeZone.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Player/LyraPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhSafeZone)

ARubezhSafeZone::ARubezhSafeZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Зона ни с чем не сталкивается: принадлежность считается по координатам, одинаково на сервере и у клиента.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->SetHiddenInGame(true);
	RootComponent = Box;
}

void ARubezhSafeZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Box->SetBoxExtent(HalfSizeCm);
}

bool ARubezhSafeZone::Contains(const FVector& Location) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Location);
	return FMath::Abs(Local.X) <= HalfSizeCm.X && FMath::Abs(Local.Y) <= HalfSizeCm.Y && FMath::Abs(Local.Z) <= HalfSizeCm.Z;
}

void ARubezhSafeZone::FilterPlayerStarts(AController* Player, TArray<ALyraPlayerStart*>& PlayerStarts)
{
	const bool bWantInside = Player && Player->IsA<APlayerController>();
	TArray<ALyraPlayerStart*> Suitable = PlayerStarts.FilterByPredicate([bWantInside](const ALyraPlayerStart* Start)
	{
		return IsInsideAny(Start->GetWorld(), Start->GetActorLocation()) == bWantInside;
	});

	// На карте без поселения все появляются где обычно.
	if (!Suitable.IsEmpty())
	{
		PlayerStarts = MoveTemp(Suitable);
	}
}

bool ARubezhSafeZone::IsInsideAny(const UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ARubezhSafeZone> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->Contains(Location))
		{
			return true;
		}
	}
	return false;
}
