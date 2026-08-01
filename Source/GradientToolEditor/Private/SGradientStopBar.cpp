// Copyright (c) Jared Taylor

#include "SGradientStopBar.h"

#include "GradientAsset.h"
#include "GradientTextureBuilder.h"
#include "ScopedTransaction.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Colors/SComplexGradient.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "SGradientStopBar"

void SGradientStopBar::Construct(const FArguments& InArgs, UGradientAsset* InGradient)
{
	Gradient = InGradient;
	OnGradientChanged = InArgs._OnGradientChanged;

	ChildSlot
	.Padding(HandleHalfWidth, 0.f, HandleHalfWidth, HandleAreaHeight)
	[
		SNew(SBox)
		.HeightOverride(GradientHeight)
		[
			SAssignNew(GradientWidget, SComplexGradient)
			.GradientColors(TAttribute<TArray<FLinearColor>>::CreateSP(this, &SGradientStopBar::GetPreviewColors))
			.HasAlphaBackground(true)
			.Orientation(Orient_Vertical)
		]
	];

	Refresh();
}

void SGradientStopBar::Refresh()
{
	if (const UGradientAsset* GradientAsset = Gradient.Get())
	{
		GradientAsset->Sample(PreviewSamples, PreviewColors);

		if (!GradientAsset->Stops.IsValidIndex(SelectedStop))
		{
			SelectedStop = INDEX_NONE;
		}
	}
	else
	{
		PreviewColors.Reset();
		SelectedStop = INDEX_NONE;
	}

	if (GradientWidget.IsValid())
	{
		GradientWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

float SGradientStopBar::TimeToLocalX(float LocalSizeX, float Time) const
{
	const float Usable = FMath::Max(LocalSizeX - 2.f * HandleHalfWidth, 1.f);
	return HandleHalfWidth + Time * Usable;
}

float SGradientStopBar::LocalXToTime(float LocalSizeX, float LocalX) const
{
	const float Usable = FMath::Max(LocalSizeX - 2.f * HandleHalfWidth, 1.f);
	return FMath::Clamp((LocalX - HandleHalfWidth) / Usable, 0.f, 1.f);
}

int32 SGradientStopBar::FindStopAtLocalX(float LocalSizeX, float LocalX) const
{
	const UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset)
	{
		return INDEX_NONE;
	}

	int32 Closest = INDEX_NONE;
	float ClosestDistance = HandleHalfWidth + 2.f;

	for (int32 Index = 0; Index < GradientAsset->Stops.Num(); ++Index)
	{
		const float Distance = FMath::Abs(TimeToLocalX(LocalSizeX, GradientAsset->Stops[Index].Time) - LocalX);
		if (Distance <= ClosestDistance)
		{
			ClosestDistance = Distance;
			Closest = Index;
		}
	}

	return Closest;
}

int32 SGradientStopBar::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset)
	{
		return Layer;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const float LocalSizeX = static_cast<float>(AllottedGeometry.GetLocalSize().X);

	const float HandleTop = GradientHeight + 2.f;
	const float HandleSize = HandleHalfWidth * 2.f;
	const float SwatchInset = 1.5f;

	++Layer;

	for (int32 Index = 0; Index < GradientAsset->Stops.Num(); ++Index)
	{
		const FGradientStop& Stop = GradientAsset->Stops[Index];
		const bool bSelected = (Index == SelectedStop);
		const float CentreX = TimeToLocalX(LocalSizeX, Stop.Time);

		if (bSelected)
		{
			TArray<FVector2f> Line = { FVector2f(CentreX, 0.f), FVector2f(CentreX, GradientHeight) };
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Line),
				ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.5f), true, 1.f);
		}

		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(FVector2f(HandleSize, HandleSize),
				FSlateLayoutTransform(FVector2f(CentreX - HandleHalfWidth, HandleTop))),
			WhiteBrush, ESlateDrawEffect::None,
			bSelected ? FLinearColor::White : FLinearColor(0.05f, 0.05f, 0.05f));

		const FLinearColor Swatch(Stop.Color.R, Stop.Color.G, Stop.Color.B, 1.f);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(HandleSize - SwatchInset * 2.f, HandleSize - SwatchInset * 2.f),
				FSlateLayoutTransform(FVector2f(CentreX - HandleHalfWidth + SwatchInset, HandleTop + SwatchInset))),
			WhiteBrush, ESlateDrawEffect::None, Swatch);
	}

	return Layer + 2;
}

