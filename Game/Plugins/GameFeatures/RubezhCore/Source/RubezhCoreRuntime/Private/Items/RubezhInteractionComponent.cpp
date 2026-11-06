// РУБЕЖ: Земля после разлома

#include "Items/RubezhInteractionComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhLootContainer.h"
#include "Items/RubezhPickupRules.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhPickupPrompt.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInteractionComponent)

namespace
{
	// Подсказка под окнами интерфейса Lyra.
	constexpr int32 PromptZOrder = -10;
	constexpr double NoticeSeconds = 1.5;

	FVector GetFocusLocation(const AActor& Actor)
	{
		if (const ARubezhWorldItem* Item = Cast<ARubezhWorldItem>(&Actor))
		{
			return Item->GetFocusLocation();
		}
		return CastChecked<ARubezhLootContainer>(&Actor)->GetFocusLocation();
	}
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
	FocusedActor.Reset();

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

	FocusedActor = FindFocusedActor();
	UpdatePrompt();
}

AActor* URubezhInteractionComponent::FindFocusedActor() const
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

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		const bool bInteractive = Cast<ARubezhWorldItem>(Candidate) || Cast<ARubezhLootContainer>(Candidate);
		if (!bInteractive || !IsValid(Candidate))
		{
			continue;
		}

		const TOptional<float> Score = Rubezh::Pickup::ScoreCandidate(ViewLocation, ViewRotation.Vector(),
			Pawn->GetActorLocation(), GetFocusLocation(*Candidate));
		if (Score.IsSet() && Score.GetValue() < BestScore)
		{
			BestScore = Score.GetValue();
			Best = Candidate;
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
	return Item.GetCount() - Trial.AddItem(Item.GetItemId(), *Definition, Item.GetCount());
}

FString URubezhInteractionComponent::DescribeContainerAction(const ARubezhLootContainer& Container) const
{
	const URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>());
	if (Inventory && Inventory->IsSearching(&Container))
	{
		return FString::Printf(TEXT("Обыск… %.1f с"), Inventory->GetSearchSecondsLeft());
	}

	const FString Key = GetDefault<URubezhSettings>()->InteractKey.GetDisplayName().ToString();
	return FString::Printf(TEXT("%s — обыскать"), *Key);
}

void URubezhInteractionComponent::ShowNotice(const FString& Text)
{
	Notice = Text;
	NoticeEndTime = GetWorld()->GetTimeSeconds() + NoticeSeconds;
}

void URubezhInteractionComponent::UpdatePrompt()
{
	if (GetWorld()->GetTimeSeconds() < NoticeEndTime)
	{
		ShowPrompt(FText::GetEmpty(), Notice, true);
		return;
	}

	const AActor* Focused = FocusedActor.Get();
	if (const ARubezhLootContainer* Container = Cast<ARubezhLootContainer>(Focused))
	{
		ShowPrompt(Container->GetDisplayName(), DescribeContainerAction(*Container), false);
		return;
	}

	const ARubezhWorldItem* Item = Cast<ARubezhWorldItem>(Focused);
	const FRubezhItemDefinition* Definition = Item ? Item->FindDefinition() : nullptr;
	if (!Definition)
	{
		HidePrompt();
		return;
	}

	const int32 Count = Item->GetCount();
	const int32 Fitting = CountFitting(*Item);
	const FString Name = Count > 1 ? FString::Printf(TEXT("%s ×%d"), *Definition->DisplayName.ToString(), Count) : Definition->DisplayName.ToString();
	const FString Key = GetDefault<URubezhSettings>()->InteractKey.GetDisplayName().ToString();
	const FString Action = Fitting <= 0 ? FString(TEXT("Нет места в рюкзаке"))
		: Fitting < Count ? FString::Printf(TEXT("%s — подобрать %d (больше не влезет)"), *Key, Fitting)
		: FString::Printf(TEXT("%s — подобрать"), *Key);
	ShowPrompt(FText::FromString(Name), Action, Fitting <= 0);
}

void URubezhInteractionComponent::HidePrompt()
{
	if (Prompt)
	{
		Prompt->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URubezhInteractionComponent::ShowPrompt(const FText& Name, const FString& Action, bool bWarning)
{
	APlayerController* Controller = GetController<APlayerController>();
	if (!Controller)
	{
		HidePrompt();
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

	Prompt->SetMessage(Name, Action, bWarning);
	Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URubezhInteractionComponent::Interact()
{
	AActor* Focused = FocusedActor.Get();
	if (ARubezhWorldItem* Item = Cast<ARubezhWorldItem>(Focused))
	{
		ServerPickUp(Item);
	}
	else if (ARubezhLootContainer* Container = Cast<ARubezhLootContainer>(Focused))
	{
		if (URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(GetPawn<APawn>()))
		{
			Inventory->ServerStartSearch(Container);
		}
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

	Item->SetCount(Inventory->AddEntry(Item->MakeEntry()));
}
