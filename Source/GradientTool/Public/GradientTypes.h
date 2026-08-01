// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "GradientTypes.generated.h"

/** How the gradient reaches a stop from the one before it. */
UENUM(BlueprintType)
enum class EGradientInterp : uint8
{
	/** Straight interpolation from the previous stop. */
	Linear,

	/** Hold the previous stop's colour, then jump at this stop. */
	Constant,

	/** Smoothstep, easing out of the previous stop and into this one. */
	Ease,

	/** Catmull-Rom through the neighbouring stops. */
	Cubic,
};

/** The space colours are interpolated in. Only affects the path between stops, never the stops themselves. */
UENUM(BlueprintType)
enum class EGradientBlendSpace : uint8
{
	/** Blend in linear RGB. Physically correct, but midpoints between saturated hues go grey. */
	Linear,

	/** Blend in sRGB-encoded space. Matches how most DCC tools draw gradients. */
	sRGB,

	/** Blend hue along the shortest arc, keeping saturation through the midpoint. */
	HSV,

	/** Perceptually uniform. Even lightness ramps and no muddy midpoints. */
	OkLab,
};

/** Source format the gradient bakes into. */
UENUM(BlueprintType)
enum class EGradientTextureFormat : uint8
{
	/** RGBA16F, uncompressed. Supports values outside 0-1. */
	HDR,

	/** BGRA8, uncompressed. A quarter the memory, clamped to 0-1. */
	LDR,
};

USTRUCT(BlueprintType)
struct FGradientStop
{
	GENERATED_BODY()

	FGradientStop() = default;

	FGradientStop(float InTime, const FLinearColor& InColor, EGradientInterp InInterp = EGradientInterp::Linear)
		: Time(InTime)
		, Color(InColor)
		, Interp(InInterp)
	{}

	/** Position along the gradient. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient, meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient)
	FLinearColor Color = FLinearColor::White;

	/** How the gradient reaches this stop from the one before it. Ignored on the first stop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gradient)
	EGradientInterp Interp = EGradientInterp::Linear;
};

namespace GradientTool
{
	/** Stops must be sorted ascending by Time. Times outside the first and last stop clamp. */
	GRADIENTTOOL_API FLinearColor EvaluateGradient(TConstArrayView<FGradientStop> Stops, EGradientBlendSpace BlendSpace, float Time);

	/** Fills OutColors with NumSamples evenly spaced evaluations spanning 0 to 1 inclusive. */
	GRADIENTTOOL_API void SampleGradient(TConstArrayView<FGradientStop> Stops, EGradientBlendSpace BlendSpace, int32 NumSamples, TArray<FLinearColor>& OutColors);
}
