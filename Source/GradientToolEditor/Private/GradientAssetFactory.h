// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "GradientAssetFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UGradientAssetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UGradientAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
