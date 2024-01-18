﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/PCGExCustomGraphProcessor.h"

#include "IPCGExDebug.h"
#include "Data/PCGExGraphParamsData.h"

#define LOCTEXT_NAMESPACE "PCGExGraphSettings"

#pragma region UPCGSettings interface


TArray<FPCGPinProperties> UPCGExCustomGraphProcessorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	FPCGPinProperties& PinPropertyParams = PinProperties.Emplace_GetRef(PCGExGraph::SourceParamsLabel, EPCGDataType::Param);

#if WITH_EDITOR
	PinPropertyParams.Tooltip = FTEXT("Graph Params. Data is de-duped internally.");
#endif

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGExCustomGraphProcessorSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::OutputPinProperties();
	FPCGPinProperties& PinParamsOutput = PinProperties.Emplace_GetRef(PCGExGraph::OutputParamsLabel, EPCGDataType::Param);

#if WITH_EDITOR
	PinParamsOutput.Tooltip = FTEXT("Graph Params forwarding. Data is de-duped internally.");
#endif

	return PinProperties;
}

FName UPCGExCustomGraphProcessorSettings::GetMainInputLabel() const { return PCGExGraph::SourceGraphsLabel; }
FName UPCGExCustomGraphProcessorSettings::GetMainOutputLabel() const { return PCGExGraph::OutputGraphsLabel; }

FPCGExCustomGraphProcessorContext::~FPCGExCustomGraphProcessorContext()
{
	PCGEX_TERMINATE_ASYNC

	PCGEX_DELETE(CachedIndexReader)
	PCGEX_DELETE(CachedIndexWriter)

	SocketInfos.Empty();

	if (CurrentGraph) { CurrentGraph->Cleanup(); }
}

#pragma endregion

bool FPCGExCustomGraphProcessorContext::AdvanceGraph(const bool bResetPointsIndex)
{
	if (bResetPointsIndex) { CurrentPointsIndex = -1; }

	if (CurrentGraph) { CurrentGraph->Cleanup(); }

	if (Graphs.Params.IsValidIndex(++CurrentParamsIndex))
	{
		CurrentGraph = Graphs.Params[CurrentParamsIndex];
		CurrentGraph->GetSocketsInfos(SocketInfos);
		return true;
	}

	CurrentGraph = nullptr;
	return false;
}

bool FPCGExCustomGraphProcessorContext::AdvancePointsIOAndResetGraph()
{
	CurrentParamsIndex = -1;
	return AdvancePointsIO();
}

void FPCGExCustomGraphProcessorContext::Reset()
{
	FPCGExPointsProcessorContext::Reset();
	CurrentParamsIndex = -1;
}

void FPCGExCustomGraphProcessorContext::SetCachedIndex(const int32 PointIndex, const int32 Index) const
{
	check(!bReadOnly)
	(*CachedIndexWriter)[PointIndex] = Index;
}

int32 FPCGExCustomGraphProcessorContext::GetCachedIndex(const int32 PointIndex) const
{
	if (bReadOnly) { return (*CachedIndexReader)[PointIndex]; }
	return (*CachedIndexWriter)[PointIndex];
}

void FPCGExCustomGraphProcessorContext::PrepareCurrentGraphForPoints(const PCGExData::FPointIO& PointIO, const bool ReadOnly)
{
	bReadOnly = ReadOnly;
	if (bReadOnly)
	{
		PCGEX_DELETE(CachedIndexWriter)
		if (!CachedIndexReader) { CachedIndexReader = new PCGEx::TFAttributeReader<int32>(CurrentGraph->CachedIndexAttributeName); }
		CachedIndexReader->Bind(const_cast<PCGExData::FPointIO&>(PointIO));
	}
	else
	{
		PCGEX_DELETE(CachedIndexReader)
		if (!CachedIndexWriter) { CachedIndexWriter = new PCGEx::TFAttributeWriter<int32>(CurrentGraph->CachedIndexAttributeName, -1, false); }
		CachedIndexWriter->BindAndGet(const_cast<PCGExData::FPointIO&>(PointIO));
	}

	CurrentGraph->PrepareForPointData(PointIO, bReadOnly);
}

PCGEX_INITIALIZE_CONTEXT(CustomGraphProcessor)

bool FPCGExCustomGraphProcessorElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Boot(InContext)) { return false; }

	PCGEX_CONTEXT(CustomGraphProcessor)

	if (Context->Graphs.IsEmpty())
	{
		PCGE_LOG(Error, GraphAndLog, FTEXT("Missing Input Params."));
		return false;
	}

	Context->MergedInputSocketsNum = 0;
	for (const UPCGExGraphParamsData* Graph : Context->Graphs.Params) { Context->MergedInputSocketsNum += Graph->GetSocketMapping()->NumSockets; }

	return true;
}

FPCGContext* FPCGExCustomGraphProcessorElement::InitializeContext(
	FPCGExPointsProcessorContext* InContext,
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node) const
{
	FPCGExPointsProcessorElementBase::InitializeContext(InContext, InputData, SourceComponent, Node);

	PCGEX_CONTEXT_AND_SETTINGS(CustomGraphProcessor)

	if (!Settings->bEnabled) { return Context; }

	TArray<FPCGTaggedData> Sources = Context->InputData.GetInputsByPin(PCGExGraph::SourceParamsLabel);
	Context->Graphs.Initialize(InContext, Sources);

	return Context;
}


#undef LOCTEXT_NAMESPACE
