// РУБЕЖ: Земля после разлома

#include "Items/RubezhLootContainer.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhWorldItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhLootContainer)

namespace
{
	constexpr float ContainerCubeSizeCm = 100.0f;
	const FName ContainerColorParameter(TEXT("Color"));

	// Тело и брошенный рюкзак: плоская тёмная «сумка» на земле, сквозь неё можно пройти.
	const FVector BodyBoxSizeCm(70.0f, 45.0f, 18.0f);
	const FVector BagBoxSizeCm(45.0f, 35.0f, 25.0f);
	const FLinearColor BodyColor(0.05f, 0.02f, 0.02f);
	const FLinearColor BagColor(0.06f, 0.07f, 0.03f);
	constexpr float BodySearchSeconds = 3.0f;
	constexpr float BagSearchSeconds = 1.0f;
}

ARubezhLootContainer::ARubezhLootContainer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	SetReplicatingMovement(false);
	PrimaryActorTick.bCanEverTick = false;
	DisplayName = FText::FromString(TEXT("Ящик"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Точка актора — на земле, коробка стоит над ней.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetMaterial(0, ShapeMaterial.Object);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
}

ARubezhLootContainer* ARubezhLootContainer::SpawnDropped(UWorld& World, const FVector& Location, float Yaw, const FText& Name,
	const FRubezhInventoryGrid& Contents, FName BagItemId, float LifetimeSeconds)
{
	const FTransform Transform(FRotator(0.0f, Yaw, 0.0f), Location);
	ARubezhLootContainer* Container = World.SpawnActorDeferred<ARubezhLootContainer>(StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Container)
	{
		UE_LOG(LogRubezh, Error, TEXT("Не удалось положить на землю «%s»"), *Name.ToString());
		return nullptr;
	}

	const bool bBag = !BagItemId.IsNone();
	Container->DisplayName = Name;
	Container->BagItemId = BagItemId;
	Container->Contents = Contents;
	Container->SlotCount = FMath::Max(Contents.GetSlotCount(), 1);
	Container->SearchSeconds = bBag ? BagSearchSeconds : BodySearchSeconds;
	Container->bDestroyWhenEmpty = true;
	Container->bBlocksCharacters = false;
	Container->BoxSizeCm = bBag ? BagBoxSizeCm : BodyBoxSizeCm;
	Container->BoxColor = bBag ? BagColor : BodyColor;
	Container->FinishSpawning(Transform);
	if (LifetimeSeconds > 0.0f)
	{
		Container->SetLifeSpan(LifetimeSeconds);
	}
	return Container;
}

ARubezhLootContainer* ARubezhLootContainer::SpawnCrate(UWorld& World, const FTransform& Transform, FName LootTableName, int32 Slots)
{
	ARubezhLootContainer* Crate = World.SpawnActorDeferred<ARubezhLootContainer>(StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Crate)
	{
		Crate->LootTable = LootTableName;
		Crate->SlotCount = FMath::Clamp(Slots, 1, 48);
		Crate->FinishSpawning(Transform);
	}
	return Crate;
}

void ARubezhLootContainer::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && Contents.GetSlotCount() == 0)
	{
		Contents = FRubezhInventoryGrid(SlotCount);
		FillFromLootTable();
	}
}

