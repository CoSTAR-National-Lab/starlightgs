#include "GSplatExportWindow.h"

#include "GSplatDialogHelpers.h"
#include "SPrimaryButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "GSplatExportWindow"


// --- SGSplatOptionGroup ---

void SGSplatOptionGroup::Construct(const FArguments& InArgs)
{
	SelectedIndex = FMath::Clamp(InArgs._SelectedIndex, 0, FMath::Max(InArgs._Options.Num() - 1, 0));

	this->ChildSlot
	[
		SAssignNew(Container, SHorizontalBox)
	];

	// Generate buttons
	OptionButtons.SetNum(InArgs._Options.Num());
	for (int32 i = 0; i < OptionButtons.Num(); i++)
	{
		Container->AddSlot()
		[
			SAssignNew(OptionButtons[i], SButton)
			.Text(InArgs._Options[i])
			.ButtonStyle(FAppStyle::Get(), i == SelectedIndex ? TEXT("PrimaryButton") : TEXT("Button"))
			.OnClicked(this, &SGSplatOptionGroup::OnOptionClicked, i)
		];
	}

	RefreshStyles();
}

FReply SGSplatOptionGroup::OnOptionClicked(int32 Index)
{
	SelectedIndex = FMath::Clamp(Index, 0, FMath::Max(OptionButtons.Num() - 1, 0));
	RefreshStyles();

	if (OnOptionChanged)
	{
		OnOptionChanged(SelectedIndex);
	}

	return FReply::Handled();
}

void SGSplatOptionGroup::RefreshStyles()
{
	for (int32 i = 0; i < OptionButtons.Num(); i++)
	{
		OptionButtons[i]->SetButtonStyle(
			&FAppStyle::Get().GetWidgetStyle<FButtonStyle>(i == SelectedIndex ? TEXT("PrimaryButton") : TEXT("Button"))
		);
	}
}


// --- SGSplatExportWindow ---

void SGSplatExportWindow::Construct(const FArguments& InArgs)
{
	Asset = InArgs._Asset;
	WidgetWindow = InArgs._WidgetWindow;

	const FGaussianSplatData& Splats = Asset->Splats;

	const bool bHasImportData = Asset->AssetImportData != nullptr && Asset->AssetImportData	->Swizzle.Num() == 3;
	const bool bHasNormals = Splats.bHasNormals && Splats.Normals.Num() == Splats.SplatCount;
	const bool bHasHighOrderSH = Splats.bHasHighOrderSH && Splats.HighOrderHarmonicsCoefficients.Num() == Splats.SplatCount * FGaussianSplatData::NumHighOrderSH;
	const bool bHasMetallicRoughness = Splats.bHasMetallicRoughness && Splats.MetallicRoughness.Num() == Splats.SplatCount;

	const int32 DefaultSpaceIndex = bHasImportData ? 0 : 1;
		
	TSharedPtr<SBox> OptionsBox;
	this->ChildSlot
	[
		SNew(SBox)
		.MaxDesiredHeight(InArgs._MaxWindowHeight)
		.MaxDesiredWidth(InArgs._MaxWindowWidth)
		[
			SNew(SVerticalBox)
			+ AddInfoSlot(
				LOCTEXT("Export_AssetName", "Asset: "),
				FText::FromName(Asset->GetFName())
			)
			+ AddInfoSlot(
				LOCTEXT("Export_SplatCount", "Num Splats: "),
				FText::AsNumber(Splats.SplatCount)
			)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SAssignNew(OptionsBox, SBox)
				.MaxDesiredHeight(650.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(1, 0)
				[
					SAssignNew(ExportButton, SPrimaryButton)
					.Text(LOCTEXT("GSplatExportWindow_Export", "Export"))
					.OnClicked(this, &SGSplatExportWindow::OnExport)
				]
				+ SUniformGridPanel::Slot(2, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("GSplatExportWindow_Cancel", "Cancel"))
					.ToolTipText(LOCTEXT("GSplatExportWindow_Cancel_ToolTip", "Cancels exporting this splat asset"))
					.OnClicked(this, &SGSplatExportWindow::OnCancel)
				]
			]
		]
	];

	OptionsBox->SetContent(
		SNew(SVerticalBox)
		+ AddOptionSlot(
			LOCTEXT("CoordinateSpaceLabel", "Coordinate Space"),
			[this, DefaultSpaceIndex, bHasImportData]
			{
				TSharedRef<SGSplatOptionGroup> Group = SAssignNew(CoordinateSpaceGroup, SGSplatOptionGroup)
				.Options({
					LOCTEXT("CoordinateSpaceOriginal", "Original"),
					LOCTEXT("CoordinateSpaceUnreal", "Unreal")
				})
				.SelectedIndex(DefaultSpaceIndex)
				.IsEnabled(bHasImportData);

				return Group;
			}
		)
		+ AddOptionSlot(
			LOCTEXT("HighOrderSHLabel", "Include High-Order SH"),
			[this, bHasHighOrderSH]
			{
				return SAssignNew(IncludeHighOrderSHCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("HighOrderSHTooltip", "Write per-splat higher-order spherical harmonic coefficients (f_rest_0..44) to the file."))
				.IsChecked(bHasHighOrderSH ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(bHasHighOrderSH);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("NormalsLabel", "Include Normals"),
			[this, bHasNormals]
			{
				return SAssignNew(IncludeNormalsCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("NormalsTooltip", "Write per-splat surface normals to the file."))
				.IsChecked(bHasNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(bHasNormals);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("MetallicRoughnessLabel", "Include Metallic/Roughness"),
			[this, bHasMetallicRoughness]
			{
				return SAssignNew(IncludeMetallicRoughnessCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("MetallicRoughnessTooltip", "Write per-splat metallic/roughness properties to the file."))
				.IsChecked(bHasMetallicRoughness ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(bHasMetallicRoughness);
			}
		)
	);
}

FReply SGSplatExportWindow::OnExport()
{
	bShouldExport = true;
	if (auto Window = WidgetWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SGSplatExportWindow::OnCancel()
{
	bShouldExport = false;
	if (auto Window = WidgetWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FGSplatExportOptions SGSplatExportWindow::GetExportOptions() const
{
	FGSplatExportOptions Options;

	Options.CoordinateSpace = (CoordinateSpaceGroup && CoordinateSpaceGroup->GetSelectedIndex() == 0)
		? EGSplatExportCoordinateSpace::OriginalCapture
		: EGSplatExportCoordinateSpace::UnrealEngine;

	Options.bIncludeHighOrderSH = IncludeHighOrderSHCheckBox.IsValid() && IncludeHighOrderSHCheckBox->IsChecked();

	Options.bIncludeNormals = IncludeNormalsCheckBox.IsValid() && IncludeNormalsCheckBox->IsChecked();

	Options.bIncludeMetallicRoughness = IncludeMetallicRoughnessCheckBox.IsValid() && IncludeMetallicRoughnessCheckBox->IsChecked();

	return Options;
}

#undef LOCTEXT_NAMESPACE
