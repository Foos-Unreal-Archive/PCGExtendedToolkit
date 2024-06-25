﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExOperation.h"
#include "Graph/PCGExCluster.h"
#include "UObject/Object.h"
#include "PCGExProbeOperation.generated.h"

namespace PCGExProbing
{
	struct FCandidate;
}

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExProbeDescriptorBase
{
	GENERATED_BODY()

	FPCGExProbeDescriptorBase()
	{
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExFetchType SearchRadiusSource = EPCGExFetchType::Constant;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, ClampMin=0, EditCondition="SearchRadiusSource==EPCGExFetchType::Constant", EditConditionHides))
	double SearchRadiusConstant = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="SearchRadiusSource==EPCGExFetchType::Attribute", EditConditionHides))
	FPCGAttributePropertyInputSelector SearchRadiusAttribute;
};

/**
 * 
 */
UCLASS(Abstract)
class PCGEXTENDEDTOOLKIT_API UPCGExProbeOperation : public UPCGExOperation
{
	GENERATED_BODY()

public:
	virtual bool PrepareForPoints(const PCGExData::FPointIO* InPointIO);
	virtual bool RequiresDirectProcessing();
	virtual void ProcessCandidates(const int32 Index, const FPCGPoint& Point, TArray<PCGExProbing::FCandidate>& Candidates);
	virtual void ProcessNode(const int32 Index, const FPCGPoint& Point);

	virtual void Cleanup() override;

	double SearchRadiusSquared = -1;
	TArray<double> SearchRadiusCache;

	TSet<uint64> UniqueEdges;

	FPCGExProbeDescriptorBase* BaseDescriptor = nullptr;

protected:
	mutable FRWLock UniqueEdgesLock;


	const PCGExData::FPointIO* PointIO = nullptr;
	TArray<double> LocalWeightMultiplier;

	void AddEdge(uint64 Edge);
};
