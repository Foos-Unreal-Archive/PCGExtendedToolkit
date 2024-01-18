﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExCluster.h"
#include "PCGExPointsProcessor.h"

#include "PCGExEdgesProcessor.generated.h"

#define PCGEX_INVALID_CLUSTER_LOG PCGE_LOG(Warning, GraphAndLog, FTEXT("Some clusters are corrupted and will be ignored. If you modified vtx/edges manually, make sure to use Sanitize Cluster first."));

/**
 * A Base node to process a set of point using GraphParams.
 */
UCLASS(Abstract, BlueprintType, ClassGroup = (Procedural))
class PCGEXTENDEDTOOLKIT_API UPCGExEdgesProcessorSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(EdgesProcessorSettings, "Edges Processor Settings", "TOOLTIP_TEXT");
	virtual FLinearColor GetNodeTitleColor() const override { return PCGEx::NodeColorEdge; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	//~End UPCGSettings interface

	//~Begin UPCGExPointsProcessorSettings interface
public:
	virtual PCGExData::EInit GetMainOutputInitMode() const override;
	virtual PCGExData::EInit GetEdgeOutputInitMode() const;

	virtual FName GetMainInputLabel() const override;
	virtual FName GetMainOutputLabel() const override;

	virtual bool GetMainAcceptMultipleData() const override;
	//~End UPCGExPointsProcessorSettings interface
};

struct PCGEXTENDEDTOOLKIT_API FPCGExEdgesProcessorContext : public FPCGExPointsProcessorContext
{
	friend class UPCGExEdgesProcessorSettings;
	friend class FPCGExEdgesProcessorElement;

	virtual ~FPCGExEdgesProcessorContext() override;

	PCGExData::FPointIOGroup* MainEdges = nullptr;
	PCGExData::FPointIO* CurrentEdges = nullptr;

	PCGExData::FPointIOTaggedDictionary* InputDictionary = nullptr;
	PCGExData::FPointIOTaggedEntries* TaggedEdges = nullptr;
	TMap<int32, int32> NodeIndicesMap;
	PCGEx::TFAttributeReader<int32>* EdgeNumReader = nullptr;

	virtual bool AdvancePointsIO() override;
	bool AdvanceEdges(bool bBuildCluster = true); // Advance edges within current points

	PCGExCluster::FCluster* CurrentCluster = nullptr;

	void OutputPointsAndEdges();

	template <class InitializeFunc, class LoopBodyFunc>
	bool ProcessCurrentEdges(InitializeFunc&& Initialize, LoopBodyFunc&& LoopBody, bool bForceSync = false) { return Process(Initialize, LoopBody, CurrentEdges->GetNum(), bForceSync); }

	template <class LoopBodyFunc>
	bool ProcessCurrentEdges(LoopBodyFunc&& LoopBody, bool bForceSync = false) { return Process(LoopBody, CurrentEdges->GetNum(), bForceSync); }

	template <class InitializeFunc, class LoopBodyFunc>
	bool ProcessCurrentCluster(InitializeFunc&& Initialize, LoopBodyFunc&& LoopBody, bool bForceSync = false) { return Process(Initialize, LoopBody, CurrentCluster->Nodes.Num(), bForceSync); }

	template <class LoopBodyFunc>
	bool ProcessCurrentCluster(LoopBodyFunc&& LoopBody, bool bForceSync = false) { return Process(LoopBody, CurrentCluster->Nodes.Num(), bForceSync); }

protected:
	int32 CurrentEdgesIndex = -1;
};

class PCGEXTENDEDTOOLKIT_API FPCGExEdgesProcessorElement : public FPCGExPointsProcessorElementBase
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGContext* InContext) const override;
	virtual FPCGContext* InitializeContext(
		FPCGExPointsProcessorContext* InContext,
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) const override;
};
