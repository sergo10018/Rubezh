// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"
#include "Items/RubezhItemDefinition.h"

#include "RubezhEquipmentSlotWidget.generated.h"

class UBorder;
class URubezhInventoryWindow;
class UTextBlock;

/**
 * Слот снаряжения в окне рюкзака: название слота и надетый предмет.
 * Перетащить сюда предмет из сетки — надеть; перетащить отсюда в сетку — снять, за окно — бросить;
 * двойной клик — снять в сетку. Собран кодом, без ассета виджета.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(URubezhInventoryWindow* InWindow, ERubezhEquipSlot InEquipSlot);

	/** Показать надетое (Item == nullptr — слот пуст). */
	void SetItem(FName InItemId, const FRubezhItemDefinition* Item);

	ERubezhEquipSlot GetEquipSlot() const { return EquipSlot; }

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	/** Подходит ли перетаскиваемый из сетки предмет в этот слот. */
	bool AcceptsDrag(const UDragDropOperation* Operation) const;
	void ResetHighlight();

	TWeakObjectPtr<URubezhInventoryWindow> Window;
	ERubezhEquipSlot EquipSlot = ERubezhEquipSlot::None;
	FName ItemId;
	FRubezhItemDefinition Definition;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlotText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemText;
};
