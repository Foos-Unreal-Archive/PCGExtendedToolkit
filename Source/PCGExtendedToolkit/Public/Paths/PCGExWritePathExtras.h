﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExPathProcessor.h"
#include "Sampling/PCGExSampling.h"
#include "PCGExWritePathExtras.generated.h"

#define PCGEX_FOREACH_FIELD_PATHEXTRAS(MACRO)\
MACRO(Dot, double)\
MACRO(DistanceToNext, double)\
MACRO(DistanceToPrev, double)\
MACRO(DistanceToStart, double)\
MACRO(DistanceToEnd, double)\
MACRO(PointTime, double)\
MACRO(PointNormal, FVector)\
MACRO(DirectionToNext, FVector)\
MACRO(DirectionToPrev, FVector)

#define PCGEX_FOREACH_FIELD_PATHEXTRAS_MARKS(MACRO)\
MACRO(PathLength, double)\
MACRO(PathDirection, FVector)\
MACRO(PathCentroid, FVector)

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class PCGEXTENDEDTOOLKIT_API UPCGExWritePathExtrasSettings : public UPCGExPathProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(WritePathExtras, "Path : Write Extras", "Extract & write extra path informations to the path.");
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	//~Begin UObject interface
public:
#if WITH_EDITOR

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End UObject interface

	//~Begin UPCGExPointsProcessorSettings interface
public:
	virtual PCGExData::EInit GetMainOutputInitMode() const override;
	//~End UPCGExPointsProcessorSettings interface

public:
	/** Consider paths to be closed -- processing will wrap between first and last points. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bClosedPath = false;

	/** Up vector used to calculate Offset direction.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Normal", meta=(PCG_Overridable))
	FVector UpVector = FVector::UpVector;

	/** Fetch the Up vector from a local point attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Normal", meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bUseLocalUpVector = false;

	/** Fetch the Up vector from a local point attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Normal", meta = (PCG_Overridable, EditCondition="bUseLocalOffset"))
	FPCGAttributePropertyInputSelector LocalUpVector;


	/** Output Path Length. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWritePathLength = false;

	/** Name of the 'double' attribute to write path length to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, EditCondition="bWritePathLength"))
	FName PathLengthAttributeName = FName("PathLength");


	/** Output averaged path direction. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWritePathDirection = false;

	/** Name of the 'FVector' attribute to write averaged direction to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, EditCondition="bWritePathDirection"))
	FName PathDirectionAttributeName = FName("PathDirection");

	/** Output averaged path direction. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWritePathCentroid = false;

	/** Name of the 'FVector' attribute to write averaged direction to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Path", meta=(PCG_Overridable, EditCondition="bWritePathCentroid"))
	FName PathCentroidAttributeName = FName("PathCentroid");

	///

	/** Output Dot product of Prev/Next directions. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDot = false;

	/** Name of the 'double' attribute to write distance to next point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDot"))
	FName DotAttributeName = FName("Dot");

	/** Output distance to next. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDistanceToNext = false;

	/** Name of the 'double' attribute to write distance to next point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDistanceToNext"))
	FName DistanceToNextAttributeName = FName("DistanceToNext");


	/** Output distance to prev. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDistanceToPrev = false;

	/** Name of the 'double' attribute to write distance to prev point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDistanceToPrev"))
	FName DistanceToPrevAttributeName = FName("DistanceToPrev");


	/** Output distance to start. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDistanceToStart = false;

	/** Name of the 'double' attribute to write distance to start to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDistanceToStart"))
	FName DistanceToStartAttributeName = FName("DistanceToStart");


	/** Output distance to end. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDistanceToEnd = false;

	/** Name of the 'double' attribute to write distance to start to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDistanceToEnd"))
	FName DistanceToEndAttributeName = FName("DistanceToEnd");


	/** Output distance to end. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWritePointTime = false;

	/** Name of the 'double' attribute to write distance to start to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWritePointTime"))
	FName PointTimeAttributeName = FName("PointTime");


	/** Output point normal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWritePointNormal = false;

	/** Name of the 'FVector' attribute to write point normal to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWritePointNormal"))
	FName PointNormalAttributeName = FName("PointNormal");


	/** Output direction to next normal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDirectionToNext = false;

	/** Name of the 'FVector' attribute to write direction to next point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDirectionToNext"))
	FName DirectionToNextAttributeName = FName("DirectionToNext");


	/** Output direction to prev normal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDirectionToPrev = false;

	/** Name of the 'FVector' attribute to write direction to prev point to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output - Points", meta=(PCG_Overridable, EditCondition="bWriteDirectionToPrev"))
	FName DirectionToPrevAttributeName = FName("DirectionToPrev");
};

struct PCGEXTENDEDTOOLKIT_API FPCGExWritePathExtrasContext final : public FPCGExPathProcessorContext
{
	friend class FPCGExWritePathExtrasElement;

	virtual ~FPCGExWritePathExtrasContext() override;

	PCGEX_FOREACH_FIELD_PATHEXTRAS(PCGEX_OUTPUT_DECL_TOGGLE)
	PCGEX_FOREACH_FIELD_PATHEXTRAS_MARKS(PCGEX_OUTPUT_DECL_TOGGLE)
};

class PCGEXTENDEDTOOLKIT_API FPCGExWritePathExtrasElement final : public FPCGExPathProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

namespace PCGExWritePathExtras
{
	class FProcessor final : public PCGExPointsMT::FPointsProcessor
	{
		PCGEX_FOREACH_FIELD_PATHEXTRAS(PCGEX_OUTPUT_DECL)

		TArray<FVector> Positions;
		bool bClosedPath = false;
		int32 LastIndex = 0;

	public:
		FProcessor(PCGExData::FPointIO* InPoints):
			FPointsProcessor(InPoints)
		{
		}

		virtual ~FProcessor() override;

		virtual bool Process(PCGExMT::FTaskManager* AsyncManager) override;
		virtual void ProcessSinglePoint(const int32 Index, FPCGPoint& Point) override;
		virtual void CompleteWork() override;
	};
}
