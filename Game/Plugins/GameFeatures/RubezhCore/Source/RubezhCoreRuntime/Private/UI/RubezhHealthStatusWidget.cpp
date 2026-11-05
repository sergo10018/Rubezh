// РУБЕЖ: Земля после разлома

#include "UI/RubezhHealthStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "RubezhSettings.h"
#include "UI/RubezhInventoryStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhHealthStatusWidget)

using namespace Rubezh::Injuries;

namespace
{
	constexpr float TreatmentBarWidth = 220.0f;
	constexpr float TreatmentBarHeight = 6.0f;

	UTextBlock* AddCenteredText(UWidgetTree& Tree, UVerticalBox& Box, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Box.AddChildToVerticalBox(Text)->SetHorizontalAlignment(HAlign_Center);
		return Text;
	}

	FString DescribeInjuries(uint8 Injuries, bool bPainRelieved)
	{
		TArray<FString> Parts;
		if (Has(Injuries, ERubezhInjury::HeavyBleeding)) { Parts.Add(TEXT("Сильное кровотечение")); }
		if (Has(Injuries, ERubezhInjury::Bleeding))      { Parts.Add(TEXT("Кровотечение")); }
		if (Has(Injuries, ERubezhInjury::LegFracture))   { Parts.Add(TEXT("Перелом ноги")); }
		if (Has(Injuries, ERubezhInjury::Pain))          { Parts.Add(bPainRelieved ? TEXT("Боль (обезболено)") : TEXT("Боль")); }
		return FString::Join(Parts, TEXT(" · "));
	}
}

void URubezhHealthStatusWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace Rubezh::InventoryStyle;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(PromptBackground);
	Background->SetPadding(FMargin(PromptPadding));
	WidgetTree->RootWidget = Background;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->SetContent(Column);

	InjuryText = AddCenteredText(*WidgetTree, *Column, PromptNameFontSize, WarningText);

	UVerticalBox* Treatment = WidgetTree->ConstructWidget<UVerticalBox>();
	TreatmentRow = Treatment;
	Column->AddChildToVerticalBox(Treatment)->SetHorizontalAlignment(HAlign_Center);
	TreatmentText = AddCenteredText(*WidgetTree, *Treatment, PromptActionFontSize, FLinearColor::White);
	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>();
	BarBox->SetWidthOverride(TreatmentBarWidth);
	BarBox->SetHeightOverride(TreatmentBarHeight);
	TreatmentBar = WidgetTree->ConstructWidget<UProgressBar>();
	TreatmentBar->SetFillColorAndOpacity(GetEncumbranceColor(ERubezhEncumbranceLevel::Normal));
	BarBox->AddChild(TreatmentBar);
	Treatment->AddChildToVerticalBox(BarBox)->SetHorizontalAlignment(HAlign_Center);

	PendingHealText = AddCenteredText(*WidgetTree, *Column, PromptActionFontSize, GetEncumbranceColor(ERubezhEncumbranceLevel::Normal));
	HintLine = AddCenteredText(*WidgetTree, *Column, PromptActionFontSize, Rubezh::InventoryStyle::HintText);

	SetAnchorsInViewport(FAnchors(0.5f, HealthStatusScreenHeight));
	SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URubezhHealthStatusWidget::SetStatus(uint8 Injuries, bool bPainRelieved, const FText& TreatmentName, float TreatmentProgress, float PendingHealSeconds)
{
	const FString InjuryLine = DescribeInjuries(Injuries, bPainRelieved);
	InjuryText->SetText(FText::FromString(InjuryLine));
	InjuryText->SetVisibility(InjuryLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	const bool bTreating = !TreatmentName.IsEmpty();
	TreatmentRow->SetVisibility(bTreating ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bTreating)
	{
		TreatmentText->SetText(FText::FromString(FString::Printf(TEXT("Применяю: %s"), *TreatmentName.ToString())));
		TreatmentBar->SetPercent(TreatmentProgress);
	}

	PendingHealText->SetVisibility(PendingHealSeconds > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (PendingHealSeconds > 0.0f)
	{
		PendingHealText->SetText(FText::FromString(FString::Printf(TEXT("Аптечка действует: %d с"), FMath::CeilToInt(PendingHealSeconds))));
	}

	const FString Key = GetDefault<URubezhSettings>()->HealKey.GetDisplayName().ToString();
	HintLine->SetText(FText::FromString(FString::Printf(TEXT("%s — лечиться"), *Key)));
	HintLine->SetVisibility((bTreating || Injuries == 0) ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
