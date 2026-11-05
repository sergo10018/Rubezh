// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Items/RubezhEquipSlot.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhInventoryItemWidget.generated.h"

class UBorder;
class URubezhInventoryWindow;
class USizeBox;
class UTextBlock;

/** Откуда предмет в окне: свой рюкзак, обыскиваемый ящик (тело, брошенный рюкзак) или слот снаряжения. */
enum class ERubezhItemSource : uint8
{
	Backpack,
	Loot,
	Equipment
};

/** Что перетаскиваем: стопку (Source + EntryIndex) или надетый предмет (FromSlot). */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	ERubezhItemSource Source = ERubezhItemSource::Backpack;
	int32 EntryIndex = INDEX_NONE;
	FName ItemId;
	ERubezhEquipSlot FromSlot = ERubezhEquipSlot::None;
};

/**
 * Плитка предмета: одна ячейка с названием и количеством. ЛКМ — перетащить; ПКМ — переложить между рюкзаком
 * и обыскиваемым; двойной клик — применить или надеть (в рюкзаке), забрать (при обыске). Собирается кодом, без ассета.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Window == nullptr — плитка только для показа (картинка под курсором при перетаскивании). */
	void Setup(URubezhInventoryWindow* InWindow, ERubezhItemSource InSource, int32 InEntryIndex,
		const FRubezhInventoryEntry& InEntry, const FRubezhItemDefinition& InDefinition);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	FText BuildTooltip() const;

	TWeakObjectPtr<URubezhInventoryWindow> Window;
	ERubezhItemSource Source = ERubezhItemSource::Backpack;
	int32 EntryIndex = INDEX_NONE;
	FRubezhInventoryEntry Entry;
	FRubezhItemDefinition Definition;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountText;
};