void ARubezhLootContainer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Кто обыскивал — окно обыска закрывается.
	const TArray<TWeakObjectPtr<URubezhInventoryComponent>> CurrentViewers = MoveTemp(Viewers);
	for (const TWeakObjectPtr<URubezhInventoryComponent>& Viewer : CurrentViewers)
	{
		if (URubezhInventoryComponent* Inventory = Viewer.Get())
		{
			Inventory->CloseLoot();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ARubezhLootContainer::FillFromLootTable()
{
	if (LootTable.IsNone())
	{
		return;
	}

	const TArray<FRubezhLootTableRow> Rows = Rubezh::Items::GetLootRows();
	FRandomStream Random(FMath::Rand());
	for (const Rubezh::Loot::FDrop& Drop : Rubezh::Loot::Roll(Rows, LootTable, Random))
	{
		const FRubezhItemDefinition* Definition = Rubezh::Items::FindDefinition(Drop.ItemId);
		if (!Definition)
		{
			UE_LOG(LogRubezh, Warning, TEXT("%s: в таблице лута неизвестный предмет '%s'"), *GetName(), *Drop.ItemId.ToString());
			continue;
		}
		Contents.AddItem(Drop.ItemId, *Definition, Drop.Count);
	}
}

FVector ARubezhLootContainer::GetFocusLocation() const
{
	return Mesh->Bounds.Origin;
}

bool ARubezhLootContainer::ModifyContents(TFunctionRef<bool(FRubezhInventoryGrid&)> Change)
{
	if (!HasAuthority() || !Change(Contents))
	{
		return false;
	}

	NotifyViewers();
	if (bDestroyWhenEmpty && Contents.IsEmpty())
	{
		// Из пустого рюкзака всё вынули — сам рюкзак остаётся лежать, его можно подобрать.
		if (!BagItemId.IsNone())
		{
			ARubezhWorldItem::SpawnAt(*GetWorld(), GetActorTransform(), BagItemId, 1);
		}
		Destroy();
	}
	return true;
}

void ARubezhLootContainer::RemoveAsWornBag()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void ARubezhLootContainer::NotifyViewers()
{
	Viewers.RemoveAll([](const TWeakObjectPtr<URubezhInventoryComponent>& Viewer) { return !Viewer.IsValid(); });
	for (const TWeakObjectPtr<URubezhInventoryComponent>& Viewer : Viewers)
	{
		Viewer->RefreshLootView();
	}
}

void ARubezhLootContainer::AddViewer(URubezhInventoryComponent& Viewer)
{
	Viewers.AddUnique(&Viewer);
}

void ARubezhLootContainer::RemoveViewer(const URubezhInventoryComponent& Viewer)
{
	Viewers.RemoveAll([&Viewer](const TWeakObjectPtr<URubezhInventoryComponent>& Item) { return Item.Get() == &Viewer; });
}

bool ARubezhLootContainer::WasSearchedBy(const URubezhInventoryComponent& Searcher) const
{
	return SearchedBy.ContainsByPredicate([&Searcher](const TWeakObjectPtr<URubezhInventoryComponent>& Item) { return Item.Get() == &Searcher; });
}

void ARubezhLootContainer::MarkSearchedBy(URubezhInventoryComponent& Searcher)
{
	SearchedBy.AddUnique(&Searcher);
}

void ARubezhLootContainer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisual();
}

void ARubezhLootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARubezhLootContainer, DisplayName);
	DOREPLIFETIME(ARubezhLootContainer, SearchSeconds);
	DOREPLIFETIME(ARubezhLootContainer, BoxSizeCm);
	DOREPLIFETIME(ARubezhLootContainer, BoxColor);
	DOREPLIFETIME(ARubezhLootContainer, bBlocksCharacters);
	DOREPLIFETIME(ARubezhLootContainer, BagItemId);
}

void ARubezhLootContainer::OnRep_Visual()
{
	UpdateVisual();
}

void ARubezhLootContainer::UpdateVisual()
{
	// Куб движка — 100 см с центром посередине; поднимаем на полвысоты, чтобы он стоял на земле.
	Mesh->SetRelativeScale3D(BoxSizeCm / ContainerCubeSizeCm);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoxSizeCm.Z * 0.5f));
	Mesh->SetCollisionEnabled(bBlocksCharacters ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, bBlocksCharacters ? ECR_Block : ECR_Ignore);
	Mesh->SetCanEverAffectNavigation(bBlocksCharacters);

	if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(ContainerColorParameter, BoxColor);
	}
}
