// РУБЕЖ: Земля после разлома

#include "Settlement/RubezhTrader.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Items/RubezhTradeRules.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhTrader)

namespace
{
	// Цилиндр движка — 100 см в диаметре и в высоту, центр посередине.
	const FVector TraderScale(0.7f, 0.7f, 1.8f);
	constexpr float TraderHeightCm = 180.0f;
	const FName TraderColorParameter(TEXT("Color"));

	FLinearColor GetTraderColor(ERubezhTraderKind Kind)
	{
		switch (Kind)
		{
		case ERubezhTraderKind::Gunsmith:  return FLinearColor(0.35f, 0.12f, 0.05f);
		case ERubezhTraderKind::Medic:     return FLinearColor(0.7f, 0.7f, 0.7f);
		case ERubezhTraderKind::Armorer:   return FLinearColor(0.1f, 0.2f, 0.06f);
		case ERubezhTraderKind::Scavenger: return FLinearColor(0.5f, 0.35f, 0.05f);
		default:                           return FLinearColor::Gray;
		}
	}
}

ARubezhTrader::ARubezhTrader(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	SetReplicatingMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Точка актора — на земле, столб стоит над ней.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(CylinderMesh.Object);
	Mesh->SetMaterial(0, ShapeMaterial.Object);
	Mesh->SetRelativeScale3D(TraderScale);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, TraderHeightCm * 0.5f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
}

FText ARubezhTrader::GetDisplayName() const
{
	return FText::FromString(Rubezh::Trade::GetTraderName(Kind));
}

FVector ARubezhTrader::GetFocusLocation() const
{
	return Mesh->Bounds.Origin;
}

void ARubezhTrader::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisual();
}

void ARubezhTrader::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARubezhTrader, Kind);
}

void ARubezhTrader::OnRep_Kind()
{
	UpdateVisual();
}

void ARubezhTrader::UpdateVisual()
{
	if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TraderColorParameter, GetTraderColor(Kind));
	}
}
