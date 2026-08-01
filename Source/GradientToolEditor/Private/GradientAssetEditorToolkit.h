// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SGradientStopBar;
class UGradientAsset;

class FGradientAssetEditorToolkit : public FAssetEditorToolkit, public FEditorUndoClient
{
public:

	virtual ~FGradientAssetEditorToolkit() override;

	void Initialize(UGradientAsset* InGradient, EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost);

	//~ Begin FAssetEditorToolkit Interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	//~ End FAssetEditorToolkit Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface

private:

	static const FName GradientTabId;
	static const FName DetailsTabId;

	TSharedRef<SDockTab> SpawnTab_Gradient(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);

	void ExtendToolbar();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);

	void ReverseStops();
	void DistributeStopsEvenly();
	void RebuildTexture();

	void OnGradientChanged();
	void OnDetailsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);
	void RefreshEverything();

	UGradientAsset* GetGradient() const;

	TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory;
	TSharedPtr<SGradientStopBar> StopBar;
	TSharedPtr<IDetailsView> DetailsView;
};
