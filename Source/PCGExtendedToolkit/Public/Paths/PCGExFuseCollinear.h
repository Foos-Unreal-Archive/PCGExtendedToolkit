﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExPathProcessor.h"
#include "PCGExPointsProcessor.h"
#include "PCGExFuseCollinear.generated.h"

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class PCGEXTENDEDTOOLKIT_API UPCGExFuseCollinearSettings : public UPCGExPathProcessorSettings
{
	GENERATED_BODY()

public:
	UPCGExFuseCollinearSettings(const FObjectInitializer& ObjectInitializer);

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(FuseCollinear, "Path : Fuse Collinear", "FuseCollinear paths points.");
#endif

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual PCGExData::EInit GetMainOutputInitMode() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, Units="Degrees", ClampMin=0, ClampMax=180))
	double Threshold = 10;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, InlineEditConditionToggle))
	//bool bDoBlend = false;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, Instanced, meta=(PCG_Overridable, NoResetToDefault, ShowOnlyInnerProperties, EditCondition="bDoBlend"))
	//TObjectPtr<UPCGExSubPointsBlendOperation> Blending = nullptr;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExFuseCollinearContext : public FPCGExPathProcessorContext
{
	friend class FPCGExFuseCollinearElement;

	virtual ~FPCGExFuseCollinearContext() override;

	double Threshold;
	//bool bDoBlend;
	//UPCGExSubPointsBlendOperation* Blending = nullptr;
};

class PCGEXTENDEDTOOLKIT_API FPCGExFuseCollinearElement : public FPCGExPathProcessorElement
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

class PCGEXTENDEDTOOLKIT_API FFuseCollinearTask : public FPCGExNonAbandonableTask
{
public:
	FFuseCollinearTask(FPCGExAsyncManager* InManager, const int32 InTaskIndex, PCGExData::FPointIO* InPointIO) :
		FPCGExNonAbandonableTask(InManager, InTaskIndex, InPointIO)
	{
	}

	virtual bool ExecuteTask() override;
};