﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExCommon.h"
#include "PCGExPointsProcessor.h"
#include "Data/PCGSpatialData.h"
#include "PCGSettings.h"
#include "Elements/PCGPointProcessingElementBase.h"
#include "Graph/PCGExGraphProcessor.h"
#include "PCGExLocalAttributeHelpers.h"
#include <shared_mutex>
#include "PCGExPartitionByValues.generated.h"

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExPartitionRuleDescriptor : public FPCGExInputDescriptorWithSingleField
{
	GENERATED_BODY()

	FPCGExPartitionRuleDescriptor(): FPCGExInputDescriptorWithSingleField()
	{
	}

	FPCGExPartitionRuleDescriptor(const FPCGExPartitionRuleDescriptor& Other): FPCGExInputDescriptorWithSingleField(Other)
	{
		FilterSize = Other.FilterSize;
		Upscale = Other.Upscale;
	}

public:
	/** Filter Size. Higher values means fewer, larger groups. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	double FilterSize = 1.0;

	/** Upscale multiplier, applied before filtering. Handy to deal with floating point values. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	double Upscale = 1.0;
};

namespace PCGExPartition
{
	struct PCGEXTENDEDTOOLKIT_API FRule : public PCGEx::FLocalSingleComponentInput
	{
		FRule()
		{
		}

		FRule(FPCGExPartitionRuleDescriptor InRule):
			FLocalSingleComponentInput(InRule.Field, InRule.Axis),
			FilterSize(InRule.FilterSize),
			Upscale(InRule.Upscale)
		{
			Descriptor = static_cast<FPCGExInputDescriptor>(InRule);
		}

	public:
		double FilterSize = 1.0;
		double Upscale = 1.0;
	};
}

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Misc")
class PCGEXTENDEDTOOLKIT_API UPCGExPartitionByValuesSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(PartitionByValues, "Partition by Values", "Outputs separate buckets of points based on an attribute' value. Each bucket is named after a unique attribute value. Note that it is recommended to use a MERGE before.");
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	virtual PCGEx::EIOInit GetPointOutputInitMode() const override;

public:
	/** Rules */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, ShowOnlyInnerProperties, FullyExpand=true))
	FPCGExPartitionRuleDescriptor PartitioningRules;

	/** Whether to write the partition Key to an attribute. Useful for debugging. Note: They key is not the index, but instead the filtered value used to distribute into partitions. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bWriteKey;

	/** Name of the int64 attribute to write the partition Key to. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bWriteKey"))
	FName KeyAttributeName = "PartitionKey";
};

struct PCGEXTENDEDTOOLKIT_API FPCGExSplitByValuesContext : public FPCGExPointsProcessorContext
{
	friend class FPCGExPartitionByValuesElement;

public:
	TMap<int64, UPCGExPointIO*> PartitionsMap;
	TMap<int64, FPCGMetadataAttribute<int64>*> KeyAttributeMap;

	TArray<PCGExPartition::FRule> Rules;
	TMap<UPCGExPointIO*, PCGExPartition::FRule*> RuleMap;
	
	UPCGExPointIOGroup* Partitions;
	FPCGExPartitionRuleDescriptor PartitionRule;
	FName PartitionKeyName;
	bool bWritePartitionKey = false;

	mutable FRWLock RulesLock;
	mutable FRWLock PartitionsLock;
	mutable FRWLock PointsLock;

	bool ValidatePointDataInput(UPCGPointData* PointData) override;
	
};

class PCGEXTENDEDTOOLKIT_API FPCGExPartitionByValuesElement : public FPCGExPointsProcessorElementBase
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual void InitializeContext(
		FPCGExPointsProcessorContext* InContext,
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) const override;
	virtual bool Validate(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;

private:
	static void DistributePoint(FPCGExSplitByValuesContext* Context, UPCGExPointIO* IO, const FPCGPoint& Point, const double InValue);
	static int64 Filter(const double InValue, const PCGExPartition::FRule& Rule);
};