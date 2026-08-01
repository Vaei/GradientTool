// Copyright (c) Jared Taylor

#include "GradientAsset.h"

#include "Engine/Texture2D.h"
#include "HAL/LowLevelMemTracker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GradientAsset)

namespace GradientTool
{
	/** Bump when the evaluator changes, so existing assets re-bake on load. */
	static constexpr uint32 BakeVersion = 1;
}

#if WITH_EDITOR
UGradientAsset::FOnGradientAssetChanged UGradientAsset::OnGradientAssetChanged;
#endif

FLinearColor UGradientAsset::Evaluate(float Time) const
{
	return GradientTool::EvaluateGradient(Stops, BlendSpace, Time);
}

void UGradientAsset::Sample(int32 NumSamples, TArray<FLinearColor>& OutColors) const
{
	GradientTool::SampleGradient(Stops, BlendSpace, NumSamples, OutColors);
}

void UGradientAsset::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (Texture && CachedGradientHash != ComputeGradientHash())
	{
		Texture->ConditionalPostLoad();
		BakeInto(Texture);
	}
#endif
}

#if WITH_EDITOR

void UGradientAsset::BakeInto(UTexture2D* Target)
{
	if (!Target)
	{
		return;
	}

	LLM_SCOPE(ELLMTag::Textures);

	Width = FMath::Clamp(Width, 2, 4096);
	SortStops();

	const bool bHDR = (Format == EGradientTextureFormat::HDR);
	const ETextureSourceFormat SourceFormat = bHDR ? TSF_RGBA16F : TSF_BGRA8;

	Target->PreEditChange(nullptr);

	Target->SRGB = false;
	Target->AddressX = TA_Clamp;
	Target->AddressY = TA_Clamp;
	Target->Filter = TF_Bilinear;
	Target->LODGroup = TEXTUREGROUP_ColorLookupTable;
	Target->CompressionSettings = bHDR ? TC_HDR : TC_VectorDisplacementmap;
	Target->MipGenSettings = TMGS_NoMipmaps;

	if (Target->Source.GetSizeX() != Width || Target->Source.GetSizeY() != 1 || Target->Source.GetFormat() != SourceFormat)
	{
		Target->Source.Init(Width, 1, /*NumSlices*/1, /*NumMips*/1, SourceFormat);
	}

	uint8* MipData = Target->Source.LockMip(0);
	{
		const float Step = 1.f / static_cast<float>(Width - 1);

		if (bHDR)
		{
			TArrayView<FFloat16Color> Dest(reinterpret_cast<FFloat16Color*>(MipData), Width);
			check(Target->Source.CalcMipSize(0) == Dest.NumBytes());

			for (int32 X = 0; X < Width; ++X)
			{
				Dest[X] = Evaluate(X * Step);
			}
		}
		else
		{
			TArrayView<FColor> Dest(reinterpret_cast<FColor*>(MipData), Width);
			check(Target->Source.CalcMipSize(0) == Dest.NumBytes());

			for (int32 X = 0; X < Width; ++X)
			{
				Dest[X] = Evaluate(X * Step).ToFColor(false);
			}
		}
	}
	Target->Source.UnlockMip(0);

	CachedGradientHash = ComputeGradientHash();

	Target->PostEditChange();
}

int32 UGradientAsset::SortStops(int32 TrackedIndex)
{
	const int32 NumStops = Stops.Num();
	if (NumStops < 2)
	{
		return TrackedIndex;
	}

	TArray<int32> Order;
	Order.Reserve(NumStops);
	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		Order.Add(Index);
	}

	Order.StableSort([this](int32 A, int32 B) { return Stops[A].Time < Stops[B].Time; });

	bool bReordered = false;
	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		if (Order[Index] != Index)
		{
			bReordered = true;
			break;
		}
	}

	if (!bReordered)
	{
		return TrackedIndex;
	}

	TArray<FGradientStop> Sorted;
	Sorted.Reserve(NumStops);

	int32 NewTrackedIndex = INDEX_NONE;
	for (int32 Index = 0; Index < NumStops; ++Index)
	{
		if (Order[Index] == TrackedIndex)
		{
			NewTrackedIndex = Index;
		}
		Sorted.Add(Stops[Order[Index]]);
	}

	Stops = MoveTemp(Sorted);
	return NewTrackedIndex;
}

uint32 UGradientAsset::ComputeGradientHash() const
{
	uint32 Hash = ::GetTypeHash(GradientTool::BakeVersion);
	Hash = HashCombine(Hash, ::GetTypeHash(Width));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Format)));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(BlendSpace)));
	Hash = HashCombine(Hash, ::GetTypeHash(Stops.Num()));

	for (const FGradientStop& Stop : Stops)
	{
		Hash = HashCombine(Hash, ::GetTypeHash(Stop.Time));
		Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.R));
		Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.G));
		Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.B));
		Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.A));
		Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Stop.Interp)));
	}

	return Hash;
}

void UGradientAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	OnGradientAssetChanged.Broadcast(this);
}

#endif // WITH_EDITOR
