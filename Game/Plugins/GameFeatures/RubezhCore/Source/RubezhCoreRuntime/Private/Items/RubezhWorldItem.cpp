// РУБЕЖ: Земля после разлома

#include "Items/RubezhWorldItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhItemDefinition.h"
#include "Items/RubezhPickupRules.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhWorldItem)

namespace
{
	// Временный вид: коробка тем больше, чем тяжелее предмет (1 кг — около 22 см в длину).
	constexpr float BoxEdgePerCubeRootKgCm = 22.0f;
	constexpr float MinBoxEdgeCm = 8.0f;
	constexpr float MaxBoxEdgeCm = 60.0f;
	constexpr float BoxWidthRatio = 0.6f;
	constexpr float BoxHeightRatio = 0.4f;
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

ARubezhWorldItem* ARubezhWorldItem::SpawnEntryInFrontOf(APawn& Pawn, const FRubezhInventoryEntry& Entry)
{
	ARubezhWorldItem* Item = SpawnInFrontOf(Pawn, Entry.ItemId, Entry.Count);
	if (Item)
	{
		Item->Rounds = Entry.Rounds;
		Item->Durability = Entry.Durability;
		Item->bHasMagazine = Entry.bHasMagazine;
		Item->bNextShotMisfires = Entry.bNextShotMisfires;
	}
	return Item;
}

FRubezhInventoryEntry ARubezhWorldItem::MakeEntry() const
{
	FRubezhInventoryEntry Entry;
	Entry.ItemId = ItemId;
	Entry.Count = Count;
	Entry.Rounds = Rounds;
	Entry.Durability = Durability;
	Entry.bHasMagazine = bHasMagazine;
	Entry.bNextShotMisfires = bNextShotMisfires;
	return Entry;
}

FVector ARubezhWorldItem::FindGround(UWorld& World, const FVector& Point, const AActor* Ignore)
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RubezhFindGround), false, Ignore);
	FHitResult Ground;
	const bool bFoundGround = World.LineTraceSingleByChannel(Ground,
		Point + FVector(0.0f, 0.0f, GroundSearchUpCm), Point - FVector(0.0f, 0.0f, GroundSearchDownCm), ECC_Visibility, QueryParams);
	return bFoundGround ? Ground.ImpactPoint : Point;
}

FVector ARubezhWorldItem::FindDropLocation(const APawn& Pawn)
{
	// Небольшой разброс, чтобы несколько выброшенных вещей не легли в одну точку.
	const FVector Scatter(FMath::FRandRange(-DropScatterCm, DropScatterCm), FMath::FRandRange(-DropScatterCm, DropScatterCm), 0.0f);
	const FVector Forward = Pawn.GetActorForwardVector().GetSafeNormal2D();
	const FVector Above = Pawn.GetActorLocation() + Forward * Rubezh::Pickup::DropForwardCm + Scatter;
	return FindGround(*Pawn.GetWorld(), Above, &Pawn);
}

ARubezhWorldItem* ARubezhWorldItem::SpawnInFrontOf(APawn& Pawn, FName ItemId, int32 Count)
{
	UWorld* World = Pawn.GetWorld();
	if (!World || !Pawn.HasAuthority())
	{
		return nullptr;
	}
	return SpawnAt(*World, FTransform(FRotator(0.0f, Pawn.GetActorRotation().Yaw, 0.0f), FindDropLocation(Pawn)), ItemId, Count);
}

ARubezhWorldItem* ARubezhWorldItem::SpawnAt(UWorld& World, const FTransform& Transform, FName ItemId, int32 Count)
{
	if (World.GetNetMode() == NM_Client || Count <= 0)
	{
		return nullptr;
	}

	ARubezhWorldItem* Item = World.SpawnActorDeferred<ARubezhWorldItem>(StaticClass(), Transform, nullptr, nullptr,
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
	return Rubezh::Items::FindDefinition(ItemId);
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
	const float WeightKg = Definition ? Definition->WeightKg : 0.0f;
	const float Edge = FMath::Clamp(BoxEdgePerCubeRootKgCm * FMath::Pow(FMath::Max(WeightKg, 0.0f), 1.0f / 3.0f), MinBoxEdgeCm, MaxBoxEdgeCm);

	// Куб движка — 100 см с центром посередине; поднимаем его на полвысоты, чтобы он лежал на земле, а не тонул в ней.
	const FVector BoxSize(Edge, FMath::Max(Edge * BoxWidthRatio, MinBoxEdgeCm), FMath::Max(Edge * BoxHeightRatio, MinBoxEdgeCm * 0.5f));
	Mesh->SetRelativeScale3D(BoxSize / EngineCubeSizeCm);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoxSize.Z * 0.5f));

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
