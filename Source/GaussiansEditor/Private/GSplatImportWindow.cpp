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

	TSharedPtr<SBox> InspectorBox;
	this->ChildSlot
	[
		SNew(SBox)
		.MaxDesiredHeight(InArgs._MaxWindowHeight)
		.MaxDesiredWidth(InArgs._MaxWindowWidth)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
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
						.Text(LOCTEXT("Import_CurrentFileTitle", "Current Asset: "))
					]
					+ SHorizontalBox::Slot()
					.Padding(5, 0, 0, 0)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(InArgs._FullPath)
						.ToolTipText(InArgs._FullPath)
					]
				]
			]
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
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ImportNormalsCheckBoxLabel", "Import Normals"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ImportNormalsCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportNormalsCheckBoxTooltip", "Import per-splat normal vectors."))
				.IsChecked(ImportData->bHasNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasNormals)
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ImportHighOrderSHCheckBoxLabel", "Import High-Order SH"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ImportHighOrderSHCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportHighOrderCheckBoxTooltip", "Import per-splat higher-order spherical harmonic coefficients."))
				.IsChecked(ImportData->bHasHighOrderSH ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasHighOrderSH)
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ImportMetallicRoughnessCheckBoxLabel", "Import Metallic/Roughness"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ImportMetallicRoughnessCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ImportMetallicRoughnessCheckBoxTooltip", "Import per-splat metallic/roughness properties."))
				.IsChecked(ImportData->bHasMetallicRoughness ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.IsEnabled(ImportData->bHasMetallicRoughness)
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ScaleFactorLabel", "Scale Factor"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SSpinBox<float>)
				.ToolTipText(LOCTEXT("ScaleFactorTooltip", "Scale factor to apply on import. Default of 100 converts from metres to centimetres."))
				.Value_Lambda([this]{ return ScaleFactor; })
				.OnValueChanged_Lambda([this](float Val){ ScaleFactor = Val; })
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("XAxisSwizzleLabel", "X Swizzle"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(XAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[0])
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("YAxisSwizzleLabel", "Y Swizzle"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(YAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[1])
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ZAxisSwizzleLabel", "Z Swizzle"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ZAxisSwizzleWidget, SGSplatSwizzleSelectionWidget)
				.DefaultAxis(ImportData->Swizzle[2])
			]
		]
		+ SVerticalBox::Slot()
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
				.Text(LOCTEXT("ApplyGammaCorrectionCheckBoxLabel", "Apply Gamma Correction"))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ApplyGammaCorrectionCheckBox, SCheckBox)
				.ToolTipText(LOCTEXT("ApplyGammaCorrectionCheckBoxTooltip", "Apply Gamma correction to base color attribute on import."))
				.IsChecked(ImportData->bApplyGammaCorrection ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			]
		]
	);
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

bool SGSplatImportWindow::GetApplyGammaCorrection() const
{
	return ApplyGammaCorrectionCheckBox->IsChecked();
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