﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExPathProcessor.h"

#include "PCGExPointsProcessor.h"
#include "Sampling/PCGExSampleNearestSpline.h"
#include "Sampling/PCGExSampling.h"

#include "PCGExSplineToPath.generated.h"

#define PCGEX_FOREACH_FIELD_SPLINETOPATH(MACRO)\
MACRO(ArriveTangent, FVector, FVector::ZeroVector)\
MACRO(LeaveTangent, FVector, FVector::ZeroVector)\
MACRO(LengthAtPoint, double, FVector::ZeroVector)\
MACRO(Alpha, double, FVector::ZeroVector)

/**
 * 
 */
UCLASS(Abstract, MinimalAPI, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class /*PCGEXTENDEDTOOLKIT_API*/ UPCGExSplineToPathSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings
#if WITH_EDITOR
	PCGEX_NODE_INFOS(SplineToPath, "Path : From Spline", "Breaks down an spline to a path.");
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings

	//~Begin UPCGExPointProcessorSettings
public:
	PCGEX_NODE_POINT_FILTER(FName("Flip SplineToPathation Conditions"), "Filters used to know whether an SplineToPathation should be flipped or not", PCGExFactories::PointFilters, false)
	//~End UPCGExPointProcessorSettings

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

public:

	/** Sample inputs.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Sampling", meta=(PCG_Overridable))
	EPCGExSplineSamplingIncludeMode SampleInputs = EPCGExSplineSamplingIncludeMode::All;
	
	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteArriveTangent = true;

	/** Name of the 'FVector' attribute to write Arrive tangent to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(DisplayName="Arrive Tangent", PCG_Overridable, EditCondition="bWriteArriveTangent"))
	FName ArriveTangentAttributeName = FName("ArriveTangent");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteLeaveTangent = true;

	/** Name of the 'FVector' attribute to write Leave tangent to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(DisplayName="Leave Tangent", PCG_Overridable, EditCondition="bWriteLeaveTangent"))
	FName LeaveTangentAttributeName = FName("LeaveTangent");


	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteLengthAtPoint = false;

	/** Name of the 'double' attribute to write the length at point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(DisplayName="Length at Point", PCG_Overridable, EditCondition="bWriteLengthAtPoint"))
	FName LengthAtPointAttributeName = FName("LengthAtPoint");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteAlpha = false;

	/** Name of the 'double' attribute to write the length at point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(DisplayName="Alpha", PCG_Overridable, EditCondition="bWriteAlpha"))
	FName AlphaAttributeName = FName("Alpha");


	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bTagIfClosedLoop = true;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging", meta=(PCG_Overridable, EditCondition="bTagIfClosedLoop"))
	FString IsClosedLoopTag = TEXT("Closed");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bTagIfOpenSpline = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging", meta=(PCG_Overridable, EditCondition="bTagIfOpenSpline"))
	FString IsOpenSplineTag = TEXT("Open");
	
};

struct /*PCGEXTENDEDTOOLKIT_API*/ FPCGExSplineToPathContext final : public FPCGExPointsProcessorContext
{
	friend class FPCGExSplineToPathElement;

	PCGEX_FOREACH_FIELD_SPLINETOPATH(PCGEX_OUTPUT_DECL_TOGGLE)
	
	TArray<const UPCGSplineData*> Targets;
	TArray<FPCGSplineStruct> Splines;

	int64 NumTargets = 0;

};

class /*PCGEXTENDEDTOOLKIT_API*/ FPCGExSplineToPathElement final : public FPCGExPointsProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGExContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

namespace PCGExSplineToPath
{
	class /*PCGEXTENDEDTOOLKIT_API*/ FWriteTask final : public PCGExMT::FPCGExTask
	{
	public:
		FWriteTask(const TSharedPtr<PCGExData::FPointIO>& InPointIO, const TSharedPtr<PCGExData::FFacade>& InPointDataFacade)
			: FPCGExTask(InPointIO), PointDataFacade(InPointDataFacade)

		{
		}

		TSharedPtr<PCGExData::FFacade> PointDataFacade;
		
		virtual bool ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) override;
	};
}