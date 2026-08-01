// Copyright (c) Jared Taylor

#include "MaterialExpressionGradient.h"

#include "GradientAsset.h"

#include "Engine/Texture2D.h"
#include "MaterialCompiler.h"
#include "MaterialShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MaterialExpressionGradient)

#define LOCTEXT_NAMESPACE "MaterialExpressionGradient"

namespace GradientTool
{
	static FText GetMenuCategory()
	{
		return LOCTEXT("MenuCategory", "Gradient");
	}

	/** The bake is always uncompressed and non-sRGB, whichever format it used. */
	static constexpr EMaterialSamplerType AtlasSamplerType = SAMPLERTYPE_LinearColor;
}

TArray<FName> UMaterialExpressionGradientBase::GetGradientNameOptions() const
{
	return Gradient ? Gradient->GetGradientNames() : TArray<FName>();
}

UObject* UMaterialExpressionGradientBase::GetReferencedTexture() const
{
	return Gradient ? ToRawPtr(Gradient->Texture) : nullptr;
}

#if WITH_EDITOR

EMaterialValueType UMaterialExpressionGradientBase::GetInputValueType(int32 InputIndex)
{
	return (GetInput(InputIndex) == &TextureObject) ? MCT_Texture2D : MCT_Float;
}

FName UMaterialExpressionGradientBase::GetInputName(int32 InputIndex) const
{
	// The default strips the property name down to "Tex", which reads as nothing in particular.
	return (GetInput(InputIndex) == &TextureObject) ? FName(TEXT("Atlas")) : Super::GetInputName(InputIndex);
}

void UMaterialExpressionGradientBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMaterialExpressionGradientBase, Gradient))
	{
		if (Gradient && Gradient->IndexOfGradient(GradientName) == INDEX_NONE)
		{
			GradientName = Gradient->NumGradients() > 0 ? Gradient->Gradients[0].Name : NAME_None;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

int32 UMaterialExpressionGradientBase::CompileTextureObject(FMaterialCompiler* Compiler, int32& OutTextureReferenceIndex)
{
	if (TextureObject.GetTracedInput().Expression)
	{
		return TextureObject.Compile(Compiler);
	}

	if (!Gradient)
	{
		return Compiler->Errorf(TEXT("Gradient node has no gradient asset assigned"));
	}

	if (!Gradient->Texture)
	{
		return Compiler->Errorf(TEXT("Gradient '%s' has no baked texture"), *Gradient->GetName());
	}

	return Compiler->Texture(Gradient->Texture, OutTextureReferenceIndex, GradientTool::AtlasSamplerType, SSM_FromTextureAsset, TMVM_None);
}

int32 UMaterialExpressionGradientBase::CompileRow(FMaterialCompiler* Compiler)
{
	if (Row.GetTracedInput().Expression)
	{
		return Row.Compile(Compiler);
	}

	const int32 RowIndex = Gradient ? Gradient->IndexOfGradient(GradientName) : INDEX_NONE;
	if (RowIndex == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("Gradient '%s' has no entry named '%s'"),
			Gradient ? *Gradient->GetName() : TEXT("None"), *GradientName.ToString());
	}

	return Compiler->Constant(static_cast<float>(RowIndex));
}

int32 UMaterialExpressionGradientBase::CompileCoordinate(FMaterialCompiler* Compiler, int32 TextureIndex, int32 RowIndex)
{
	const int32 TimeIndex = Time.GetTracedInput().Expression ? Time.Compile(Compiler) : Compiler->Constant(ConstTime);
	if (TimeIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	// Taking the size from the texture rather than the asset means adding a gradient to the atlas
	// does not leave already compiled materials sampling the wrong row.
	const int32 Size = Compiler->TextureProperty(TextureIndex, TMTM_TextureSize);
	const int32 SizeX = Compiler->ComponentMask(Size, 1, 0, 0, 0);
	const int32 SizeY = Compiler->ComponentMask(Size, 0, 1, 0, 0);

	// Time 0 and 1 land on the centres of the first and last texels, so a sample reproduces Evaluate.
	const int32 U = Compiler->Div(
		Compiler->Add(Compiler->Mul(Compiler->ComponentMask(TimeIndex, 1, 0, 0, 0),
			Compiler->Sub(SizeX, Compiler->Constant(1.f))), Compiler->Constant(0.5f)), SizeX);

	const int32 V = Compiler->Div(Compiler->Add(RowIndex, Compiler->Constant(0.5f)), SizeY);

	return Compiler->AppendVector(U, V);
}

FString UMaterialExpressionGradientBase::GetGradientCaption() const
{
	if (Row.GetTracedInput().Expression)
	{
		return Gradient ? Gradient->GetName() : TEXT("None");
	}

	if (!Gradient)
	{
		return TEXT("None");
	}

	return FString::Printf(TEXT("%s : %s"), *Gradient->GetName(), *GradientName.ToString());
}

#endif // WITH_EDITOR

UMaterialExpressionSampleGradient::UMaterialExpressionSampleGradient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	MenuCategories.Add(GradientTool::GetMenuCategory());

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("RGB"), 1, 1, 1, 1, 0));
	Outputs.Add(FExpressionOutput(TEXT("R"), 1, 1, 0, 0, 0));
	Outputs.Add(FExpressionOutput(TEXT("G"), 1, 0, 1, 0, 0));
	Outputs.Add(FExpressionOutput(TEXT("B"), 1, 0, 0, 1, 0));
	Outputs.Add(FExpressionOutput(TEXT("A"), 1, 0, 0, 0, 1));
	Outputs.Add(FExpressionOutput(TEXT("RGBA"), 1, 1, 1, 1, 1));

	bShowOutputNameOnPin = true;
	bCollapsed = false;
