// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"

class UGradientAsset;
class UTexture2D;
struct FAssetData;

namespace GradientToolEditor
{
	/** The name the companion texture should carry for this gradient. */
	FString MakeTextureAssetName(const UGradientAsset* Gradient);

	/** Returns the gradient's companion texture, creating it beside the gradient if it does not exist yet. */
	UTexture2D* EnsureTexture(UGradientAsset* Gradient);

	/** Ensures the texture exists, re-bakes it from the stops, and dirties both packages. */
	void RebuildGradient(UGradientAsset* Gradient);

	/** Keeps a companion texture named after the gradient that owns it. */
	void HandleAssetRenamed(const FAssetData& NewAssetData, const FString& OldObjectPath);

	/** Regenerates the companion texture for a gradient that loaded without one. */
	void HandleAssetLoaded(UObject* Asset);
}
