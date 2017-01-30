// Fill out your copyright notice in the Description page of Project Settings.

#include "Bachelor.h"
#include "LSystemPlant.h"

#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

#include "Bachelor/Procedural/Data/MeshData.h"
#include "Bachelor/Procedural/Branch.h"
#include "Bachelor/Utility/MeshDataConstructor.h"
#include "Bachelor/Utility/MeshConstructor.h"
#include "Bachelor/Utility/BranchUtility.h"
#include "Bachelor/Utility/LSystemInterpreter.h"

#define PRODUCTION_WAS_UNSUCCESSFUL "ProductionUnsuccessfullError"

// Sets default values
ALSystemPlant::ALSystemPlant()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USphereComponent* Root = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	Root->InitSphereRadius(50.0f);
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	Mesh->SetupAttachment(RootComponent);

	InitUtilityValues();
}

ALSystemPlant::~ALSystemPlant() {
	//delete AllMeshData;
	delete RootBranch;
}

// Called when the game starts or when spawned
void ALSystemPlant::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Name: %s"), *this->GetName());

	ConstructProductionMap();
	ConstructFunctionMap();

	CompleteDerivation();
	UE_LOG(LogTemp, Warning, TEXT("Result of derivation: %s"), *CurrentDerivation);

/*
	if (SmoothOutBranchingAngles) {
		UBranchUtility::SmoothOutBranchingAngles(RootBranch);
	}
	if (PolyReductionByCurveReduction) {
		UBranchUtility::RecursiveReduceGrownBranches(RootBranch);
	}
	UMeshConstructor::GenerateTreeMesh(&TreeConstructionData); */
}

// Called every frame
void ALSystemPlant::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
}

void ALSystemPlant::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	/*if ((PropertyName == GET_MEMBER_NAME_CHECKED(ALSystemPlant, MaxNumGrowthIterations))) {
		// Potential place to regenerate tree
	}*/

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ALSystemPlant::InitUtilityValues() {
	TreeConstructionData.Mesh = Mesh;
	TreeConstructionData.AllMeshData = AllMeshData;
	TreeConstructionData.RootBranch = RootBranch;
}

void ALSystemPlant::ConstructProductionMap() {
	for (FProductionData productionData : LSystemData.Productions) {
		ProductionMap.Add(productionData.ID[0], productionData);
	}
}

void ALSystemPlant::ConstructFunctionMap() {

}

void ALSystemPlant::CompleteDerivation() {
	CurrentDerivation = LSystemData.Axiom;
	for (int i = 0; i < LSystemData.NumberOfDerivations; ++i) {
		bool derivationWasSuccessful = Derivate();

		if (!derivationWasSuccessful) {
			UE_LOG(LogTemp, Warning, TEXT("Critical Error during Derivation - interrupting."));
			break;
		}
	}
}

bool ALSystemPlant::Derivate() {
	FString derivationResult;
	bool derivationWasSuccessful = true;
	for (int i = 0; i < CurrentDerivation.Len(); ++i) {
		TCHAR currentChar = CurrentDerivation[i];
		FProductionData* potentialProduction = ProductionMap.Find(currentChar);
		if (NULL == potentialProduction) {
			derivationResult.AppendChar(currentChar);
		}
		else {
			int numberOfCharsToIgnore = 0;
			FString productionResult = CheckProduction(potentialProduction, i, numberOfCharsToIgnore);
			if (PRODUCTION_WAS_UNSUCCESSFUL == productionResult) {
				derivationWasSuccessful = false;
				UE_LOG(LogTemp, Warning, TEXT("Error during Derivation - Error during application of Production."));
				return derivationWasSuccessful;
			}
			i += numberOfCharsToIgnore;
			derivationResult += productionResult;
		}
	}
	if (derivationWasSuccessful) {
		CurrentDerivation = derivationResult;
	}
	return derivationWasSuccessful;
}

FString ALSystemPlant::CheckProduction(FProductionData* Production, int KeyIndex, int& OutNumberOfCharsToIgnore){
	int numParameters = Production->ParameterList.Num();

	FString productionResult = Production->ProductionResult;
	if (0 == numParameters) {
		return productionResult;
	}

	FString rightOfProduction = CurrentDerivation.RightChop(KeyIndex + 1);
	FString contentBetweenBrackets = ULSystemInterpreter::GetContentBetweenBrackets(rightOfProduction);
	if (INTERPRETER_ERROR_NUMBER_OF_BRACKETS == contentBetweenBrackets) {
		return PRODUCTION_WAS_UNSUCCESSFUL;
	}
	if (INTERPRETER_ERROR_FIRST_CHAR_IS_NOT_A_BRACKET == contentBetweenBrackets) {
		UE_LOG(LogTemp, Warning, TEXT("Expected Bracket after production %s - this production expects parameters."), *Production->ID);
		return PRODUCTION_WAS_UNSUCCESSFUL;
	}

	OutNumberOfCharsToIgnore = contentBetweenBrackets.Len() + 2; // content of parameterlist + brackets

	TMap<FString, FString> parameterValues;
	bool fillingWasSuccessful = FillParameterValues(parameterValues, Production, contentBetweenBrackets);
	if (!fillingWasSuccessful) {
		return PRODUCTION_WAS_UNSUCCESSFUL;
	}

	for (FString parameter : Production->ParameterList) {
		const TCHAR* parameterValue = **parameterValues.Find(parameter);
		const TCHAR* constParameter = *parameter;
		productionResult = productionResult.Replace(constParameter, parameterValue, ESearchCase::CaseSensitive);
	}

	return productionResult;
}

bool ALSystemPlant::FillParameterValues(TMap<FString, FString> &OutParameterValues, FProductionData* Production, FString ContentBetweenBrackets) {
	int numParameters = Production->ParameterList.Num();

	if (numParameters > 1) {
		TArray<FString> commaDividedContent;
		int numElementsInCommaDividedContent = ContentBetweenBrackets.ParseIntoArray(commaDividedContent, TEXT(","), false);

		if (numParameters != numElementsInCommaDividedContent) {
			UE_LOG(LogTemp, Warning, TEXT("Number of Parameters for Production %s is %d, was given %d"), *Production->ID, numParameters, numElementsInCommaDividedContent);
			return false;
		}

		for (int i = 0; i < numParameters; ++i) {
			FString parameter = Production->ParameterList[i];
			FString parameterValue = commaDividedContent[i];
			OutParameterValues.Add(parameter, parameterValue);
		}
	}
	else {
		FString parameter = Production->ParameterList[0];
		FString parameterValue = ContentBetweenBrackets;
		OutParameterValues.Add(parameter, parameterValue);
	}
	return true;
}