FReply SGradientStopBar::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset)
	{
		return FReply::Unhandled();
	}

	const FVector2f LocalPosition = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	const float LocalSizeX = static_cast<float>(MyGeometry.GetLocalSize().X);
	const int32 HitStop = FindStopAtLocalX(LocalSizeX, LocalPosition.X);

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (HitStop == INDEX_NONE)
		{
			return FReply::Unhandled();
		}

		SelectedStop = HitStop;
		Invalidate(EInvalidateWidgetReason::Paint);

		FSlateApplication::Get().PushMenu(SharedThis(this), FWidgetPath(), BuildStopContextMenu(HitStop),
			MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (HitStop == INDEX_NONE)
		{
			return FReply::Unhandled();
		}

		DeleteStop(HitStop);
		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	bAddedStopOnDown = (HitStop == INDEX_NONE);

	if (bAddedStopOnDown)
	{
		ActiveTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("AddStop", "Add Gradient Stop"));
		GradientAsset->Modify();
		AddStopAtTime(LocalXToTime(LocalSizeX, LocalPosition.X));
	}
	else
	{
		ActiveTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("MoveStop", "Move Gradient Stop"));
		GradientAsset->Modify();
		SelectedStop = HitStop;
	}

	DraggedStop = SelectedStop;
	bDragMoved = false;

	return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SGradientStopBar::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset || DraggedStop == INDEX_NONE || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (!GradientAsset->Stops.IsValidIndex(DraggedStop))
	{
		DraggedStop = INDEX_NONE;
		return FReply::Unhandled();
	}

	const FVector2f LocalPosition = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	const float NewTime = LocalXToTime(static_cast<float>(MyGeometry.GetLocalSize().X), LocalPosition.X);

	if (FMath::IsNearlyEqual(GradientAsset->Stops[DraggedStop].Time, NewTime))
	{
		return FReply::Handled();
	}

	GradientAsset->Stops[DraggedStop].Time = NewTime;
	DraggedStop = GradientAsset->SortStops(DraggedStop);
	SelectedStop = DraggedStop;
	bDragMoved = true;

	CommitChange(/*bRebuild*/false);

	return FReply::Handled();
}

FReply SGradientStopBar::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (ActiveTransaction.IsValid() && !bDragMoved && !bAddedStopOnDown)
	{
		ActiveTransaction->Cancel();
	}
	else if (bDragMoved)
	{
		CommitChange(/*bRebuild*/true);
	}

	ActiveTransaction.Reset();
	DraggedStop = INDEX_NONE;
	bDragMoved = false;
	bAddedStopOnDown = false;

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SGradientStopBar::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	const FVector2f LocalPosition = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	const int32 HitStop = FindStopAtLocalX(static_cast<float>(MyGeometry.GetLocalSize().X), LocalPosition.X);

	if (HitStop == INDEX_NONE)
	{
		return FReply::Unhandled();
	}

	SelectedStop = HitStop;
	OpenStopColorPicker(HitStop);

	return FReply::Handled();
}

FReply SGradientStopBar::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Delete || KeyEvent.GetKey() == EKeys::BackSpace)
	{
		if (SelectedStop != INDEX_NONE)
		{
			DeleteStop(SelectedStop);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SGradientStopBar::CommitChange(bool bRebuild)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset)
	{
		return;
	}

	if (bRebuild)
	{
		GradientToolEditor::RebuildGradient(GradientAsset);
	}

	Refresh();
	OnGradientChanged.ExecuteIfBound();
}

void SGradientStopBar::AddStopAtTime(float Time)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset)
	{
		return;
	}

	const int32 NewIndex = GradientAsset->Stops.Add(FGradientStop(Time, GradientAsset->Evaluate(Time)));
	SelectedStop = GradientAsset->SortStops(NewIndex);

	CommitChange(/*bRebuild*/true);
}

void SGradientStopBar::DeleteStop(int32 StopIndex)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset || !GradientAsset->Stops.IsValidIndex(StopIndex) || GradientAsset->Stops.Num() <= 2)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteStop", "Delete Gradient Stop"));
	GradientAsset->Modify();

	GradientAsset->Stops.RemoveAt(StopIndex);
	SelectedStop = INDEX_NONE;

	CommitChange(/*bRebuild*/true);
}

void SGradientStopBar::SetStopInterp(int32 StopIndex, EGradientInterp Interp)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset || !GradientAsset->Stops.IsValidIndex(StopIndex))
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetStopInterp", "Set Gradient Stop Interpolation"));
	GradientAsset->Modify();

	GradientAsset->Stops[StopIndex].Interp = Interp;

	CommitChange(/*bRebuild*/true);
}

void SGradientStopBar::OpenStopColorPicker(int32 StopIndex)
{
	const UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset || !GradientAsset->Stops.IsValidIndex(StopIndex))
	{
		return;
	}

	ColorPickerStop = StopIndex;

	FColorPickerArgs PickerArgs;
	PickerArgs.bIsModal = false;
	PickerArgs.bUseAlpha = true;
	PickerArgs.ParentWidget = SharedThis(this);
	PickerArgs.InitialColor = GradientAsset->Stops[StopIndex].Color;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SGradientStopBar::OnStopColorCommitted);

	PickerArgs.OnInteractivePickBegin = FSimpleDelegate::CreateSPLambda(this, [this]()
	{
		if (UGradientAsset* GradientAsset = Gradient.Get())
		{
			bInteractiveColorPick = true;
			ActiveTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("SetStopColor", "Set Gradient Stop Colour"));
			GradientAsset->Modify();
		}
	});

	PickerArgs.OnInteractivePickEnd = FSimpleDelegate::CreateSPLambda(this, [this]()
	{
		bInteractiveColorPick = false;
		ActiveTransaction.Reset();
		CommitChange(/*bRebuild*/true);
	});

	OpenColorPicker(PickerArgs);
}

void SGradientStopBar::OnStopColorCommitted(FLinearColor NewColor)
{
	UGradientAsset* GradientAsset = Gradient.Get();
	if (!GradientAsset || !GradientAsset->Stops.IsValidIndex(ColorPickerStop))
	{
		return;
	}

	// Dragging inside the picker fires this continuously; the bake waits for the drag to end.
	if (bInteractiveColorPick)
	{
		GradientAsset->Stops[ColorPickerStop].Color = NewColor;
		CommitChange(/*bRebuild*/false);
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetStopColor", "Set Gradient Stop Colour"));
	GradientAsset->Modify();

	GradientAsset->Stops[ColorPickerStop].Color = NewColor;

	CommitChange(/*bRebuild*/true);
}

TSharedRef<SWidget> SGradientStopBar::BuildStopContextMenu(int32 StopIndex)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("StopSection", "Gradient Stop"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SetColorEntry", "Set Colour..."),
			LOCTEXT("SetColorEntryTooltip", "Pick the colour for this stop."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SGradientStopBar::OpenStopColorPicker, StopIndex)));

		MenuBuilder.AddSubMenu(
			LOCTEXT("InterpolationEntry", "Interpolation"),
			LOCTEXT("InterpolationEntryTooltip", "How the gradient reaches this stop from the one before it."),
			FNewMenuDelegate::CreateSPLambda(this, [this, StopIndex](FMenuBuilder& SubMenuBuilder)
			{
				const UEnum* InterpEnum = StaticEnum<EGradientInterp>();
				for (int32 EnumIndex = 0; EnumIndex < InterpEnum->NumEnums() - 1; ++EnumIndex)
				{
					const EGradientInterp Interp = static_cast<EGradientInterp>(InterpEnum->GetValueByIndex(EnumIndex));
					SubMenuBuilder.AddMenuEntry(
						InterpEnum->GetDisplayNameTextByIndex(EnumIndex),
						InterpEnum->GetToolTipTextByIndex(EnumIndex),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateSP(this, &SGradientStopBar::SetStopInterp, StopIndex, Interp)));
				}
			}));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("DeleteStopEntry", "Delete Stop"),
			LOCTEXT("DeleteStopEntryTooltip", "Remove this stop. Middle click does the same. A gradient always keeps at least two."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SGradientStopBar::DeleteStop, StopIndex),
				FCanExecuteAction::CreateSPLambda(this, [this]()
				{
					const UGradientAsset* GradientAsset = Gradient.Get();
					return GradientAsset && GradientAsset->Stops.Num() > 2;
				})));
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
