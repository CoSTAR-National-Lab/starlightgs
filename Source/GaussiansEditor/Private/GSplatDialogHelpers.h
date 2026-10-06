#pragma once

#include "CoreMinimal.h"
#include "GSplatImportData.h"

#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"


/**
 * Shared helpers for the GSplat import/export option dialogs.
 */

// Adds a label and text with a different background to standout
inline SVerticalBox::FSlot::FSlotArguments AddInfoSlot(FText&& Label, FText&& Value)
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

// Adds an option with label and widget to control the option
inline SVerticalBox::FSlot::FSlotArguments AddOptionSlot(
	FText&& Label,
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
