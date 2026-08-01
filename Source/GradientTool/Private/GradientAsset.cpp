// Copyright (c) Jared Taylor

#include "GradientAsset.h"

#include "Engine/Texture2D.h"
#include "HAL/LowLevelMemTracker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GradientAsset)

namespace GradientTool
{
	/** Bump when the evaluator changes, so existing assets re-bake on load. */
	static constexpr uint32 BakeVersion = 2;
}

#if WITH_EDITOR
UGradientAsset::FOnGradientAssetChanged UGradientAsset::OnGradientAssetChanged;
UGradientAsset::FOnGradientAssetDuplicated UGradientAsset::OnGradientAssetDuplicated;
#endif

int32 UGradientAsset::IndexOfGradient(FName GradientName) const
{
	return Gradients.IndexOfByPredicate([GradientName](const FGradientLayer& Layer) { return Layer.Name == GradientName; });
}

TArray<FName> UGradientAsset::GetGradientNames() const
{
	TArray<FName> Names;
	Names.Reserve(Gradients.Num());

	for (const FGradientLayer& Layer : Gradients)
	{
		Names.Add(Layer.Name);
	}

	return Names;
}

FLinearColor UGradientAsset::Evaluate(FName GradientName, float Time) const
{
	return EvaluateByIndex(IndexOfGradient(GradientName), Time);
}

FLinearColor UGradientAsset::EvaluateByIndex(int32 GradientIndex, float Time) const
{
	return Gradients.IsValidIndex(GradientIndex) ? Gradients[GradientIndex].Evaluate(Time) : FLinearColor::Black;
}

float UGradientAsset::GetGradientV(FName GradientName) const
{
	const int32 Index = FMath::Max(IndexOfGradient(GradientName), 0);
	return (static_cast<float>(Index) + 0.5f) / static_cast<float>(NumRows());
}

const FGradientLayer* UGradientAsset::FindGradient(FName GradientName) const
{
	const int32 Index = IndexOfGradient(GradientName);
	return Gradients.IsValidIndex(Index) ? &Gradients[Index] : nullptr;
}

FGradientLayer* UGradientAsset::FindGradient(FName GradientName)
{
	const int32 Index = IndexOfGradient(GradientName);
	return Gradients.IsValidIndex(Index) ? &Gradients[Index] : nullptr;
}

bool UGradientAsset::ResolveValue(FName EntryName, int32& OutValue, int32 DefaultValue) const
{
	const int32 Index = IndexOfGradient(EntryName);
	OutValue = (Index != INDEX_NONE) ? Index : DefaultValue;
	return Index != INDEX_NONE;
}

void UGradientAsset::ForEachEntry(TFunctionRef<void(FName Name, int32 Value)> Iterator) const
{
	for (int32 Index = 0; Index < Gradients.Num(); ++Index)
	{
		Iterator(Gradients[Index].Name, Index);
	}
}

void UGradientAsset::PostLoad()
{
	Super::PostLoad();

	if (Gradients.Num() == 0 && Stops_DEPRECATED.Num() > 0)
	{
		FGradientLayer& Layer = Gradients.AddDefaulted_GetRef();
		Layer.Stops = MoveTemp(Stops_DEPRECATED);
		Layer.BlendSpace = BlendSpace_DEPRECATED;

		Stops_DEPRECATED.Reset();
	}

#if WITH_EDITOR
	if (Texture && CachedGradientHash != ComputeGradientHash())
	{
		Texture->ConditionalPostLoad();
		BakeInto(Texture);
	}
#endif
}

#if WITH_EDITOR

void UGradientAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (bDuplicateForPIE)
	{
		return;
	}

	// A copy bakes into a texture of its own rather than writing over the original's.
	Texture = nullptr;
	CachedGradientHash = 0;
	CachedLayoutHash = 0;

	OnGradientAssetDuplicated.Broadcast(this);
}

