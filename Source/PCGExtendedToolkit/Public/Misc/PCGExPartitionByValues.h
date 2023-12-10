﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExFilter.h"

#include "Graph/PCGExGraphProcessor.h"

#include "PCGExPartitionByValues.generated.h"

namespace PCGExPartition
{
	const PCGExMT::AsyncState State_DistributeToPartition = PCGExMT::AsyncStateCounter::Unique();


	class FKPartition;

	class PCGEXTENDEDTOOLKIT_API FKPartition
	{
	protected:
		mutable FRWLock LayersLock;
		mutable FRWLock PointLock;

	public:
		FKPartition(FKPartition* InParent, int64 InKey, FPCGExFilter::FRule* InRule);
		~FKPartition();

		FKPartition* Parent = nullptr;
		int64 PartitionKey = 0;
		FPCGExFilter::FRule* Rule = nullptr;

		TMap<int64, FKPartition*> SubLayers;
		TArray<int32> Points;

		int32 GetNum() const { return Points.Num(); }
		int32 GetSubPartitionsNum();

		FKPartition* GetPartition(int64 Key, FPCGExFilter::FRule* InRule);
		void Add(const int64 Index);
		void Register(TArray<FKPartition*>& Partitions);
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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual bool GetMainPointsInputAcceptMultipleData() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	virtual PCGExData::EInit GetPointOutputInitMode() const override;

public:
	/** If false, will only write partition identifier values instead of splitting partitions into new point datasets. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bSplitOutput = true;

	/** Rules */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(TitleProperty="{TitlePropertyName}"))
	TArray<FPCGExFilterRuleDescriptor> PartitionRules;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExSplitByValuesContext : public FPCGExPointsProcessorContext
{
	friend class FPCGExPartitionByValuesElement;

	~FPCGExSplitByValuesContext();

	TArray<FPCGExFilterRuleDescriptor> RulesDescriptors;
	TArray<FPCGExFilter::FRule> Rules;
	mutable FRWLock RulesLock;

	bool bSplitOutput = true;
	PCGExPartition::FKPartition* RootPartition = nullptr;

	int32 NumPartitions = -1;
	TArray<PCGExPartition::FKPartition*> Partitions;
};

class PCGEXTENDEDTOOLKIT_API FPCGExPartitionByValuesElement : public FPCGExPointsProcessorElementBase
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;
	virtual bool Validate(FPCGContext* InContext) const override;

protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
