#include "GSplatImportWindow.h"

#include "GSplatImporter.h"
#include "SPrimaryButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "GSplatImportWindow"


FText AxisToText(EGSplatSwizzleAxis Axis)
{
	switch (Axis)
	{
	case EGSplatSwizzleAxis::X:	 return INVTEXT("+X");
	case EGSplatSwizzleAxis::Y:	 return INVTEXT("+Y");
	case EGSplatSwizzleAxis::Z:	 return INVTEXT("+Z");
	case EGSplatSwizzleAxis::Xn: return INVTEXT("-X");
	case EGSplatSwizzleAxis::Yn: return INVTEXT("-Y");
	case EGSplatSwizzleAxis::Zn: return INVTEXT("-Z");
	default:
		checkNoEntry();
		break;
	}

	return INVTEXT("Invalid axis!");
}

void SGSplatSwizzleSelectionWidget::Construct(const FArguments& InArgs)
{
	SelectedAxis = InArgs._DefaultAxis;

	this->ChildSlot
	[
		SAssignNew(Container, SHorizontalBox)
	];

	// Generate buttons
	int32 NumAxes = static_cast<int32>(EGSplatSwizzleAxis::Max);
	SwizzleButtons.SetNum(NumAxes);
	for (int32 i = 0; i < NumAxes; i++)
	{
		EGSplatSwizzleAxis Axis = static_cast<EGSplatSwizzleAxis>(i);

		Container->AddSlot()
		[
			SAssignNew(SwizzleButtons[i], SButton)
			.Text(AxisToText(Axis))
			.ButtonStyle(FAppStyle::Get(), Axis == SelectedAxis ? TEXT("PrimaryButton") : TEXT("Button"))
			.OnClicked(this, &SGSplatSwizzleSelectionWidget::OnSwizzleClicked, Axis)
		];
	}
}

FReply SGSplatSwizzleSelectionWidget::OnSwizzleClicked(EGSplatSwizzleAxis Axis)
{
	SelectedAxis = Axis;
	RefreshStyles();

	return FReply::Handled();
}

void SGSplatSwizzleSelectionWidget::RefreshStyles()
{
	for (int32 i = 0; i < SwizzleButtons.Num(); i++)
	{
		EGSplatSwizzleAxis Axis = static_cast<EGSplatSwizzleAxis>(i);
		SwizzleButtons[i]->SetButtonStyle(
			&FAppStyle::Get().GetWidgetStyle<FButtonStyle>(Axis == SelectedAxis ? TEXT("PrimaryButton") : TEXT("Button"))
		);
	}
}


