// Copyright (c) Jared Taylor

#include "GradientTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GradientTypes)

namespace GradientTool
{
namespace Private
{
	float LinearToSrgbChannel(float C)
	{
		return (C <= 0.0031308f) ? C * 12.92f : 1.055f * FMath::Pow(C, 1.f / 2.4f) - 0.055f;
	}

	float SrgbToLinearChannel(float C)
	{
		return (C <= 0.04045f) ? C / 12.92f : FMath::Pow((C + 0.055f) / 1.055f, 2.4f);
	}

	float SignedCbrt(float X)
	{
		return (X < 0.f) ? -FMath::Pow(-X, 1.f / 3.f) : FMath::Pow(X, 1.f / 3.f);
	}

	FVector3f LinearToOkLab(const FVector3f& C)
	{
		const float L = 0.4122214708f * C.X + 0.5363325363f * C.Y + 0.0514459929f * C.Z;
		const float M = 0.2119034982f * C.X + 0.6806995451f * C.Y + 0.1073969566f * C.Z;
		const float S = 0.0883024619f * C.X + 0.2817188376f * C.Y + 0.6299787005f * C.Z;

		const float L_ = SignedCbrt(L);
		const float M_ = SignedCbrt(M);
		const float S_ = SignedCbrt(S);

		return FVector3f(
			0.2104542553f * L_ + 0.7936177850f * M_ - 0.0040720468f * S_,
			1.9779984951f * L_ - 2.4285922050f * M_ + 0.4505937099f * S_,
			0.0259040371f * L_ + 0.7827717662f * M_ - 0.8086757660f * S_);
	}

	FVector3f OkLabToLinear(const FVector3f& C)
	{
		const float L_ = C.X + 0.3963377774f * C.Y + 0.2158037573f * C.Z;
		const float M_ = C.X - 0.1055613458f * C.Y - 0.0638541728f * C.Z;
		const float S_ = C.X - 0.0894841775f * C.Y - 1.2914855480f * C.Z;

		const float L = L_ * L_ * L_;
		const float M = M_ * M_ * M_;
		const float S = S_ * S_ * S_;

		return FVector3f(
			 4.0767416621f * L - 3.3077115913f * M + 0.2309699292f * S,
			-1.2684380046f * L + 2.6097574011f * M - 0.3413193965f * S,
			-0.0041960863f * L - 0.7034186147f * M + 1.7076147010f * S);
	}

	FVector3f ToBlendSpace(const FLinearColor& Color, EGradientBlendSpace Space)
	{
		switch (Space)
		{
		case EGradientBlendSpace::sRGB:
			return FVector3f(LinearToSrgbChannel(Color.R), LinearToSrgbChannel(Color.G), LinearToSrgbChannel(Color.B));

		case EGradientBlendSpace::HSV:
			{
				const FLinearColor HSV = Color.LinearRGBToHSV();
				return FVector3f(HSV.R, HSV.G, HSV.B);
			}

		case EGradientBlendSpace::OkLab:
			return LinearToOkLab(FVector3f(Color.R, Color.G, Color.B));

		case EGradientBlendSpace::Linear:
		default:
			return FVector3f(Color.R, Color.G, Color.B);
		}
	}

	FLinearColor FromBlendSpace(const FVector3f& Value, float Alpha, EGradientBlendSpace Space)
	{
		switch (Space)
		{
		case EGradientBlendSpace::sRGB:
			return FLinearColor(SrgbToLinearChannel(Value.X), SrgbToLinearChannel(Value.Y), SrgbToLinearChannel(Value.Z), Alpha);

		case EGradientBlendSpace::HSV:
			{
				const float Hue = FMath::Fmod(FMath::Fmod(Value.X, 360.f) + 360.f, 360.f);
				FLinearColor Result = FLinearColor(Hue, Value.Y, Value.Z, Alpha).HSVToLinearRGB();
				Result.A = Alpha;
				return Result;
			}

		case EGradientBlendSpace::OkLab:
			{
				const FVector3f RGB = OkLabToLinear(Value);
				return FLinearColor(RGB.X, RGB.Y, RGB.Z, Alpha);
			}

		case EGradientBlendSpace::Linear:
		default:
			return FLinearColor(Value.X, Value.Y, Value.Z, Alpha);
		}
	}

