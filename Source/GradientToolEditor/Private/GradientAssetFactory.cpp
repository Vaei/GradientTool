// Copyright (c) Jared Taylor

#include "GradientAssetFactory.h"

#include "GradientAsset.h"
#include "GradientTextureBuilder.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GradientAssetFactory)

UGradientAssetFactory::UGradientAssetFactory()
{
	SupportedClass = UGradientAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
}

UObject* UGradientAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UGradientAsset* Gradient = NewObject<UGradientAsset>(InParent, InClass, InName, Flags);

	Gradient->Gradients.Add(FGradientLayer());

	GradientToolEditor::RebuildGradient(Gradient);

	return Gradient;
}