void SGSplatImportWindow::Construct(const FArguments& InArgs)
{
	ImportData = InArgs._ImportData;
	WidgetWindow = InArgs._WidgetWindow;

	ScaleFactor = ImportData->ScaleFactor;

	MetallicDefaultValue = ImportData->MetallicDefaultValue;
	RoughnessDefaultValue = ImportData->RoughnessDefaultValue;

	TSharedPtr<SBox> InspectorBox;
	this->ChildSlot
	[
		SNew(SBox)
		.MaxDesiredHeight(InArgs._MaxWindowHeight)
		.MaxDesiredWidth(InArgs._MaxWindowWidth)
		[
			SNew(SVerticalBox)
			+ AddInfoSlot(
				LOCTEXT("Import_CurrentFileTitle", "Current Asset: "),
				FText(InArgs._FullPath)
			)
			+ AddInfoSlot(
				LOCTEXT("Import_CurrentSplatCount", "Num Splats: "),
				FText::AsNumber(ImportData->NumSplats)
			)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SAssignNew(InspectorBox, SBox)
				.MaxDesiredHeight(650.0f)
				.WidthOverride(400.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(1, 0)
				[
					SAssignNew(ImportButton, SPrimaryButton)
					.Text(LOCTEXT("GSplatImportWindow_Import", "Import"))
					.IsEnabled(this, &SGSplatImportWindow::CanImport)
					.OnClicked(this, &SGSplatImportWindow::OnImport)
				]
				+ SUniformGridPanel::Slot(2, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("GSplatImportWindow_Cancel", "Cancel"))
					.ToolTipText(LOCTEXT("GSplatImportWindow_Cancel_ToolTip", "Cancels importing this ply file"))
					.OnClicked(this, &SGSplatImportWindow::OnCancel)
				]
			]
		]
	];

	InspectorBox->SetContent(
		SNew(SVerticalBox)
		+ AddOptionSlot(
			LOCTEXT("ImportNormalsCheckBoxLabel", "Import Normals"),
			[this]
			{
				return SAssignNew(ImportNormalsCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportNormalsCheckBoxTooltip", "Import per-splat normal vectors."))
				.IsChecked(ImportData->bHasNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasNormals);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ImportHighOrderSHCheckBoxLabel", "Import High-Order SH"),
			[this]
			{
				return SAssignNew(ImportHighOrderSHCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportHighOrderCheckBoxTooltip", "Import per-splat higher-order spherical harmonic coefficients."))
				.IsChecked(ImportData->bHasHighOrderSH ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasHighOrderSH);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ImportMetallicRoughnessCheckBoxLabel", "Import Metallic/Roughness"),
			[this]
			{
				return SAssignNew(ImportMetallicRoughnessCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportMetallicRoughnessCheckBoxTooltip", "Import per-splat metallic/roughness properties."))
				.IsChecked(ImportData->bHasMetallicRoughness ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasMetallicRoughness);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("PopMetallicRoughnessWithDefaultCheckBoxLabel", "Populate Metallic/Roughness With Default"),
			[this]
			{
				return SAssignNew(PopulateMetallicRoughnessCheckBox, SCheckBox)
					.ToolTipText(LOCTEXT("PopMetallicRoughnessWithDefaultCheckBoxTooltip", "Populate per-splat metallic/roughness properties with a default value."))
					.IsChecked(ImportData->bPopulateMRWithDefault ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.IsEnabled(!ImportData->bHasMetallicRoughness);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("MetallicDefaultLabel", "Metallic Default Value"),
			[this]
			{
				return SNew(SSpinBox<float>)
					.ToolTipText(LOCTEXT("MetallicDefaultTooltip", "Default metallic value to assign to every splat."))
					.Value_Lambda([this] { return MetallicDefaultValue; })
					.OnValueChanged_Lambda([this](float Val) { MetallicDefaultValue = Val; })
					.IsEnabled_Lambda([this](){ return PopulateMetallicRoughnessCheckBox->IsChecked(); });
			}
		)
		+ AddOptionSlot(
			LOCTEXT("RoughnessDefaultLabel", "Roughness Default Value"),
			[this]
			{
				return SNew(SSpinBox<float>)
					.ToolTipText(LOCTEXT("RoughnessDefaultTooltip", "Default roughness value to assign to every splat."))
					.Value_Lambda([this] { return RoughnessDefaultValue; })
					.OnValueChanged_Lambda([this](float Val) { RoughnessDefaultValue = Val; })
					.IsEnabled_Lambda([this](){ return PopulateMetallicRoughnessCheckBox->IsChecked(); });
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ScaleFactorLabel", "Scale Factor"),
			[this]
			{
				return SNew(SSpinBox<float>)
				.ToolTipText(LOCTEXT("ScaleFactorTooltip", "Scale factor to apply on import. Default of 100 converts from metres to centimetres."))
				.Value_Lambda([this]{ return ScaleFactor; })
				.OnValueChanged_Lambda([this](float Val){ ScaleFactor = Val; });
			}
		)
		+ AddOptionSlot(
			LOCTEXT("XAxisSwizzleLabel", "X Swizzle"),
			[this]
			{
				return SAssignNew(XAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[0]);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("YAxisSwizzleLabel", "Y Swizzle"),
			[this]
			{
				return SAssignNew(YAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[1]);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ZAxisSwizzleLabel", "Z Swizzle"),
			[this]
			{
				return SAssignNew(ZAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[2]);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ConvertSHCheckBoxLabel", "Base Colour is RGB"),
			[this]
			{
				return SAssignNew(BaseColorRGBCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ConvertSHCheckBoxTooltip", "Check if the original PLY file has encoded base colour in RGB (as opposed to SH coefficients)."))
				.IsChecked(ImportData->bBaseColorIsRGB ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("ApplyGammaCorrectionCheckBoxLabel", "Apply Gamma Correction"),
			[this]
			{
				return SAssignNew(ApplyGammaCorrectionCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ApplyGammaCorrectionCheckBoxTooltip", "Apply Gamma correction to base color attribute on import."))
				.IsChecked(ImportData->bApplyGammaCorrection ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
			}
		)
		+ AddOptionSlot(
			LOCTEXT("OpacityAsAlphaCheckBoxLabel", "Import Opacity as Alpha"),
			[this]
			{
				return SAssignNew(OpacityAsAlphaCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("OpacityAsAlphaCheckBoxTooltip", "Treat opacity values as alpha values on import."))
				.IsChecked(ImportData->bOpacityAsAlpha ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
			}
		)
	);
}

SVerticalBox::FSlot::FSlotArguments SGSplatImportWindow::AddInfoSlot(FText&& Label, FText&& Value)
{
	return MoveTemp(SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(Label)
				]
				+ SHorizontalBox::Slot()
				.Padding(5, 0, 0, 0)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Value)
					.ToolTipText(Value)
				]
			]
		]);
}

SVerticalBox::FSlot::FSlotArguments SGSplatImportWindow::AddOptionSlot(FText&& Label,
	TFunction<TSharedPtr<SWidget>()>&& CreateWidgetFunc)
{
	return MoveTemp(SVerticalBox::Slot()
	.AutoHeight()
	.Padding(2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(Label)
		]
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.AutoWidth()
		.Padding(2.0f)
		[
			CreateWidgetFunc().ToSharedRef()
		]
	]);
}

FReply SGSplatImportWindow::OnImport()
{
	bShouldImport = true;
	if (auto Window = WidgetWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SGSplatImportWindow::OnCancel()
{
	bShouldImport = false;
	if (auto Window = WidgetWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	return FReply::Handled();
}

bool SGSplatImportWindow::ShouldImport() const
{
	return bShouldImport;
}

bool SGSplatImportWindow::ShouldImportNormals() const
{
	return ImportNormalsCheckBox->IsChecked();
}

bool SGSplatImportWindow::ShouldImportHighOrderSH() const
{
	return ImportHighOrderSHCheckBox->IsChecked();
}

bool SGSplatImportWindow::ShouldImportMetallicRoughness() const
{
	return ImportMetallicRoughnessCheckBox->IsChecked();
}

bool SGSplatImportWindow::ShouldPopulateMetallicRoughnessWithDefault() const
{
	return PopulateMetallicRoughnessCheckBox->IsChecked();
}

bool SGSplatImportWindow::GetBaseColorIsRGB() const
{
	return BaseColorRGBCheckBox->IsChecked();
}

bool SGSplatImportWindow::GetApplyGammaCorrection() const
{
	return ApplyGammaCorrectionCheckBox->IsChecked();
}

bool SGSplatImportWindow::GetOpacityAsAlpha() const
{
	return OpacityAsAlphaCheckBox->IsChecked();
}

void SGSplatImportWindow::GetImportSwizzle(TArray<EGSplatSwizzleAxis>& Axes) const
{
	check(Axes.Num() == 3);
	Axes[0] = XAxisSwizzleWidget->GetSelectedAxis();
	Axes[1] = YAxisSwizzleWidget->GetSelectedAxis();
	Axes[2] = ZAxisSwizzleWidget->GetSelectedAxis();
}

bool SGSplatImportWindow::CanImport() const
{
	if (ScaleFactor == 0.0f)
	{
		return false;
	}

	// Check all swizzle axes are different
	TArray<EGSplatSwizzleAxis> Axes{ EGSplatSwizzleAxis::X, EGSplatSwizzleAxis::Y, EGSplatSwizzleAxis::Z };
	GetImportSwizzle(Axes);
	uint32 Counter = 0;
	for (const auto& i : Axes)
	for (const auto& j : Axes)
	{
		auto AbsAxis = [](EGSplatSwizzleAxis Axis)
		{
			switch (Axis)
			{
			case EGSplatSwizzleAxis::X:	 
			case EGSplatSwizzleAxis::Xn: return EGSplatSwizzleAxis::X;
			case EGSplatSwizzleAxis::Y:	 
			case EGSplatSwizzleAxis::Yn: return EGSplatSwizzleAxis::Y;
			case EGSplatSwizzleAxis::Z:	 
			case EGSplatSwizzleAxis::Zn: return EGSplatSwizzleAxis::Z;
			default:
				checkNoEntry();
			}
			return EGSplatSwizzleAxis::Max;
		};
		Counter += AbsAxis(i) == AbsAxis(j);
	}

	if (Counter != 3)
	{
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE