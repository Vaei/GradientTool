// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GradientTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FScopedTransaction;
class UGradientAsset;

/** Gradient strip with draggable colour stops beneath it, editing one gradient of an atlas. */
class SGradientStopBar : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SGradientStopBar)
		: _GradientIndex(0)
	{}

		/** Which gradient of the atlas this bar edits. */
		SLATE_ARGUMENT(int32, GradientIndex)

		/** Raised after any edit that changed the gradient. */
		SLATE_EVENT(FSimpleDelegate, OnGradientChanged)

		/** Raised when the user interacts with this bar. */
		SLATE_EVENT(FSimpleDelegate, OnSelected)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UGradientAsset* InGradient);

	/** Re-reads the asset. Call when the gradient was changed from outside this widget. */
	void Refresh();

	int32 GetSelectedStop() const { return SelectedStop; }

	//~ Begin SWidget Interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	//~ End SWidget Interface

private:

	static constexpr float GradientHeight = 48.f;
	static constexpr float HandleAreaHeight = 18.f;
	static constexpr float HandleHalfWidth = 6.f;
	static constexpr int32 PreviewSamples = 256;

	FGradientLayer* GetLayer() const;

	float TimeToLocalX(float LocalSizeX, float Time) const;
	float LocalXToTime(float LocalSizeX, float LocalX) const;
	int32 FindStopAtLocalX(float LocalSizeX, float LocalX) const;

	TArray<FLinearColor> GetPreviewColors() const { return PreviewColors; }

	/** Applies the edit to the asset, re-bakes if asked, and notifies the toolkit. */
	void CommitChange(bool bRebuild);

	void AddStopAtTime(float Time);
	void DeleteStop(int32 StopIndex);
	void SetStopInterp(int32 StopIndex, EGradientInterp Interp);
	void OpenStopColorPicker(int32 StopIndex);
	void OnStopColorCommitted(FLinearColor NewColor);
	TSharedRef<SWidget> BuildStopContextMenu(int32 StopIndex);

	TWeakObjectPtr<UGradientAsset> Gradient;
	int32 GradientIndex = 0;

	FSimpleDelegate OnGradientChanged;
	FSimpleDelegate OnSelected;

	TSharedPtr<class SComplexGradient> GradientWidget;
	TArray<FLinearColor> PreviewColors;

	int32 SelectedStop = INDEX_NONE;
	int32 DraggedStop = INDEX_NONE;
	int32 ColorPickerStop = INDEX_NONE;

	bool bDragMoved = false;
	bool bAddedStopOnDown = false;
	bool bInteractiveColorPick = false;

	TUniquePtr<FScopedTransaction> ActiveTransaction;
};
