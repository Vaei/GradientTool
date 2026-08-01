// Copyright (c) Jared Taylor

#include "GradientAssetEditorToolkit.h"

#include "GradientAsset.h"
#include "GradientTextureBuilder.h"
#include "SGradientStopBar.h"
#include "ScopedTransaction.h"

#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "GradientAssetEditorToolkit"

const FName FGradientAssetEditorToolkit::GradientTabId(TEXT("GradientAssetEditor_Gradient"));
const FName FGradientAssetEditorToolkit::DetailsTabId(TEXT("GradientAssetEditor_Details"));

FGradientAssetEditorToolkit::~FGradientAssetEditorToolkit()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

UGradientAsset* FGradientAssetEditorToolkit::GetGradient() const
{
	return Cast<UGradientAsset>(GetEditingObject());
}

void FGradientAssetEditorToolkit::Initialize(UGradientAsset* InGradient, EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(InGradient);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FGradientAssetEditorToolkit::OnDetailsPropertyChanged);

	GradientList = SNew(SVerticalBox);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("GradientAssetEditor_Layout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.45f)
				->AddTab(GradientTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.55f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("GradientAssetEditor"), Layout,
		/*bCreateDefaultStandaloneMenu*/true, /*bCreateDefaultToolbar*/true, InGradient);

	RebuildGradientList();
	ExtendToolbar();

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

void FGradientAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "Gradient Editor"));
	const TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	InTabManager->RegisterTabSpawner(GradientTabId, FOnSpawnTab::CreateSP(this, &FGradientAssetEditorToolkit::SpawnTab_Gradient))
		.SetDisplayName(LOCTEXT("GradientTab", "Gradients"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Color"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FGradientAssetEditorToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FGradientAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GradientTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

FName FGradientAssetEditorToolkit::GetToolkitFName() const
{
	return FName("GradientAssetEditor");
}

FText FGradientAssetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Gradient Editor");
}

FString FGradientAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("TabPrefix", "Gradient ").ToString();
}

FLinearColor FGradientAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.75f, 0.25f, 0.25f, 0.5f);
}

TSharedRef<SDockTab> FGradientAssetEditorToolkit::SpawnTab_Gradient(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("GradientTab", "Gradients"))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					GradientList.ToSharedRef()
				]
			]
		];
}

TSharedRef<SDockTab> FGradientAssetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

void FGradientAssetEditorToolkit::RebuildGradientList()
{
	if (!GradientList.IsValid())
	{
		return;
	}

	GradientList->ClearChildren();
	StopBars.Reset();

	const UGradientAsset* Gradient = GetGradient();
	if (!Gradient)
	{
		return;
	}

	SelectedGradient = FMath::Clamp(SelectedGradient, 0, FMath::Max(Gradient->Gradients.Num() - 1, 0));

	for (int32 Index = 0; Index < Gradient->Gradients.Num(); ++Index)
	{
		GradientList->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			BuildGradientRow(Index)
		];
	}
}

TSharedRef<SWidget> FGradientAssetEditorToolkit::BuildGradientRow(int32 GradientIndex)
{
	TSharedPtr<SGradientStopBar> StopBar;

	const TSharedRef<SWidget> Row =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(TAttribute<FSlateColor>::CreateSPLambda(this, [this, GradientIndex]()
		{
			return SelectedGradient == GradientIndex ? FSlateColor(FLinearColor(0.3f, 0.5f, 0.85f)) : FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f));
		}))
		.Padding(6.f)
		.OnMouseButtonDown(FPointerEventHandler::CreateSPLambda(this, [this, GradientIndex](const FGeometry&, const FPointerEvent&)
		{
			SelectedGradient = GradientIndex;
			return FReply::Handled();
		}))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(140.f)
				[
					SNew(SEditableTextBox)
					.Text(TAttribute<FText>::CreateSPLambda(this, [this, GradientIndex]()
					{
						const UGradientAsset* Gradient = GetGradient();
						return (Gradient && Gradient->Gradients.IsValidIndex(GradientIndex))
							? FText::FromName(Gradient->Gradients[GradientIndex].Name)
							: FText::GetEmpty();
					}))
					.ToolTipText(LOCTEXT("GradientNameTooltip", "Name material nodes use to address this gradient's row."))
					.OnTextCommitted(FOnTextCommitted::CreateSPLambda(this, [this, GradientIndex](const FText& NewText, ETextCommit::Type)
					{
						RenameGradient(GradientIndex, NewText);
					}))
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SAssignNew(StopBar, SGradientStopBar, GetGradient())
				.GradientIndex(GradientIndex)
				.OnGradientChanged(FSimpleDelegate::CreateSP(this, &FGradientAssetEditorToolkit::OnGradientChanged))
				.OnSelected(FSimpleDelegate::CreateSPLambda(this, [this, GradientIndex]()
				{
					SelectedGradient = GradientIndex;
				}))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("RemoveGradientTooltip", "Remove this gradient from the atlas."))
				.IsEnabled(TAttribute<bool>::CreateSP(this, &FGradientAssetEditorToolkit::CanRemoveGradient))
				.OnClicked(FOnClicked::CreateSPLambda(this, [this, GradientIndex]()
				{
					RemoveGradient(GradientIndex);
					return FReply::Handled();
				}))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Delete"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];

	StopBars.Add(StopBar);

	return Row;
}

void FGradientAssetEditorToolkit::ExtendToolbar()
{
	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();

	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FGradientAssetEditorToolkit::FillToolbar));

	AddToolbarExtender(ToolbarExtender);
	RegenerateMenusAndToolbars();
}

void FGradientAssetEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("Atlas");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::AddGradient)),
			NAME_None,
			LOCTEXT("AddGradient", "Add"),
			LOCTEXT("AddGradientTooltip", "Add another gradient to the atlas, as a new row of the texture."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Gradient");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::ReverseStops)),
			NAME_None,
			LOCTEXT("Reverse", "Reverse"),
			LOCTEXT("ReverseTooltip", "Mirror every stop of the selected gradient about its centre."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FlipHorizontal"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::DistributeStopsEvenly)),
			NAME_None,
			LOCTEXT("Distribute", "Distribute"),
			LOCTEXT("DistributeTooltip", "Space the selected gradient's stops evenly from 0 to 1, keeping their order and colours."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Normalize"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::RebuildTexture)),
			NAME_None,
			LOCTEXT("Rebuild", "Rebuild"),
			LOCTEXT("RebuildTooltip", "Bake the texture again from the gradients."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
	}
	ToolbarBuilder.EndSection();
}

void FGradientAssetEditorToolkit::AddGradient()
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddGradientTransaction", "Add Gradient"));
	Gradient->Modify();

	SelectedGradient = Gradient->Gradients.Add(FGradientLayer(Gradient->MakeUniqueGradientName(NAME_None)));

	GradientToolEditor::RebuildGradient(Gradient);
	RefreshEverything();
}

bool FGradientAssetEditorToolkit::CanRemoveGradient() const
{
	const UGradientAsset* Gradient = GetGradient();
	return Gradient && Gradient->Gradients.Num() > 1;
}

void FGradientAssetEditorToolkit::RemoveGradient(int32 GradientIndex)
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || !Gradient->Gradients.IsValidIndex(GradientIndex) || Gradient->Gradients.Num() <= 1)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("RemoveGradientTransaction", "Remove Gradient"));
	Gradient->Modify();

	Gradient->Gradients.RemoveAt(GradientIndex);
	SelectedGradient = FMath::Clamp(SelectedGradient, 0, Gradient->Gradients.Num() - 1);

	GradientToolEditor::RebuildGradient(Gradient);
	RefreshEverything();
}

void FGradientAssetEditorToolkit::RenameGradient(int32 GradientIndex, const FText& NewName)
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || !Gradient->Gradients.IsValidIndex(GradientIndex))
	{
		return;
	}

	const FName Desired = Gradient->MakeUniqueGradientName(FName(*NewName.ToString()), GradientIndex);
	if (Desired == Gradient->Gradients[GradientIndex].Name)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("RenameGradientTransaction", "Rename Gradient"));
	Gradient->Modify();

	Gradient->Gradients[GradientIndex].Name = Desired;

	GradientToolEditor::RebuildGradient(Gradient);
	RefreshEverything();
}

void FGradientAssetEditorToolkit::ReverseStops()
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || !Gradient->Gradients.IsValidIndex(SelectedGradient))
	{
		return;
	}

	TArray<FGradientStop>& Stops = Gradient->Gradients[SelectedGradient].Stops;
	if (Stops.Num() < 2)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ReverseTransaction", "Reverse Gradient"));
	Gradient->Modify();

	const int32 NumStops = Stops.Num();
	TArray<FGradientStop> Reversed;
	Reversed.SetNum(NumStops);

	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		const FGradientStop& Source = Stops[NumStops - 1 - Index];
		Reversed[Index].Time = 1.f - Source.Time;
		Reversed[Index].Color = Source.Color;
		Reversed[Index].Interp = (Index == 0) ? EGradientInterp::Linear : Stops[NumStops - Index].Interp;
	}

	Stops = MoveTemp(Reversed);
	GradientToolEditor::RebuildGradient(Gradient);

	RefreshEverything();
}

void FGradientAssetEditorToolkit::DistributeStopsEvenly()
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || !Gradient->Gradients.IsValidIndex(SelectedGradient))
	{
		return;
	}

	TArray<FGradientStop>& Stops = Gradient->Gradients[SelectedGradient].Stops;
	if (Stops.Num() < 2)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DistributeTransaction", "Distribute Gradient Stops"));
	Gradient->Modify();

	const int32 NumStops = Stops.Num();
	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		Stops[Index].Time = static_cast<float>(Index) / static_cast<float>(NumStops - 1);
	}

	GradientToolEditor::RebuildGradient(Gradient);

	RefreshEverything();
}

void FGradientAssetEditorToolkit::RebuildTexture()
{
	if (UGradientAsset* Gradient = GetGradient())
	{
		GradientToolEditor::RebuildGradient(Gradient);
		RefreshEverything();
	}
}

void FGradientAssetEditorToolkit::OnGradientChanged()
{
	for (const TSharedPtr<SGradientStopBar>& StopBar : StopBars)
	{
		if (StopBar.IsValid())
		{
			StopBar->Refresh();
		}
	}
}

void FGradientAssetEditorToolkit::OnDetailsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshEverything();
}

void FGradientAssetEditorToolkit::RefreshEverything()
{
	const UGradientAsset* Gradient = GetGradient();

	// Recreating rows mid-drag would destroy the widget holding mouse capture, so only do it when
	// the gradient count actually changed.
	if (Gradient && Gradient->Gradients.Num() != StopBars.Num())
	{
		RebuildGradientList();
	}
	else
	{
		OnGradientChanged();
	}

	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

void FGradientAssetEditorToolkit::PostUndo(bool bSuccess)
{
	if (UGradientAsset* Gradient = GetGradient())
	{
		GradientToolEditor::RebuildGradient(Gradient);
	}

	RefreshEverything();
}

void FGradientAssetEditorToolkit::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

#undef LOCTEXT_NAMESPACE
