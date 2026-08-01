// Copyright (c) Jared Taylor

#include "AssetDefinition_GradientAsset.h"

#include "GradientAsset.h"
#include "GradientAssetEditorToolkit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_GradientAsset)

#define LOCTEXT_NAMESPACE "AssetDefinition_GradientAsset"

FText UAssetDefinition_GradientAsset::GetAssetDisplayName() const
{
	return LOCTEXT("AssetDisplayName", "Gradient");
}

FLinearColor UAssetDefinition_GradientAsset::GetAssetColor() const
{
	return FLinearColor(FColor(192, 64, 64));
}

TSoftClassPtr<UObject> UAssetDefinition_GradientAsset::GetAssetClass() const
{
	return UGradientAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_GradientAsset::GetAssetCategories() const
{
	static const auto Categories = { EAssetCategoryPaths::Texture };
	return Categories;
}

EAssetCommandResult UAssetDefinition_GradientAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	if (OpenArgs.OpenMethod != EAssetOpenMethod::Edit)
	{
		return EAssetCommandResult::Unhandled;
	}

	for (UGradientAsset* Gradient : OpenArgs.LoadObjects<UGradientAsset>())
	{
		const TSharedRef<FGradientAssetEditorToolkit> Toolkit = MakeShared<FGradientAssetEditorToolkit>();
		Toolkit->Initialize(Gradient, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
