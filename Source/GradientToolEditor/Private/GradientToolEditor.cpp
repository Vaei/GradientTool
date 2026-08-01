// Copyright (c) Jared Taylor

#include "GradientToolEditor.h"

#include "GradientAsset.h"
#include "GradientAssetThumbnailRenderer.h"
#include "GradientTextureBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ThumbnailRendering/ThumbnailManager.h"

#define LOCTEXT_NAMESPACE "FGradientToolEditorModule"

void FGradientToolEditorModule::StartupModule()
{
	UGradientAsset::OnGradientAssetChanged.AddStatic(&GradientToolEditor::RebuildGradient);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRenamedHandle = AssetRegistryModule.Get().OnAssetRenamed().AddStatic(&GradientToolEditor::HandleAssetRenamed);

	AssetLoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&GradientToolEditor::HandleAssetLoaded);

	UThumbnailManager::Get().RegisterCustomRenderer(UGradientAsset::StaticClass(), UGradientAssetThumbnailRenderer::StaticClass());
}

void FGradientToolEditorModule::ShutdownModule()
{
	UGradientAsset::OnGradientAssetChanged.RemoveAll(this);

	FCoreUObjectDelegates::OnAssetLoaded.Remove(AssetLoadedHandle);

	if (const FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
	{
		AssetRegistryModule->Get().OnAssetRenamed().Remove(AssetRenamedHandle);
	}

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UGradientAsset::StaticClass());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGradientToolEditorModule, GradientToolEditor)