#endif
}

#if WITH_EDITOR

int32 UMaterialExpressionSampleGradient::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 TextureReferenceIndex = INDEX_NONE;

	const int32 TextureIndex = CompileTextureObject(Compiler, TextureReferenceIndex);
	if (TextureIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 RowIndex = CompileRow(Compiler);
	if (RowIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 CoordinateIndex = CompileCoordinate(Compiler, TextureIndex, RowIndex);
	if (CoordinateIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return Compiler->TextureSample(TextureIndex, CoordinateIndex, GradientTool::AtlasSamplerType,
		INDEX_NONE, INDEX_NONE, TMVM_None, SSM_FromTextureAsset, TGM_None, TextureReferenceIndex);
}

void UMaterialExpressionSampleGradient::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Sample Gradient"));
	OutCaptions.Add(GetGradientCaption());
}

void UMaterialExpressionSampleGradient::GetExpressionToolTip(TArray<FString>& OutToolTip)
{
	ConvertToMultilineToolTip(TEXT("Samples one named gradient out of a Gradient asset's baked atlas."), 40, OutToolTip);
}

#endif // WITH_EDITOR

UMaterialExpressionGradientCoordinate::UMaterialExpressionGradientCoordinate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	MenuCategories.Add(GradientTool::GetMenuCategory());

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("UV"), 1, 1, 1, 0, 0));
	Outputs.Add(FExpressionOutput(TEXT("V"), 1, 0, 1, 0, 0));

	bShowOutputNameOnPin = true;
	bCollapsed = false;
#endif
}

#if WITH_EDITOR

int32 UMaterialExpressionGradientCoordinate::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 TextureReferenceIndex = INDEX_NONE;

	const int32 TextureIndex = CompileTextureObject(Compiler, TextureReferenceIndex);
	if (TextureIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 RowIndex = CompileRow(Compiler);
	if (RowIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return CompileCoordinate(Compiler, TextureIndex, RowIndex);
}

void UMaterialExpressionGradientCoordinate::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Gradient Coordinate"));
	OutCaptions.Add(GetGradientCaption());
}

void UMaterialExpressionGradientCoordinate::GetExpressionToolTip(TArray<FString>& OutToolTip)
{
	ConvertToMultilineToolTip(TEXT("The UV that reads one named gradient out of a Gradient asset's baked atlas."), 40, OutToolTip);
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
