// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhInventoryItemWidget.generated.h"

class UBorder;
class URubezhInventoryWindow;
class USizeBox;
class UTextBlock;

/** Что перетаскиваем: какую стопку и за какую точку её взяли (в пикселях от левого верхнего угла). */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	int32 EntryIndex = INDEX_NONE;
	bool bRotated = false;
	FIntPoint Footprint = FIntPoint(1, 1);
	FVector2D GrabOffset = FVector2D::ZeroVector;
};

/**
 * Плитка предмета в сетке: цветной прямоугольник с названием и количеством.
 * ЛКМ — перетащить, ПКМ — повернуть. Собирается кодом, без ассета виджета.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Window == nullptr — плитка только для показа (картинка под курсором при перетаскивании). */
	void Setup(URubezhInventoryWindow* InWindow, int32 InEntryIndex, const FRubezhInventoryEntry& InEntry, const FRubezhItemDefinition& InDefinition);

	FIntPoint GetFootprint() const { return Definition.GetFootprint(Entry.bRotated); }

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	FText BuildTooltip() const;

	TWeakObjectPtr<URubezhInventoryWindow> Window;
	int32 EntryIndex = INDEX_NONE;
	FRubezhInventoryEntry Entry;
	FRubezhItemDefinition Definition;
	FVector2D GrabOffset = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountText;
};
