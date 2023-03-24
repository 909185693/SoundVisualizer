// Fill out your copyright notice in the Description page of Project Settings.


#include "SoundVisualizer.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "GameFramework/Actor.h"


// Sets default values
ASoundVisualizer::ASoundVisualizer()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponents = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));//�����
	SceneComponents->SetupAttachment(RootComponent);

	Ber = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ber"));//��̬������ʵ��
	Ber->SetupAttachment(SceneComponents);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));//��������
	Spline->SetupAttachment(Ber);
}

void ASoundVisualizer::OnConstruction(const FTransform& Transform)//����ű�����
{
	Super::OnConstruction(Transform);

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (IsValid(Spline))//ȷ���Ƿ�Ϊ����������������
		{
			Spline->SetVisibility(true, false);//�����������߿ɼ���
		}

		if (IsValid(Ber))
		{
			Ber->ClearInstances();	//ˢ������ǰ�������ɵ�ʵ��
		}

		Frequency.Empty();
		BaseTransform.Empty();

		X = 0;
		Y = 1;

		if (bUseCustom)
		{
			InstanceCustom();
		} 
		else
		{
			for (int32 Index = 1; NumberOfBars >= Index; Index++)	//����NumberOfBarsֵ������ʵ��
			{
				FTransform AddInstanceVal;

				switch (BarType)//��ͬ�������͵Ĵ���
				{
				case EBarType::Line://ֱ��
				{
					FVector TransformLocation(0.f, (Index - 1) * BarSpacing, 0.f);
					FRotator TransformRotation(0.f, 0.f, 0.f);
					FVector TransformScale(1.f, 1.f, 1.f);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion()); //�õ�ʵ������
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				case EBarType::Circle://Բ��
				{
					float AngleDegResult = 0;

					AngleDegResult = CalculationAngle(Index, NumberOfBars); //����ʵ�����ڽǶ�

					float CosResult = FMath::Cos(PI / (180.f) * AngleDegResult);//��ʵ�����ڽǶȻ��������ֵ
					float SinResult = FMath::Sin(PI / (180.f) * AngleDegResult);

					float DirectionVal = 0;

					switch (Direction)//����ʵ������ʱ����ĽǶ�
					{
					case EDirection::Inside://��Բ������
						DirectionVal = 90.f;
						break;

					case EDirection::Outside://��Բ������
						DirectionVal = 270.f;
						break;

					case EDirection::Up://��Բ�Ϸ�
						DirectionVal = 0.f;
						break;
					default:
						break;
					}

					FVector TransformLocation(CosResult * Radius, SinResult * Radius, 0.f);
					FRotator TransformRotation(DirectionVal, AngleDegResult, 0.f);
					FVector TransformScale(1.f, 1.f, 1.f);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion());
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				case EBarType::Grid://����
				{
					FVector TransformLocation(X * BarSpacing, Y * BarSpacing, 0.f);
					FRotator TransformRotation(0.f, 0.f, 0.f);
					FVector TransformScale(1.f, 1.f, 1.f);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion());
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				case EBarType::Spline://��������
				{
					FVector TransformLocation;
					FRotator TransformRotation;
					FVector TransformScale(1.f, 1.f, 1.f);

					float DistanceVal;
					DistanceVal = (float)Index / (float)NumberOfBars * Spline->GetSplineLength();

					TransformLocation = Spline->USplineComponent::GetLocationAtDistanceAlongSpline(DistanceVal, ESplineCoordinateSpace::Local);
					TransformRotation = Spline->USplineComponent::GetRotationAtDistanceAlongSpline(DistanceVal, ESplineCoordinateSpace::Local);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion());
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				default:
					break;
				}
				Ber->AddInstance(AddInstanceVal);//����ʵ��

				BaseTransform.Add(AddInstanceVal);//����Ƶ���ӻ��ṩʵ������

				X++;//���������������Ͳ���

				if (X >= FMath::RoundToInt(FMath::Sqrt(NumberOfBars)))//���������������Ͳ���
				{
					Y++;
					X = 0;
				}
			}
		}
	}
}

