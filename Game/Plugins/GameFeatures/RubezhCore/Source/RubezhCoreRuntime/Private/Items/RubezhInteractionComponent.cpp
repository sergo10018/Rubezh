// РУБЕЖ: Земля после разлома

#include "Items/RubezhInteractionComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhPickupRules.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhPickupPrompt.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInteractionComponent)

namespace
{
	// Подсказка под окнами интерфейса Lyra.
	constexpr int32 PromptZOrder = -10;
}

URubezhInteractionComponent::URubezhInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void URubezhInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Prompt)
	{
		Prompt->RemoveFromParent();
		Prompt = nullptr;
	}
	FocusedItem.Reset();

	Super::EndPlay(EndPlayReason);
}

bool URubezhInteractionComponent::IsLocalPlayerPawn() const
{
	const APawn* Pawn = GetPawn<APawn>();
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

void URubezhInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Цель и подсказка нужны только тому, кто управляет персонажем; сервер проверяет подбор отдельно.
	if (!IsLocalPlayerPawn())
	{
		return;
	}

	FocusedItem = FindFocusedItem();
	UpdatePrompt();
}

ARubezhWorldItem* URubezhInteractionComponent::FindFocusedItem() const
{
	const APawn* Pawn = GetPawn<APawn>();
	const APlayerController* Controller = GetController<APlayerController>();
	UWorld* World = GetWorld();
	if (!Pawn || !Controller || !World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RubezhFindPickup), false, Pawn);
	World->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldDynamic), FCollisionShape::MakeSphere(Rubezh::Pickup::ReachCm), QueryParams);

	ARubezhWorldItem* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ARubezhWorldItem* Item = Cast<ARubezhWorldItem>(Overlap.GetActor());
		if (!Item || !IsValid(Item))
		{
			continue;
		}

		const TOptional<float> Score = Rubezh::Pickup::ScoreCandidate(ViewLocation, ViewRotation.Vector(),
			Pawn->GetActorLocation(), Item->GetFocusLocation());
		if (Score.IsSet() && Score.GetValue() < BestScore)
		{
			BestScore = Score.GetValue();
			Best = Item;
		}
	}
	return Best;
}

int32 URubezhInteractionComponent::CountFitting(const ARubezhWorldItem& Item) const
{
	const URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>());
	const FRubezhItemDefinition* Definition = Item.FindDefinition();
	if (!Inventory || !Definition)
	{
		return 0;
	}

	// Пробуем на копии рюкзака: настоящий меняет только сервер.
	FRubezhInventoryGrid Trial = Inventory->GetGrid();
	const int32 NotAdded = Trial.AddItem(Item.GetItemId(), *Definition, Item.GetCount(),
		[Inventory](FName Id) { return Inventory->FindDefinition(Id); });
	return Item.GetCount() - NotAdded;
}

void URubezhInteractionComponent::UpdatePrompt()
{
	APlayerController* Controller = GetController<APlayerController>();
	const ARubezhWorldItem* Item = FocusedItem.Get();
	const FRubezhItemDefinition* Definition = Item ? Item->FindDefinition() : nullptr;
	if (!Controller || !Definition)
	{
		if (Prompt)
		{
			Prompt->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!Prompt)
	{
		// Подсказка стоит под прицелом, а не над предметом: предметы лежат у ног, и там её закрывает интерфейс.
		Prompt = CreateWidget<URubezhPickupPrompt>(Controller);
		Prompt->SetAnchorsInViewport(FAnchors(0.5f, Rubezh::InventoryStyle::PromptScreenHeight));
		Prompt->SetAlignmentInViewport(FVector2D(0.5f, 0.0f));
		Prompt->AddToPlayerScreen(PromptZOrder);
	}

	Prompt->SetContent(Definition->DisplayName, Item->GetCount(), CountFitting(*Item));
	Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URubezhInteractionComponent::Interact()
{
	if (ARubezhWorldItem* Item = FocusedItem.Get())
	{
		ServerPickUp(Item);
	}
}

void URubezhInteractionComponent::ServerPickUp_Implementation(ARubezhWorldItem* Item)
{
	const APawn* Pawn = GetPawn<APawn>();
	URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(Pawn);
	if (!Pawn || !Inventory || !IsValid(Item))
	{
		return;
	}

	// Клиенту не верим: проверяем расстояние сами (с запасом на задержку сети).
	if (!Rubezh::Pickup::IsWithinReach(Pawn->GetActorLocation(), Item->GetFocusLocation(), true))
	{
		UE_LOG(LogRubezh, Warning, TEXT("%s пытается подобрать '%s' слишком далеко"), *GetNameSafe(Pawn), *Item->GetItemId().ToString());
		return;
	}

	const int32 NotAdded = Inventory->AddItem(Item->GetItemId(), Item->GetCount());
	Item->SetCount(NotAdded);
}
