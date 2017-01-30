#pragma once

#include "UnrealMath.h"
#include "ProductionData.generated.h"

USTRUCT()
struct BACHELOR_API FProductionData
{
	GENERATED_USTRUCT_BODY()
public:

	FProductionData() {
		
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		TArray<FString> ParameterList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		FString ProductionResult;

};
