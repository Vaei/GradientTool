// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "MaterialExpressionIO.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionGradient.generated.h"

class FMaterialCompiler;
class UGradientAsset;

/** Shared plumbing for the nodes that address one gradient inside a GradientAsset atlas. */
UCLASS(abstract, hidecategories=Object)
class GRADIENTTOOL_API UMaterialExpressionGradientBase : public UMaterialExpression
{
	GENERATED_BODY()

public:

	/** Position along the gradient, 0 to 1. */
	UPROPERTY(meta=(RequiredInput="false", ToolTip="Defaults to 'Const Time' if not specified"))
	FExpressionInput Time;

	/** Atlas texture to read, overriding the one on Gradient. Feed this a Texture Object Parameter to let instances swap atlases. */
	UPROPERTY(meta=(RequiredInput="false", ToolTip="Defaults to the texture baked by 'Gradient' if not specified"))
	FExpressionInput TextureObject;

	/** Row to read, overriding the one Gradient Name resolves to. Feed this a Gradient Row Parameter to let instances swap gradients. */
	UPROPERTY(meta=(RequiredInput="false", ToolTip="Defaults to the row of 'Gradient Name' if not specified"))
	FExpressionInput Row;

	/** The atlas to read from. */
	UPROPERTY(EditAnywhere, Category=Gradient)
	TObjectPtr<UGradientAsset> Gradient;

	/** Which gradient within the atlas. */
	UPROPERTY(EditAnywhere, Category=Gradient, meta=(GetOptions="GetGradientNameOptions"))
	FName GradientName;

	/** Used when Time is not connected. */
	UPROPERTY(EditAnywhere, Category=Gradient, meta=(OverridingInputProperty="Time", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float ConstTime = 0.f;

	UFUNCTION()
	TArray<FName> GetGradientNameOptions() const;

	//~ Begin UMaterialExpression Interface
	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }

#if WITH_EDITOR
	virtual EMaterialValueType GetInputValueType(int32 InputIndex) override;
	virtual FName GetInputName(int32 InputIndex) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UMaterialExpression Interface

protected:

#if WITH_EDITOR
	/** Emits the atlas texture object, raising a compile error and returning INDEX_NONE if it cannot. */
	int32 CompileTextureObject(FMaterialCompiler* Compiler, int32& OutTextureReferenceIndex);

	/** Emits the row to read, from the Row input if connected and the gradient name otherwise. */
	int32 CompileRow(FMaterialCompiler* Compiler);

	/** Emits the UV that reads the row, taking the atlas dimensions from the live texture. */
	int32 CompileCoordinate(FMaterialCompiler* Compiler, int32 TextureIndex, int32 RowIndex);

	FString GetGradientCaption() const;
#endif
};

/** Reads a colour out of a gradient atlas. */
UCLASS(collapsecategories, hidecategories=Object)
class GRADIENTTOOL_API UMaterialExpressionSampleGradient : public UMaterialExpressionGradientBase
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual void GetExpressionToolTip(TArray<FString>& OutToolTip) override;
	//~ End UMaterialExpression Interface
#endif
};

/** The UV that reads a gradient out of an atlas, for feeding a texture sample built by hand. */
UCLASS(collapsecategories, hidecategories=Object)
class GRADIENTTOOL_API UMaterialExpressionGradientCoordinate : public UMaterialExpressionGradientBase
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual void GetExpressionToolTip(TArray<FString>& OutToolTip) override;
	//~ End UMaterialExpression Interface
#endif
};
