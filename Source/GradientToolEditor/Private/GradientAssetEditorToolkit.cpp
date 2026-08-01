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
#include "Widgets/Layout/SBorder.h"

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

	StopBar = SNew(SGradientStopBar, InGradient)
		.OnGradientChanged(FSimpleDelegate::CreateSP(this, &FGradientAssetEditorToolkit::OnGradientChanged));

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("GradientAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(GradientTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("GradientAssetEditor"), Layout,
		/*bCreateDefaultStandaloneMenu*/true, /*bCreateDefaultToolbar*/true, InGradient);

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
		.SetDisplayName(LOCTEXT("GradientTab", "Gradient"))
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
		.Label(LOCTEXT("GradientTab", "Gradient"))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(12.f)
			.VAlign(VAlign_Center)
			[
				StopBar.ToSharedRef()
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
	ToolbarBuilder.BeginSection("Gradient");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::ReverseStops)),
			NAME_None,
			LOCTEXT("Reverse", "Reverse"),
			LOCTEXT("ReverseTooltip", "Mirror every stop about the centre of the gradient."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FlipHorizontal"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::DistributeStopsEvenly)),
			NAME_None,
			LOCTEXT("Distribute", "Distribute"),
			LOCTEXT("DistributeTooltip", "Space the stops evenly from 0 to 1, keeping their order and colours."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Normalize"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FGradientAssetEditorToolkit::RebuildTexture)),
			NAME_None,
			LOCTEXT("Rebuild", "Rebuild"),
			LOCTEXT("RebuildTooltip", "Bake the texture again from the stops."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
	}
	ToolbarBuilder.EndSection();
}

void FGradientAssetEditorToolkit::ReverseStops()
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || Gradient->Stops.Num() < 2)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ReverseTransaction", "Reverse Gradient"));
	Gradient->Modify();

	const int32 NumStops = Gradient->Stops.Num();
	TArray<FGradientStop> Reversed;
	Reversed.SetNum(NumStops);

	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		const FGradientStop& Source = Gradient->Stops[NumStops - 1 - Index];
		Reversed[Index].Time = 1.f - Source.Time;
		Reversed[Index].Color = Source.Color;
		Reversed[Index].Interp = (Index == 0) ? EGradientInterp::Linear : Gradient->Stops[NumStops - Index].Interp;
	}

	Gradient->Stops = MoveTemp(Reversed);
	GradientToolEditor::RebuildGradient(Gradient);

	RefreshEverything();
}

void FGradientAssetEditorToolkit::DistributeStopsEvenly()
{
	UGradientAsset* Gradient = GetGradient();
	if (!Gradient || Gradient->Stops.Num() < 2)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DistributeTransaction", "Distribute Gradient Stops"));
	Gradient->Modify();

	const int32 NumStops = Gradient->Stops.Num();
	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		Gradient->Stops[Index].Time = static_cast<float>(Index) / static_cast<float>(NumStops - 1);
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
	if (StopBar.IsValid())
	{
		StopBar->Refresh();
	}
}

void FGradientAssetEditorToolkit::OnDetailsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	OnGradientChanged();
}

void FGradientAssetEditorToolkit::RefreshEverything()
{
	if (StopBar.IsValid())
	{
		StopBar->Refresh();
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
