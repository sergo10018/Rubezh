// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"
#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhItemDefinition.h"

#include "RubezhEquipmentSlotWidget.generated.h"

class UBorder;
class URubezhInventoryWindow;
class UTextBlock;

/**
 * Слот снаряжения или оружия в окне рюкзака: название слота и предмет в нём.
 * Перетащить сюда предмет из рюкзака — надеть; перетащить отсюда в рюкзак — снять, за окно — бросить
 * (рюкзак падает вместе с вещами); двойной клик — снять в рюкзак. Собран кодом, без ассета виджета.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(URubezhInventoryWindow* InWindow, ERubezhEquipSlot InEquipSlot);

	/** Сделать слотом оружия: 0 и 1 — основное, 2 — пистолет. */
	void SetupWeapon(URubezhInventoryWindow* InWindow, int32 InWeaponSlot);

	/** Показать оружие в слоте (Item == nullptr — слот пуст). */
	void SetWeapon(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition* Item);

	int32 GetWeaponSlot() const { return WeaponSlot; }

	/** Показать надетое (Item == nullptr — слот пуст). */
	void SetItem(FName InItemId, const FRubezhItemDefinition* Item, float Durability = 100.0f);

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
	/** Подходит ли перетаскиваемый из рюкзака предмет в этот слот. */
	bool AcceptsDrag(const UDragDropOperation* Operation) const;
	void ResetHighlight();

	TWeakObjectPtr<URubezhInventoryWindow> Window;
	ERubezhEquipSlot EquipSlot = ERubezhEquipSlot::None;
	int32 WeaponSlot = INDEX_NONE;
	FRubezhInventoryEntry WeaponEntry;
	FName ItemId;
	FRubezhItemDefinition Definition;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlotText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemText;
};