void UGradientAsset::BakeInto(UTexture2D* Target)
{
	if (!Target)
	{
		return;
	}

	LLM_SCOPE(ELLMTag::Textures);

	Width = FMath::Clamp(Width, 2, 4096);
	EnsureUniqueGradientNames();

	for (FGradientLayer& Layer : Gradients)
	{
		Layer.SortStops();
	}

	const int32 Height = NumRows();
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

	if (Target->Source.GetSizeX() != Width || Target->Source.GetSizeY() != Height || Target->Source.GetFormat() != SourceFormat)
	{
		Target->Source.Init(Width, Height, /*NumSlices*/1, /*NumMips*/1, SourceFormat);
	}

	uint8* MipData = Target->Source.LockMip(0);
	{
		const float Step = 1.f / static_cast<float>(Width - 1);
		const int32 NumTexels = Width * Height;

		if (bHDR)
		{
			TArrayView<FFloat16Color> Dest(reinterpret_cast<FFloat16Color*>(MipData), NumTexels);
			check(Target->Source.CalcMipSize(0) == Dest.NumBytes());

			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					Dest[Y * Width + X] = EvaluateByIndex(Y, X * Step);
				}
			}
		}
		else
		{
			TArrayView<FColor> Dest(reinterpret_cast<FColor*>(MipData), NumTexels);
			check(Target->Source.CalcMipSize(0) == Dest.NumBytes());

			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					Dest[Y * Width + X] = EvaluateByIndex(Y, X * Step).ToFColor(false);
				}
			}
		}
	}
	Target->Source.UnlockMip(0);

	CachedGradientHash = ComputeGradientHash();
	CachedLayoutHash = ComputeLayoutHash();

	Target->PostEditChange();
}

FName UGradientAsset::MakeUniqueGradientName(FName BaseName, int32 IgnoreIndex) const
{
	const FString Base = BaseName.IsNone() ? FString(TEXT("Gradient")) : BaseName.ToString();

	FName Candidate(*Base);
	int32 Suffix = 0;

	while (true)
	{
		bool bTaken = false;
		for (int32 Index = 0; Index < Gradients.Num(); ++Index)
		{
			if (Index != IgnoreIndex && Gradients[Index].Name == Candidate)
			{
				bTaken = true;
				break;
			}
		}

		if (!bTaken)
		{
			return Candidate;
		}

		Candidate = FName(*FString::Printf(TEXT("%s_%d"), *Base, ++Suffix));
	}
}

void UGradientAsset::EnsureUniqueGradientNames()
{
	// Earlier gradients keep their name, so a duplicate added at the end is the one that moves.
	TSet<FName> Settled;
	Settled.Reserve(Gradients.Num());

	for (int32 Index = 0; Index < Gradients.Num(); ++Index)
	{
		const FString Base = Gradients[Index].Name.IsNone() ? FString(TEXT("Gradient")) : Gradients[Index].Name.ToString();

		FName Candidate(*Base);
		int32 Suffix = 0;

		while (true)
		{
			bool bTaken = Settled.Contains(Candidate);

			// A generated name must also dodge the gradients further down, which have not moved yet.
			for (int32 Other = Index + 1; !bTaken && Suffix > 0 && Other < Gradients.Num(); ++Other)
			{
				bTaken = (Gradients[Other].Name == Candidate);
			}

			if (!bTaken)
			{
				break;
			}

			Candidate = FName(*FString::Printf(TEXT("%s_%d"), *Base, ++Suffix));
		}

		Gradients[Index].Name = Candidate;
		Settled.Add(Candidate);
	}
}

uint32 UGradientAsset::ComputeGradientHash() const
{
	uint32 Hash = ::GetTypeHash(GradientTool::BakeVersion);
	Hash = HashCombine(Hash, ::GetTypeHash(Width));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Format)));
	Hash = HashCombine(Hash, ::GetTypeHash(Gradients.Num()));

	for (const FGradientLayer& Layer : Gradients)
	{
		Hash = HashCombine(Hash, GetTypeHash(Layer.Name));
		Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Layer.BlendSpace)));
		Hash = HashCombine(Hash, ::GetTypeHash(Layer.Stops.Num()));

		for (const FGradientStop& Stop : Layer.Stops)
		{
			Hash = HashCombine(Hash, ::GetTypeHash(Stop.Time));
			Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.R));
			Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.G));
			Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.B));
			Hash = HashCombine(Hash, ::GetTypeHash(Stop.Color.A));
			Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Stop.Interp)));
		}
	}

	return Hash;
}

uint32 UGradientAsset::ComputeLayoutHash() const
{
	uint32 Hash = ::GetTypeHash(Gradients.Num());

	for (const FGradientLayer& Layer : Gradients)
	{
		Hash = HashCombine(Hash, GetTypeHash(Layer.Name));
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