	/** Hue is cyclic, so shift each successive hue onto the branch nearest its predecessor before interpolating. */
	void UnwrapHues(TArrayView<FVector3f> Points)
	{
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			if (Points[Index].Y <= UE_KINDA_SMALL_NUMBER)
			{
				Points[Index].X = Points[Index - 1].X;
				continue;
			}

			if (Points[Index - 1].Y <= UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float Delta = Points[Index].X - Points[Index - 1].X;
			Points[Index].X -= 360.f * FMath::RoundToFloat(Delta / 360.f);
		}
	}

	template<typename T>
	T CatmullRom(const T& P0, const T& P1, const T& P2, const T& P3, float T0)
	{
		const float T2 = T0 * T0;
		const float T3 = T2 * T0;
		return 0.5f * ((2.f * P1)
			+ (P2 - P0) * T0
			+ (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2
			+ (P3 - 3.f * P2 + 3.f * P1 - P0) * T3);
	}
}

FLinearColor EvaluateGradient(TConstArrayView<FGradientStop> Stops, EGradientBlendSpace BlendSpace, float Time)
{
	using namespace Private;

	const int32 NumStops = Stops.Num();
	if (NumStops == 0)
	{
		return FLinearColor::Black;
	}
	if (NumStops == 1)
	{
		return Stops[0].Color;
	}

	int32 Upper = 0;
	while (Upper < NumStops && Stops[Upper].Time <= Time)
	{
		++Upper;
	}

	if (Upper == 0)
	{
		return Stops[0].Color;
	}
	if (Upper >= NumStops)
	{
		return Stops.Last().Color;
	}

	const int32 LowerIndex = Upper - 1;
	const FGradientStop& Lower = Stops[LowerIndex];
	const FGradientStop& Higher = Stops[Upper];

	const float Span = Higher.Time - Lower.Time;
	float Alpha = (Span > UE_SMALL_NUMBER) ? (Time - Lower.Time) / Span : 0.f;

	switch (Higher.Interp)
	{
	case EGradientInterp::Constant:
		return Lower.Color;

	case EGradientInterp::Ease:
		Alpha = Alpha * Alpha * (3.f - 2.f * Alpha);
		break;

	case EGradientInterp::Cubic:
		{
			const int32 PrevIndex = FMath::Max(LowerIndex - 1, 0);
			const int32 NextIndex = FMath::Min(Upper + 1, NumStops - 1);

			FVector3f Points[4] =
			{
				ToBlendSpace(Stops[PrevIndex].Color, BlendSpace),
				ToBlendSpace(Lower.Color, BlendSpace),
				ToBlendSpace(Higher.Color, BlendSpace),
				ToBlendSpace(Stops[NextIndex].Color, BlendSpace),
			};

			if (BlendSpace == EGradientBlendSpace::HSV)
			{
				UnwrapHues(Points);
			}

			const float OutAlpha = CatmullRom(Stops[PrevIndex].Color.A, Lower.Color.A, Higher.Color.A, Stops[NextIndex].Color.A, Alpha);
			return FromBlendSpace(CatmullRom(Points[0], Points[1], Points[2], Points[3], Alpha), OutAlpha, BlendSpace);
		}

	case EGradientInterp::Linear:
	default:
		break;
	}

	FVector3f Points[2] =
	{
		ToBlendSpace(Lower.Color, BlendSpace),
		ToBlendSpace(Higher.Color, BlendSpace),
	};

	if (BlendSpace == EGradientBlendSpace::HSV)
	{
		UnwrapHues(Points);
	}

	const float OutAlpha = FMath::Lerp(Lower.Color.A, Higher.Color.A, Alpha);
	return FromBlendSpace(FMath::Lerp(Points[0], Points[1], Alpha), OutAlpha, BlendSpace);
}

void SampleGradient(TConstArrayView<FGradientStop> Stops, EGradientBlendSpace BlendSpace, int32 NumSamples, TArray<FLinearColor>& OutColors)
{
	NumSamples = FMath::Max(NumSamples, 2);
	OutColors.SetNumUninitialized(NumSamples);

	const float Step = 1.f / static_cast<float>(NumSamples - 1);
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		OutColors[Index] = EvaluateGradient(Stops, BlendSpace, Index * Step);
	}
}
}
