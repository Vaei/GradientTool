// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GradientTypes.h"
#include "GradientAsset.generated.h"

class UTexture2D;

/**
 * A colour gradient that keeps a baked texture in step with itself.
 *
 * The gradient owns a UTexture2D stored as its own asset alongside this one. Every edit rewrites that
 * texture's source data, so what a material samples is always the current gradient.
 */
UCLASS(BlueprintType)
class GRADIENTTOOL_API UGradientAsset : public UObject
{
	GENERATED_BODY()

public:

	/** Sorted ascending by Time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient, meta=(TitleProperty="Time"))
	TArray<FGradientStop> Stops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient)
	EGradientBlendSpace BlendSpace = EGradientBlendSpace::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient, meta=(ClampMin="2", ClampMax="4096", UIMin="2", UIMax="1024"))
	int32 Width = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient)
	EGradientTextureFormat Format = EGradientTextureFormat::HDR;

	/** The baked gradient. Assign this to materials. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Gradient)
	TObjectPtr<UTexture2D> Texture;

	UFUNCTION(BlueprintPure, Category=Gradient)
	FLinearColor Evaluate(float Time) const;

	/** Fills OutColors with NumSamples evenly spaced evaluations spanning 0 to 1 inclusive. */
	void Sample(int32 NumSamples, TArray<FLinearColor>& OutColors) const;

	virtual void PostLoad() override;

#if WITH_EDITOR
	/** Raised whenever a gradient is edited. The editor module listens and re-bakes. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGradientAssetChanged, UGradientAsset*);
	static FOnGradientAssetChanged OnGradientAssetChanged;

	/** Rewrites Target's source data from the stops and applies the LUT texture settings. */
	void BakeInto(UTexture2D* Target);

	/** Sorts Stops ascending by Time. Returns where the stop at TrackedIndex ended up. */
	int32 SortStops(int32 TrackedIndex = INDEX_NONE);

	/** Everything that affects the baked pixels, so a load can tell whether the texture is current. */
	uint32 ComputeGradientHash() const;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32 CachedGradientHash = 0;
#endif
};
