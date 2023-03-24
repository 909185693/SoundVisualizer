// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "SoundVisualizer.generated.h"

UENUM(BlueprintType)
enum class EBarType : uint8
{
	Line,
	Circle,
	Grid,
	Spline
};

UENUM(BlueprintType)
enum class EBarMovement : uint8
{
	ScaleBothSides,
	ScaleOneSide,
	Location,
	LocationScale,
	LocationBothSides,
	Step
};

UENUM(BlueprintType)
enum class EDirection : uint8
{
	Inside,
	Outside,
	Up
};

UCLASS()
class SOUNDVISUALIZER_API ASoundVisualizer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASoundVisualizer();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UInstancedStaticMeshComponent* Ber;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USplineComponent* Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	EDirection Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	EBarType BarType;

	EBarMovement BarMovement;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	int32 NumberOfBars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	float BarSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	float FrequenciesHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	int32 FrequencyToAnalyze;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	float UpdateRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVisualizer")
	float Smooth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundVisualizer")
	bool bUseCustom;

	UPROPERTY(BlueprintReadWrite, Category = "SoundVisualizer")
	int32 X;

	UPROPERTY(BlueprintReadWrite, Category = "SoundVisualizer")
	int32 Y;

	TArray<float> Magnitudes;

	TArray<FTransform> Frequencies;

	TArray<FTransform> BaseTransform;

	float Test;

	float CompleteTime;

	TArray<float> Frequency;

	FTimerHandle Handle;

	bool bDoOnce;

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	void GetFrequenciesData();

	void RhythmMagic();

	UFUNCTION(BlueprintCallable)
	float CalculationAngle(float Index, float Length);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void InstanceCustom();

};
