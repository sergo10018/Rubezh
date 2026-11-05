// РУБЕЖ: Земля после разлома

#include "UI/RubezhPickupPrompt.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/RubezhInventoryStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhPickupPrompt)

namespace
{
	UTextBlock* MakeCenteredText(UWidgetTree& Tree, UVerticalBox& Box, int32 FontSize)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* TextSlot = Box.AddChildToVerticalBox(Text);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		return Text;
	}
}

void URubezhPickupPrompt::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace Rubezh::InventoryStyle;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(PromptBackground);
	Background->SetPadding(FMargin(PromptPadding));
	WidgetTree->RootWidget = Background;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->SetContent(Column);
	NameText = MakeCenteredText(*WidgetTree, *Column, PromptNameFontSize);
	ActionText = MakeCenteredText(*WidgetTree, *Column, PromptActionFontSize);

	// Подсказка не должна перехватывать мышь.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URubezhPickupPrompt::SetMessage(const FText& Name, const FString& Action, bool bWarning)
{
	using namespace Rubezh::InventoryStyle;

	NameText->SetText(Name);
	ActionText->SetText(FText::FromString(Action));
	ActionText->SetColorAndOpacity(FSlateColor(bWarning ? WarningText : HintText));
}
