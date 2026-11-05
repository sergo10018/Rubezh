// РУБЕЖ: Земля после разлома

#include "Items/RubezhWorldItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Items/RubezhItemDefinition.h"
#include "Items/RubezhPickupRules.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhWorldItem)

namespace
{
	// Временный вид: одна клетка сетки рюкзака = 12 см коробки.
	constexpr float CellSizeCm = 12.0f;
	constexpr float BoxHeightCm = 8.0f;
	constexpr float EngineCubeSizeCm = 100.0f;

	// Насколько выше и ниже точки перед персонажем искать землю, когда предмет выбрасывают.
	constexpr float GroundSearchUpCm = 60.0f;
	constexpr float GroundSearchDownCm = 400.0f;
	constexpr float DropScatterCm = 20.0f;

	const FName ColorParameter(TEXT("Color"));

	FLinearColor GetWorldItemColor(ERubezhItemCategory Category)
	{
		switch (Category)
		{
		case ERubezhItemCategory::Equipment:  return FLinearColor(0.12f, 0.16f, 0.07f);
		case ERubezhItemCategory::Consumable: return FLinearColor(0.05f, 0.18f, 0.25f);
		case ERubezhItemCategory::Valuable:   return FLinearColor(0.45f, 0.30f, 0.05f);
		case ERubezhItemCategory::Junk:
		default:                              return FLinearColor(0.25f, 0.12f, 0.07f);
		}
	}
}

ARubezhWorldItem::ARubezhWorldItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	SetReplicatingMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Точка актора — на земле; коробка над ней, поэтому корень отдельный.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetMaterial(0, ShapeMaterial.Object);
	// Предмет виден трассировкам и пулям, но персонажи о него не спотыкаются, а камера сквозь него проходит.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
}

ARubezhWorldItem* ARubezhWorldItem::SpawnInFrontOf(APawn& Pawn, FName ItemId, int32 Count)
{
	UWorld* World = Pawn.GetWorld();
	if (!World || !Pawn.HasAuthority() || Count <= 0)
	{
		return nullptr;
	}

	// Небольшой разброс, чтобы несколько выброшенных вещей не легли в одну точку.
	const FVector Scatter(FMath::FRandRange(-DropScatterCm, DropScatterCm), FMath::FRandRange(-DropScatterCm, DropScatterCm), 0.0f);
	const FVector Forward = Pawn.GetActorForwardVector().GetSafeNormal2D();
	const FVector Above = Pawn.GetActorLocation() + Forward * Rubezh::Pickup::DropForwardCm + Scatter;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RubezhDropItem), false, &Pawn);
	FHitResult Ground;
	const bool bFoundGround = World->LineTraceSingleByChannel(Ground,
		Above + FVector(0.0f, 0.0f, GroundSearchUpCm), Above - FVector(0.0f, 0.0f, GroundSearchDownCm), ECC_Visibility, QueryParams);
	const FVector Location = bFoundGround ? Ground.ImpactPoint : Above;

	const FTransform Transform(FRotator(0.0f, Pawn.GetActorRotation().Yaw, 0.0f), Location);
	ARubezhWorldItem* Item = World->SpawnActorDeferred<ARubezhWorldItem>(StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Item)
	{
		UE_LOG(LogRubezh, Error, TEXT("Не удалось положить на землю '%s' ×%d"), *ItemId.ToString(), Count);
		return nullptr;
	}

	Item->ItemId = ItemId;
	Item->Count = Count;
	Item->FinishSpawning(Transform);
	return Item;
}

void ARubezhWorldItem::SetCount(int32 NewCount)
{
	if (!HasAuthority())
	{
		return;
	}
	if (NewCount <= 0)
	{
		Destroy();
		return;
	}

	Count = NewCount;
	UpdateVisual();
}

FVector ARubezhWorldItem::GetFocusLocation() const
{
	return Mesh->Bounds.Origin;
}

const FRubezhItemDefinition* ARubezhWorldItem::FindDefinition() const
{
	const UDataTable* Table = GetDefault<URubezhSettings>()->ItemTable.LoadSynchronous();
	return Table ? Table->FindRow<FRubezhItemDefinition>(ItemId, TEXT("RubezhWorldItem"), false) : nullptr;
}

void ARubezhWorldItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisual();
}

void ARubezhWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARubezhWorldItem, ItemId);
	DOREPLIFETIME(ARubezhWorldItem, Count);
}

void ARubezhWorldItem::OnRep_Item()
{
	UpdateVisual();
}

void ARubezhWorldItem::UpdateVisual()
{
	const FRubezhItemDefinition* Definition = FindDefinition();
	const FIntPoint Footprint = Definition ? Definition->GetFootprint(false) : FIntPoint(1, 1);

	// Куб движка — 100 см с центром посередине; поднимаем его на полвысоты, чтобы он лежал на земле, а не тонул в ней.
	const FVector BoxSize(Footprint.X * CellSizeCm, Footprint.Y * CellSizeCm, BoxHeightCm);
	Mesh->SetRelativeScale3D(BoxSize / EngineCubeSizeCm);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoxHeightCm * 0.5f));

	if (!Definition)
	{
		return;
	}
	UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0);
	if (Material)
	{
		Material->SetVectorParameterValue(ColorParameter, GetWorldItemColor(Definition->Category));
	}
}
