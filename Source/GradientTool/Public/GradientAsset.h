// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GradientTypes.h"
#include "Materials/MaterialEnumeration.h"
#include "GradientAsset.generated.h"

class UTexture2D;

/**
 * One or more colour gradients that keep a baked texture atlas in step with themselves.
 *
 * Each gradient bakes to its own row of a UTexture2D stored as its own asset alongside this one.
 * Every edit rewrites that texture's source data, so what a material samples is always current.
 *
 * The asset doubles as a material enumeration of its gradient names, so a scalar parameter set to
 * Enumeration control can offer those names in a material instance.
 */
UCLASS(BlueprintType)
class GRADIENTTOOL_API UGradientAsset : public UObject, public IMaterialEnumerationProvider
{
	GENERATED_BODY()

public:

	/** One row of the baked texture each, top to bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient, meta=(TitleProperty="Name"))
	TArray<FGradientLayer> Gradients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient, meta=(ClampMin="2", ClampMax="4096", UIMin="2", UIMax="1024"))
	int32 Width = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient)
	EGradientTextureFormat Format = EGradientTextureFormat::HDR;

	/** The baked atlas. Assign this to materials, or point a Sample Gradient node at this asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Gradient)
	TObjectPtr<UTexture2D> Texture;

	UFUNCTION(BlueprintPure, Category=Gradient)
	int32 NumGradients() const { return Gradients.Num(); }

	/** Row the named gradient bakes into, or INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category=Gradient)
	int32 IndexOfGradient(FName GradientName) const;

	UFUNCTION(BlueprintPure, Category=Gradient)
	TArray<FName> GetGradientNames() const;

	UFUNCTION(BlueprintPure, Category=Gradient)
	FLinearColor Evaluate(FName GradientName, float Time) const;

	UFUNCTION(BlueprintPure, Category=Gradient)
	FLinearColor EvaluateByIndex(int32 GradientIndex, float Time) const;

	/** V that lands on the centre of the named gradient's row. Unknown names give the first row. */
	UFUNCTION(BlueprintPure, Category=Gradient)
	float GetGradientV(FName GradientName) const;

	const FGradientLayer* FindGradient(FName GradientName) const;
	FGradientLayer* FindGradient(FName GradientName);

	/** Rows the atlas bakes to. Always at least one, even with no gradients authored. */
	int32 NumRows() const { return FMath::Max(Gradients.Num(), 1); }

	//~ Begin IMaterialEnumerationProvider Interface
	virtual bool ResolveValue(FName EntryName, int32& OutValue, int32 DefaultValue = 0) const override;
	virtual void ForEachEntry(TFunctionRef<void(FName Name, int32 Value)> Iterator) const override;
	//~ End IMaterialEnumerationProvider Interface

	virtual void PostLoad() override;

#if WITH_EDITOR
	/** Raised whenever a gradient is edited. The editor module listens and re-bakes. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGradientAssetChanged, UGradientAsset*);
	static FOnGradientAssetChanged OnGradientAssetChanged;

	/** Raised by a copy that needs a texture of its own. The editor module listens and makes one. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGradientAssetDuplicated, UGradientAsset*);
	static FOnGradientAssetDuplicated OnGradientAssetDuplicated;

	virtual void PostDuplicate(bool bDuplicateForPIE) override;

	/** Rewrites Target's source data from the gradients and applies the LUT texture settings. */
	void BakeInto(UTexture2D* Target);

	/** Names every gradient, renaming duplicates until each is distinct. */
	void EnsureUniqueGradientNames();

	/** BaseName, or BaseName with a numeric suffix if another gradient already has it. */
	FName MakeUniqueGradientName(FName BaseName, int32 IgnoreIndex = INDEX_NONE) const;

	/** Everything that affects the baked pixels, so a load can tell whether the texture is current. */
	uint32 ComputeGradientHash() const;

	/** Just the row assignment. Materials that address a row by name go stale when this changes. */
	uint32 ComputeLayoutHash() const;

	uint32 GetCachedLayoutHash() const { return CachedLayoutHash; }

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	UPROPERTY()
	TArray<FGradientStop> Stops_DEPRECATED;

	UPROPERTY()
	EGradientBlendSpace BlendSpace_DEPRECATED = EGradientBlendSpace::Linear;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32 CachedGradientHash = 0;

	UPROPERTY()
	uint32 CachedLayoutHash = 0;
#endif
};
