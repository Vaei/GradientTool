// Copyright (c) Jared Taylor

#include "GradientAssetThumbnailRenderer.h"

#include "GradientAsset.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GradientAssetThumbnailRenderer)

bool UGradientAssetThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	const UGradientAsset* Gradient = Cast<UGradientAsset>(Object);
	return Gradient && Gradient->Texture && Gradient->Texture->GetResource();
}

void UGradientAssetThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	const UGradientAsset* Gradient = Cast<UGradientAsset>(Object);
	if (!Gradient || !Gradient->Texture)
	{
		return;
	}

	const FTextureResource* Resource = Gradient->Texture->GetResource();
	if (!Resource)
	{
		return;
	}

	FCanvasTileItem TileItem(FVector2D(X, Y), Resource, FVector2D(Width, Height), FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Opaque;
	Canvas->DrawItem(TileItem);
}