// Called when the game starts or when spawned
void ASoundVisualizer::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() < ROLE_Authority || GetNetMode() == ENetMode::NM_Standalone)
	{
		bDoOnce = false;

		UAudioMixerBlueprintLibrary::StartAnalyzingOutput(this, nullptr, EFFTSize::Max, EFFTPeakInterpolationMethod::Linear, EFFTWindowType::Hann);//��ʼ������Ϸ����Ƶ���

		for (int32 Index = 1; NumberOfBars >= Index; Index++)
		{
			int32 AddArryVal = FrequencyToAnalyze / NumberOfBars * Index;//�ṩ��Ҫ���ӻ�������Ƶ��
			Frequency.Add((float)AddArryVal);
		}
	}

	//GetWorldTimerManager().SetTimer(Handle, this, &ASoundVisualizer::GetFrequenciesData, UpdateRate, true, -1.f);
}

// Called every frame
void ASoundVisualizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole()<ROLE_Authority || GetNetMode() == ENetMode::NM_Standalone)
	{
		if ((GetWorld()->GetTimeSeconds() - CompleteTime) >= UpdateRate)
		{
			UAudioMixerBlueprintLibrary::GetMagnitudeForFrequencies(this, Frequency, Magnitudes, nullptr);//���������Ƶ��洢��Magnitudes������

			Frequencies.Empty();

			for (int32 Index = 0; Magnitudes.Num() > Index; Index++)//����Ƶ������ӵ�������
			{
				FTransform InstanceTransform;

				Ber->GetInstanceTransform(Index, InstanceTransform, false);

				FVector TempVector(InstanceTransform.GetScale3D().X, InstanceTransform.GetScale3D().Y, FMath::LogX(2.7f, FMath::Max(Magnitudes[Index], 1.f)) * FrequenciesHeight);

				InstanceTransform.SetScale3D(TempVector);

				Frequencies.Add(InstanceTransform);
			}
			CompleteTime = GetWorld()->GetTimeSeconds();

			if (Frequencies.Num() > 0)
			{
				bDoOnce = true;
			}
		}

		if (bDoOnce)
		{
			for (int32 Index = 0; NumberOfBars - 1 >= Index; Index++)
			{
				FTransform InstanceTransform;
				FTransform NewInstanceTransform;
				int32 FrequenciesIndex;

				if (Index > NumberOfBars - 1)
				{
					FrequenciesIndex = Index - NumberOfBars;
				}
				else
				{
					FrequenciesIndex = Index;
				}
				Ber->GetInstanceTransform(Index, InstanceTransform, false);//�õ�ʵ������

				FVector Vector(Frequencies[FrequenciesIndex].GetScale3D().X, Frequencies[FrequenciesIndex].GetScale3D().Y, FMath::Max(FMath::Lerp(InstanceTransform.GetScale3D().Z, Frequencies[FrequenciesIndex].GetScale3D().Z, 1.f - Smooth), 0.01f));//ͨ����Ƶ��������õ����ƺ����������

				NewInstanceTransform.SetLocation(InstanceTransform.GetLocation());//����ʵ������
				NewInstanceTransform.SetRotation(InstanceTransform.GetRotation());
				NewInstanceTransform.SetScale3D(Vector);//���ò�ֵ������ƽ������

				Ber->UpdateInstanceTransform(Index, NewInstanceTransform, false, true, false);//ͨ��ÿ֡���þ�̬�������ˢ�º������ı������������
			}
		}
	}
}

void ASoundVisualizer::GetFrequenciesData()
{
	UAudioMixerBlueprintLibrary::GetMagnitudeForFrequencies(this, Frequency, Magnitudes, nullptr);//���������Ƶ��洢��Magnitudes������

	Frequencies.Empty();

	for (int32 Index = 0; Magnitudes.Num() > Index; Index++)//����Ƶ������ӵ�������
	{
		FTransform InstanceTransform;

		Ber->GetInstanceTransform(Index, InstanceTransform, false);

		FVector TempVector(InstanceTransform.GetScale3D().X, InstanceTransform.GetScale3D().Y, FMath::LogX(2.7f, FMath::Max(Magnitudes[Index], 1.f)) * FrequenciesHeight);

		InstanceTransform.SetScale3D(TempVector);

		Frequencies.Add(InstanceTransform);
	}
	bDoOnce = true;
}

float ASoundVisualizer::CalculationAngle(float Index, float Length)
{
	return Index / Length * 360.0f;
}



//void ASoundVisualizer::RhythmMagic_Implementation()
//{
//
//}

void ASoundVisualizer::InstanceCustom_Implementation()
{
	
}

