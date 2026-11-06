// РУБЕЖ: Земля после разлома
// Оформление окна инвентаря: размеры и цвета в одном месте. Временный вид прототипа — без текстур и иконок.

#pragma once

#include "Items/RubezhItemCategory.h"
#include "Math/Color.h"
#include "Survival/RubezhEncumbrance.h"

namespace Rubezh::InventoryStyle
{
	inline constexpr float CellSize = 72.0f;
	inline constexpr int32 SlotColumns = 6;	// ячеек в ряду рюкзака и ящика
	inline constexpr float CellGap = 2.0f;
	inline constexpr float WindowPadding = 20.0f;
	inline constexpr int32 TitleFontSize = 22;
	inline constexpr int32 TextFontSize = 13;
	inline constexpr int32 ItemNameFontSize = 10;
	inline constexpr int32 ItemCountFontSize = 12;
	inline constexpr float DraggedItemOpacity = 0.35f;

	// Колонка снаряжения слева от сетки.
	inline constexpr float EquipSlotWidth = 270.0f;
	inline constexpr float EquipSlotHeight = 58.0f;
	inline constexpr float EquipSlotGap = 6.0f;
	inline constexpr float ColumnGap = 20.0f;
	inline constexpr float ErrorShowSeconds = 3.0f;

	// Цвета линейные: на экране они светлее, чем кажутся по числам.
	inline const FLinearColor ScreenDim(0.0f, 0.0f, 0.0f, 0.55f);
	inline const FLinearColor WindowBackground(0.012f, 0.012f, 0.01f, 0.97f);
	inline const FLinearColor GridBackground(0.003f, 0.003f, 0.003f, 1.0f);
	inline const FLinearColor EmptyCell(0.022f, 0.022f, 0.02f, 1.0f);
	inline const FLinearColor DropAllowed(0.2f, 0.8f, 0.25f, 0.35f);
	inline const FLinearColor DropBlocked(0.9f, 0.15f, 0.1f, 0.35f);
	inline const FLinearColor HintText(0.6f, 0.6f, 0.55f, 1.0f);
	inline const FLinearColor WarningText(0.9f, 0.3f, 0.2f, 1.0f);
	inline const FLinearColor ButtonColor(0.12f, 0.12f, 0.1f, 1.0f);

	// Подсказка подбора предмета с земли.
	inline constexpr int32 PromptNameFontSize = 14;
	inline constexpr int32 PromptActionFontSize = 12;
	inline constexpr float PromptPadding = 8.0f;
	inline constexpr float PromptScreenHeight = 0.56f;	// доля высоты экрана: чуть ниже прицела
	inline const FLinearColor PromptBackground(0.0f, 0.0f, 0.0f, 0.6f);

	// Строка травм и лечения — над полоской здоровья Lyra (доля высоты экрана, нижний край).
	inline constexpr float HealthStatusScreenHeight = 0.86f;

	inline FLinearColor GetCategoryColor(ERubezhItemCategory Category)
	{
		switch (Category)
		{
		case ERubezhItemCategory::Equipment:  return FLinearColor(0.045f, 0.06f, 0.03f, 1.0f);  // хаки
		case ERubezhItemCategory::Consumable: return FLinearColor(0.02f, 0.05f, 0.065f, 1.0f); // сине-зелёный
		case ERubezhItemCategory::Valuable:   return FLinearColor(0.10f, 0.07f, 0.015f, 1.0f); // латунь
		case ERubezhItemCategory::Junk:
		default:                              return FLinearColor(0.06f, 0.035f, 0.025f, 1.0f); // ржавчина
		}
	}

	inline FLinearColor GetEncumbranceColor(ERubezhEncumbranceLevel Level)
	{
		switch (Level)
		{
		case ERubezhEncumbranceLevel::Normal:     return FLinearColor(0.55f, 0.65f, 0.45f, 1.0f);
		case ERubezhEncumbranceLevel::Burdened:   return FLinearColor(0.85f, 0.75f, 0.25f, 1.0f);
		case ERubezhEncumbranceLevel::Overloaded: return FLinearColor(0.9f, 0.45f, 0.15f, 1.0f);
		case ERubezhEncumbranceLevel::Immobile:
		default:                                  return FLinearColor(0.85f, 0.15f, 0.1f, 1.0f);
		}
	}

	inline const TCHAR* GetEncumbranceName(ERubezhEncumbranceLevel Level)
	{
		switch (Level)
		{
		case ERubezhEncumbranceLevel::Normal:     return TEXT("норма");
		case ERubezhEncumbranceLevel::Burdened:   return TEXT("нагружен");
		case ERubezhEncumbranceLevel::Overloaded: return TEXT("перегруз — бег невозможен");
		case ERubezhEncumbranceLevel::Immobile:
		default:                                  return TEXT("неподъёмно — не сдвинуться");
		}
	}
}
