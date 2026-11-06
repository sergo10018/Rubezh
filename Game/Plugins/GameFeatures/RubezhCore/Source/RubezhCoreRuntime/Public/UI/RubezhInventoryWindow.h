// РУБЕЖ: Земля после разлома

#pragma once

#include "CommonActivatableWidget.h"
#include "Items/RubezhEquipSlot.h"
#include "UI/RubezhInventoryItemWidget.h"

#include "RubezhInventoryWindow.generated.h"

class UButton;
class UProgressBar;
class URubezhEquipmentSlotWidget;
class URubezhInventoryComponent;
class URubezhSlotGridWidget;
class URubezhTraderPanel;
class UTextBlock;
class UVerticalBox;
struct FRubezhItemDefinition;

/**
 * Окно рюкзака: слева снаряжение и суммарная защита, в середине ячейки рюкзака, вес и перегруз,
 * справа — что обыскиваем (ящик, тело, брошенный рюкзак), если обыск идёт, или торговец (URubezhTraderPanel), если идёт сделка.
 * Открывается на слое UI.Layer.GameMenu (курсор включается, игра не получает ввод), закрывается клавишей
 * инвентаря или Esc. Все изменения — запросы к серверу; окно перерисовывается, когда сервер прислал новое содержимое.
 * Отпущенное за окном выбрасывается на землю.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryWindow : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	URubezhInventoryWindow(const FObjectInitializer& ObjectInitializer);

	/** ПКМ по плитке: переложить между рюкзаком и обыскиваемым. */
	void QuickMove(ERubezhItemSource Source, int32 EntryIndex);

	/** Двойной клик по плитке: применить или надеть (рюкзак), забрать (обыск). */
	void Activate(ERubezhItemSource Source, int32 EntryIndex);

	/** Предмет отпущен над ячейками Target: в ячейку Cell (INDEX_NONE — куда влезет). */
	void HandleGridDrop(ERubezhItemSource Target, int32 Cell, const URubezhInventoryDragOperation& Operation);

	void RequestEquip(int32 EntryIndex);
	/** Снять в рюкзак: в ячейку TargetSlot или (не задана) куда влезет. */
	void RequestUnequip(ERubezhEquipSlot EquipSlot, TOptional<int32> TargetSlot);

	/** Взять оружие из рюкзака в слот (не задан — подходящий сам). */
	void RequestEquipWeapon(int32 EntryIndex, TOptional<int32> WeaponSlot);
	/** Убрать оружие из слота в ячейку Cell или (не задана) куда влезет. */
	void RequestUnequipWeapon(int32 WeaponSlot, TOptional<int32> Cell);

	bool IsLootOpen() const;
	void HideDropPreview();
	void ShowGroundHint(bool bShow);
	void RefreshNow() { Refresh(); }

	/** Описание предмета в стопке рюкзака. */
	const FRubezhItemDefinition* FindEntryDefinition(int32 EntryIndex) const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleTakeAllClicked();

	UFUNCTION()
	void HandleWearBagClicked();

	void BuildLayout();
	UWidget* BuildEquipmentColumn();
	UWidget* BuildBackpackColumn();
	UWidget* BuildLootColumn();
	UButton* MakeButton(const FString& Label);

	void RefreshWeight(const URubezhInventoryComponent& Inventory);
	void RefreshEquipment(const URubezhInventoryComponent& Inventory);
	void RefreshLoot(const URubezhInventoryComponent& Inventory);

	/** Положить стопку рюкзака в обыскиваемое / забрать стопку из обыскиваемого. */
	void PutIntoLoot(int32 EntryIndex, int32 TargetSlot);
	void TakeFromLoot(int32 LootEntryIndex, int32 TargetSlot);
	void RequestUse(int32 EntryIndex);
	/** Патроны отпущены на магазин: начать набивку. false — под курсором не подходящий магазин. */
	bool TryLoadMagazine(const URubezhInventoryDragOperation& Operation, int32 Cell);

	/** Показать причину отказа красным в строке подсказки на несколько секунд. */
	void ShowError(const FString& Message);
	void ClearError();
	bool bShowingError = false;
	FTimerHandle ErrorTimer;

	/** Курсор над окном? Отпущенное за окном выбрасывается на землю. */
	bool IsOverWindow(const FDragDropEvent& DragDropEvent) const;

	URubezhInventoryComponent* FindInventory() const;

	TWeakObjectPtr<URubezhInventoryComponent> BoundInventory;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintLine;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProtectionText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> WindowPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URubezhEquipmentSlotWidget>> EquipSlots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URubezhEquipmentSlotWidget>> WeaponSlotWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> WeightBar;

	UPROPERTY(Transient)
	TObjectPtr<URubezhSlotGridWidget> BackpackGrid;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> LootColumn;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LootTitle;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WearBagButton;

	UPROPERTY(Transient)
	TObjectPtr<URubezhSlotGridWidget> LootGrid;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY(Transient)
	TObjectPtr<URubezhTraderPanel> TraderPanel;
};
