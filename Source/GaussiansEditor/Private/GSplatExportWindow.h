#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "GSplatAsset.h"
#include "GSplatExporter.h"
#include "SPrimaryButton.h"


/**
 * A row of toggle buttons used to pick one of several mutually exclusive export options.
 * Mirrors the styling of SGSplatSwizzleSelectionWidget in the import window.
 */
class SGSplatOptionGroup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGSplatOptionGroup)
		: _Options({})
		, _SelectedIndex(0)
	{}
		SLATE_ARGUMENT(TArray<FText>, Options)
		SLATE_ARGUMENT(int32, SelectedIndex)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	int32 GetSelectedIndex() const { return SelectedIndex; }

	// Called with the new index whenever the selection changes
	TFunction<void(int32)> OnOptionChanged;

private:
	FReply OnOptionClicked(int32 Index);
	void RefreshStyles();

	TSharedPtr<SHorizontalBox> Container;
	TArray<TSharedPtr<SButton>> OptionButtons;
	int32 SelectedIndex = 0;
};


/**
 * Modal option window shown before exporting a GSplat asset to a PLY file.
 * Mirrors SGSplatImportWindow.
 */
class SGSplatExportWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGSplatExportWindow) {}
		SLATE_ARGUMENT(UGSplatAsset*, Asset)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(float, MaxWindowHeight)
		SLATE_ARGUMENT(float, MaxWindowWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FReply OnExport();
	FReply OnCancel();
	bool ShouldExport() const { return bShouldExport; }

	// Collects the options selected in this window
	FGSplatExportOptions GetExportOptions() const;

private:
	UGSplatAsset* Asset = nullptr;
	TWeakPtr<SWindow> WidgetWindow;
	bool bShouldExport = false;

	TSharedPtr<SGSplatOptionGroup> CoordinateSpaceGroup;

	TSharedPtr<SCheckBox> IncludeHighOrderSHCheckBox;
	TSharedPtr<SCheckBox> IncludeNormalsCheckBox;
	TSharedPtr<SCheckBox> IncludeMetallicRoughnessCheckBox;

	TSharedPtr<SPrimaryButton> ExportButton;
};
