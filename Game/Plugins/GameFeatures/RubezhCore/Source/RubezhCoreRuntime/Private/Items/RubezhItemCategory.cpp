// РУБЕЖ: Земля после разлома

#include "Items/RubezhItemCategory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhItemCategory)

namespace Rubezh::Items
{
	bool IsLostOnDeath(ERubezhItemCategory Category)
	{
		switch (Category)
		{
		case ERubezhItemCategory::Equipment:
			return false;
		case ERubezhItemCategory::Consumable:
		case ERubezhItemCategory::Junk:
		case ERubezhItemCategory::Valuable:
			return true;
		}
		return true;
	}
}
