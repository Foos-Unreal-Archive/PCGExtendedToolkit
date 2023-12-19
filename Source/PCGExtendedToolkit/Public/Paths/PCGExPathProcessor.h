﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"

#include "PCGExPointsProcessor.h"
#include "Graph/PCGExGraph.h"
#include "PCGExPathProcessor.generated.h"

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(Abstract, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class PCGEXTENDEDTOOLKIT_API UPCGExPathProcessorSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	UPCGExPathProcessorSettings(const FObjectInitializer& ObjectInitializer);
	virtual PCGExData::EInit GetMainOutputInitMode() const override;

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	//PCGEX_NODE_INFOS(PathProcessor, "PathProcessor", "Processes paths segments.");
	virtual FLinearColor GetNodeTitleColor() const override { return PCGEx::NodeColorPath; }
#endif

	virtual FName GetMainInputLabel() const override;
	virtual FName GetMainOutputLabel() const override;
	//~End UPCGSettings interface
};

struct PCGEXTENDEDTOOLKIT_API FPCGExPathProcessorContext : public FPCGExPointsProcessorContext
{
	friend class FPCGExPathProcessorElement;
};

class PCGEXTENDEDTOOLKIT_API FPCGExPathProcessorElement : public FPCGExPointsProcessorElementBase
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;
};