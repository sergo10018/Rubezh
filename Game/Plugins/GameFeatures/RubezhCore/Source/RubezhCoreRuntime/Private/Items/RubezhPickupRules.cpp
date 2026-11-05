// РУБЕЖ: Земля после разлома

#include "Items/RubezhPickupRules.h"

namespace Rubezh::Pickup
{
	bool IsWithinReach(const FVector& PawnLocation, const FVector& ItemLocation, bool bServerCheck)
	{
		const float Reach = ReachCm + (bServerCheck ? ServerReachToleranceCm : 0.0f);
		return FVector::DistSquared(PawnLocation, ItemLocation) <= FMath::Square(Reach);
	}

	TOptional<float> ScoreCandidate(const FVector& ViewLocation, const FVector& ViewDirection,
		const FVector& PawnLocation, const FVector& ItemLocation)
	{
		if (!IsWithinReach(PawnLocation, ItemLocation, false))
		{
			return {};
		}

		const FVector ToItem = (ItemLocation - ViewLocation).GetSafeNormal();
		const FVector Forward = ViewDirection.GetSafeNormal();
		if (ToItem.IsNearlyZero() || Forward.IsNearlyZero())
		{
			return {};
		}

		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToItem, Forward), -1.0f, 1.0f)));
		if (AngleDeg > MaxViewAngleDeg)
		{
			return {};
		}
		return AngleDeg;
	}
}
