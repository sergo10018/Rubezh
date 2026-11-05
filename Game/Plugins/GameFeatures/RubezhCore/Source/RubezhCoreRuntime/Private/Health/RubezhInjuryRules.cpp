// РУБЕЖ: Земля после разлома

#include "Health/RubezhInjuryRules.h"

#include "Items/RubezhItemDefinition.h"

namespace Rubezh::Injuries
{
	namespace
	{
		using FCandidates = TArray<TPair<FName, const FRubezhItemDefinition*>>;

		FName FindCure(const FCandidates& Available, ERubezhInjury Injury)
		{
			for (const TPair<FName, const FRubezhItemDefinition*>& Candidate : Available)
			{
				if (Candidate.Value && Candidate.Value->UseSeconds > 0.0f && Candidate.Value->Cures == Injury)
				{
					return Candidate.Key;
				}
			}
			return NAME_None;
		}

		FName FindPainRelief(const FCandidates& Available)
		{
			for (const TPair<FName, const FRubezhItemDefinition*>& Candidate : Available)
			{
				if (Candidate.Value && Candidate.Value->UseSeconds > 0.0f && Candidate.Value->PainReliefSeconds > 0.0f)
				{
					return Candidate.Key;
				}
			}
			return NAME_None;
		}

		/** Самая маленькая аптечка, которой хватает до полного здоровья; если не хватает ни одной — самая большая. */
		FName FindHealing(const FCandidates& Available, float Missing)
		{
			FName Enough = NAME_None;
			float EnoughAmount = TNumericLimits<float>::Max();
			FName Largest = NAME_None;
			float LargestAmount = 0.0f;

			for (const TPair<FName, const FRubezhItemDefinition*>& Candidate : Available)
			{
				const FRubezhItemDefinition* Item = Candidate.Value;
				if (!Item || Item->UseSeconds <= 0.0f || Item->HealAmount <= 0.0f)
				{
					continue;
				}
				if (Item->HealAmount >= Missing && Item->HealAmount < EnoughAmount)
				{
					Enough = Candidate.Key;
					EnoughAmount = Item->HealAmount;
				}
				if (Item->HealAmount > LargestAmount)
				{
					Largest = Candidate.Key;
					LargestAmount = Item->HealAmount;
				}
			}
			return Enough != NAME_None ? Enough : Largest;
		}
	}

	FMask RollHitInjuries(const FHitInfo& Hit, FRandomStream& Random)
	{
		if (Hit.Damage <= 0.0f)
		{
			return 0;
		}

		FMask Result = 0;
		if (Random.FRand() < BleedingChance)
		{
			Result |= Bit(ERubezhInjury::Bleeding);
		}
		if (Hit.Damage >= HeavyHitMinDamage && Random.FRand() < HeavyBleedingChance)
		{
			Result |= Bit(ERubezhInjury::HeavyBleeding);
		}
		if (Hit.bLegHit && Random.FRand() < LegFractureChance)
		{
			Result |= Bit(ERubezhInjury::LegFracture);
		}
		if (Result != 0)
		{
			Result |= Bit(ERubezhInjury::Pain);
		}
		return Result;
	}

	bool IsLegBone(FName BoneName)
	{
		const FString Name = BoneName.ToString();
		return Name.Contains(TEXT("thigh")) || Name.Contains(TEXT("calf")) || Name.Contains(TEXT("foot")) || Name.Contains(TEXT("ball"));
	}

	bool IsFallFracture(float FallHeightCm)
	{
		return FallHeightCm > FallFractureHeightCm;
	}

	float GetHealthLossPerSecond(FMask Injuries)
	{
		float Loss = 0.0f;
		if (Has(Injuries, ERubezhInjury::Bleeding))
		{
			Loss += BleedingHealthPerSecond;
		}
		if (Has(Injuries, ERubezhInjury::HeavyBleeding))
		{
			Loss += HeavyBleedingHealthPerSecond;
		}
		return Loss;
	}

	bool IsMedical(const FRubezhItemDefinition& Item)
	{
		return Item.UseSeconds > 0.0f && (Item.HealAmount > 0.0f || Item.Cures != ERubezhInjury::None || Item.PainReliefSeconds > 0.0f);
	}

	bool IsUseful(const FRubezhItemDefinition& Item, FMask Injuries, bool bPainRelieved, float Health, float MaxHealth)
	{
		if (!IsMedical(Item))
		{
			return false;
		}

		const bool bCuresSomething = Item.Cures != ERubezhInjury::None && Has(Injuries, Item.Cures);
		const bool bHealsSomething = Item.HealAmount > 0.0f && Health < MaxHealth;
		const bool bRelievesPain = Item.PainReliefSeconds > 0.0f && Has(Injuries, ERubezhInjury::Pain) && !bPainRelieved;
		return bCuresSomething || bHealsSomething || bRelievesPain;
	}

	FName ChooseQuickTreatment(const TArray<TPair<FName, const FRubezhItemDefinition*>>& Available,
		FMask Injuries, bool bPainRelieved, float Health, float MaxHealth)
	{
		for (const ERubezhInjury Injury : { ERubezhInjury::HeavyBleeding, ERubezhInjury::Bleeding, ERubezhInjury::LegFracture })
		{
			if (Has(Injuries, Injury))
			{
				if (const FName Cure = FindCure(Available, Injury); Cure != NAME_None)
				{
					return Cure;
				}
			}
		}

		if (Has(Injuries, ERubezhInjury::Pain) && !bPainRelieved)
		{
			if (const FName Relief = FindPainRelief(Available); Relief != NAME_None)
			{
				return Relief;
			}
		}

		return Health < MaxHealth ? FindHealing(Available, MaxHealth - Health) : NAME_None;
	}
}
