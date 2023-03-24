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

	SceneComponents = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));//根组件
	SceneComponents->SetupAttachment(RootComponent);

	Ber = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ber"));//静态网格体实例
	Ber->SetupAttachment(SceneComponents);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));//样条曲线
	Spline->SetupAttachment(Ber);
}

void ASoundVisualizer::OnConstruction(const FTransform& Transform)//构造脚本函数
{
	Super::OnConstruction(Transform);

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (IsValid(Spline))//确认是否为样条曲线生成类型
		{
			Spline->SetVisibility(true, false);//设置样条曲线可见性
		}

		if (IsValid(Ber))
		{
			Ber->ClearInstances();	//刷新属性前清理生成的实例
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
			for (int32 Index = 1; NumberOfBars >= Index; Index++)	//根据NumberOfBars值来生成实例
			{
				FTransform AddInstanceVal;

				switch (BarType)//不同生成类型的处理
				{
				case EBarType::Line://直线
				{
					FVector TransformLocation(0.f, (Index - 1) * BarSpacing, 0.f);
					FRotator TransformRotation(0.f, 0.f, 0.f);
					FVector TransformScale(1.f, 1.f, 1.f);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion()); //得到实例坐标
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				case EBarType::Circle://圆形
				{
					float AngleDegResult = 0;

					AngleDegResult = CalculationAngle(Index, NumberOfBars); //计算实例所在角度

					float CosResult = FMath::Cos(PI / (180.f) * AngleDegResult);//把实例所在角度换算成坐标值
					float SinResult = FMath::Sin(PI / (180.f) * AngleDegResult);

					float DirectionVal = 0;

					switch (Direction)//设置实例缩放时朝向的角度
					{
					case EDirection::Inside://朝圆外缩放
						DirectionVal = 90.f;
						break;

					case EDirection::Outside://朝圆内缩放
						DirectionVal = 270.f;
						break;

					case EDirection::Up://朝圆上方
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
				case EBarType::Grid://网格
				{
					FVector TransformLocation(X * BarSpacing, Y * BarSpacing, 0.f);
					FRotator TransformRotation(0.f, 0.f, 0.f);
					FVector TransformScale(1.f, 1.f, 1.f);

					AddInstanceVal.SetRotation(TransformRotation.Quaternion());
					AddInstanceVal.SetLocation(TransformLocation);
					AddInstanceVal.SetScale3D(TransformScale);
					break;
				}
				case EBarType::Spline://样条曲线
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
				Ber->AddInstance(AddInstanceVal);//生成实例

				BaseTransform.Add(AddInstanceVal);//给音频可视化提供实例坐标

				X++;//处理网格生成类型参数

				if (X >= FMath::RoundToInt(FMath::Sqrt(NumberOfBars)))//处理网格生成类型参数
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

		UAudioMixerBlueprintLibrary::StartAnalyzingOutput(this, nullptr, EFFTSize::Max, EFFTPeakInterpolationMethod::Linear, EFFTWindowType::Hann);//开始接受游戏主音频输出

		for (int32 Index = 1; NumberOfBars >= Index; Index++)
		{
			int32 AddArryVal = FrequencyToAnalyze / NumberOfBars * Index;//提供需要可视化的音乐频率
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
			UAudioMixerBlueprintLibrary::GetMagnitudeForFrequencies(this, Frequency, Magnitudes, nullptr);//分析输出音频后存储到Magnitudes数组中

			Frequencies.Empty();

			for (int32 Index = 0; Magnitudes.Num() > Index; Index++)//将音频数据添加到数组中
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
				Ber->GetInstanceTransform(Index, InstanceTransform, false);//得到实例坐标

				FVector Vector(Frequencies[FrequenciesIndex].GetScale3D().X, Frequencies[FrequenciesIndex].GetScale3D().Y, FMath::Max(FMath::Lerp(InstanceTransform.GetScale3D().Z, Frequencies[FrequenciesIndex].GetScale3D().Z, 1.f - Smooth), 0.01f));//通过音频数据数组得到限制后的缩放坐标

				NewInstanceTransform.SetLocation(InstanceTransform.GetLocation());//设置实例坐标
				NewInstanceTransform.SetRotation(InstanceTransform.GetRotation());
				NewInstanceTransform.SetScale3D(Vector);//利用插值函数来平滑缩放

				Ber->UpdateInstanceTransform(Index, NewInstanceTransform, false, true, false);//通过每帧调用静态网格组件刷新函数来改变网格体的缩放
			}
		}
	}
}

void ASoundVisualizer::GetFrequenciesData()
{
	UAudioMixerBlueprintLibrary::GetMagnitudeForFrequencies(this, Frequency, Magnitudes, nullptr);//分析输出音频后存储到Magnitudes数组中

	Frequencies.Empty();

	for (int32 Index = 0; Magnitudes.Num() > Index; Index++)//将音频数据添加到数组中
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

