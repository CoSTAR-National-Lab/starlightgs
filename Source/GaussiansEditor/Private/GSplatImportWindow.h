#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "GSplatImporter.h"


class SGSplatSwizzleSelectionWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGSplatSwizzleSelectionWidget) {}
		SLATE_ARGUMENT(EGSplatSwizzleAxis, DefaultAxis)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	inline EGSplatSwizzleAxis GetSelectedAxis() const { return SelectedAxis; }

private:
	FReply OnSwizzleClicked(EGSplatSwizzleAxis Axis);

	void RefreshStyles();

private:
	TSharedPtr<SHorizontalBox> Container;
	TArray<TSharedPtr<SButton>> SwizzleButtons;

	EGSplatSwizzleAxis SelectedAxis = EGSplatSwizzleAxis::X;
};


class SGSplatImportWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGSplatImportWindow) {}
		SLATE_ARGUMENT(FGSplatImportData*, ImportData)

		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_ARGUMENT(FText, FullPath)
		SLATE_ARGUMENT(float, MaxWindowHeight)
		SLATE_ARGUMENT(float, MaxWindowWidth)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	FReply OnImport();
	FReply OnCancel();
	bool ShouldImport() const;

	bool ShouldImportNormals() const;
	bool ShouldImportHighOrderSH() const;
	bool ShouldImportMetallicRoughness() const;

	inline float GetScaleFactor() const { return ScaleFactor; }

	// Gets the swizzle to apply on import
	void GetImportSwizzle(TStaticArray<EGSplatSwizzleAxis, 3>& Axes) const;

private:
	bool CanImport() const;

private:
	FGSplatImportData* ImportData = nullptr;

	TSharedPtr<SButton> ImportButton = nullptr;
	TSharedPtr<SCheckBox> ImportNormalsCheckBox = nullptr;
	TSharedPtr<SCheckBox> ImportHighOrderSHCheckBox = nullptr;
	TSharedPtr<SCheckBox> ImportMetallicRoughnessCheckBox = nullptr;
	TSharedPtr<SNumericEntryBox<float>> ScaleFactoryEntryBox = nullptr;

	TSharedPtr<SGSplatSwizzleSelectionWidget> XAxisSwizzleWidget = nullptr;
	TSharedPtr<SGSplatSwizzleSelectionWidget> YAxisSwizzleWidget = nullptr;
	TSharedPtr<SGSplatSwizzleSelectionWidget> ZAxisSwizzleWidget = nullptr;

	TWeakPtr<SWindow> WidgetWindow = nullptr;

	float ScaleFactor = 0.0f;

	bool bShouldImport = false;
};
