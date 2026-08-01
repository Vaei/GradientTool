// Copyright (c) Jared Taylor

#include "GradientTextureBuilder.h"

#include "GradientAsset.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Containers/Ticker.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GradientToolEditor
{
namespace Private
{
	/** RenameAssets re-enters OnAssetRenamed; without this the companion rename would chase itself. */
	static bool bRenamingCompanionTexture = false;

	void SaveTexturePackage(UTexture2D* Texture)
	{
		UPackage* Package = Texture->GetPackage();
		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		UPackage::SavePackage(Package, Texture, *FileName, SaveArgs);
	}
}

FString MakeTextureAssetName(const UGradientAsset* Gradient)
{
	FString Name = Gradient->GetName();
	Name.RemoveFromStart(TEXT("GR_"));
	return TEXT("T_") + Name;
}

UTexture2D* EnsureTexture(UGradientAsset* Gradient)
{
	if (!Gradient)
	{
		return nullptr;
	}

	if (Gradient->Texture)
	{
		return Gradient->Texture;
	}

	const FString PackagePath = FPackageName::GetLongPackagePath(Gradient->GetOutermost()->GetName());

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FString TexturePackageName;
	FString TextureAssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / MakeTextureAssetName(Gradient), FString(), TexturePackageName, TextureAssetName);

	UPackage* TexturePackage = CreatePackage(*TexturePackageName);
	if (!TexturePackage)
	{
		return nullptr;
	}

	UTexture2D* Texture = NewObject<UTexture2D>(TexturePackage, *TextureAssetName, RF_Public | RF_Standalone);

	Gradient->Modify();
	Gradient->Texture = Texture;

	FAssetRegistryModule::AssetCreated(Texture);
	TexturePackage->MarkPackageDirty();

	return Texture;
}

void RebuildGradient(UGradientAsset* Gradient)
{
	const bool bTextureExisted = Gradient && Gradient->Texture != nullptr;

	UTexture2D* Texture = EnsureTexture(Gradient);
	if (!Texture)
	{
		return;
	}

	Gradient->BakeInto(Texture);

	// A brand new texture goes to disk straight away, or the gradient's reference to it would
	// dangle for anyone who saved the gradient on its own.
	if (!bTextureExisted)
	{
		Private::SaveTexturePackage(Texture);
	}

	Gradient->MarkPackageDirty();
	Texture->MarkPackageDirty();
}

void HandleAssetLoaded(UObject* Asset)
{
	UGradientAsset* Gradient = Cast<UGradientAsset>(Asset);
	if (!Gradient || Gradient->Texture)
	{
		return;
	}

	// Creating and saving the replacement package has to wait until loading has finished.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakGradient = TWeakObjectPtr<UGradientAsset>(Gradient)](float)
		{
			if (UGradientAsset* LoadedGradient = WeakGradient.Get())
			{
				RebuildGradient(LoadedGradient);
			}
			return false;
		}), 0.f);
}

void HandleAssetRenamed(const FAssetData& NewAssetData, const FString& OldObjectPath)
{
	if (Private::bRenamingCompanionTexture)
	{
		return;
	}

	UGradientAsset* Gradient = Cast<UGradientAsset>(NewAssetData.FastGetAsset(false));
	if (!Gradient || !Gradient->Texture)
	{
		return;
	}

	const FString DesiredPath = FPackageName::GetLongPackagePath(Gradient->GetOutermost()->GetName());
	const FString DesiredName = MakeTextureAssetName(Gradient);

	UTexture2D* Texture = Gradient->Texture;
	if (Texture->GetName() == DesiredName && FPackageName::GetLongPackagePath(Texture->GetOutermost()->GetName()) == DesiredPath)
	{
		return;
	}

	// The registry is mid-rename here, so let it finish before starting another one.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakTexture = TWeakObjectPtr<UTexture2D>(Texture), DesiredPath, DesiredName](float)
		{
			UTexture2D* RenameTarget = WeakTexture.Get();
			if (!RenameTarget)
			{
				return false;
			}

			TGuardValue<bool> Guard(Private::bRenamingCompanionTexture, true);

			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

			TArray<FAssetRenameData> Renames;
			Renames.Emplace(RenameTarget, DesiredPath, DesiredName);
			AssetTools.RenameAssets(Renames);

			return false;
		}), 0.f);
}
}
