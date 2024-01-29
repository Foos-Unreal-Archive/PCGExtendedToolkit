﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/PCGExBuildDelaunayGraph2D.h"

#include "Elements/Metadata/PCGMetadataElementCommon.h"
#include "Geometry/PCGExGeoDelaunay.h"
#include "Graph/PCGExConsolidateCustomGraph.h"
#include "Graph/PCGExCluster.h"

#define LOCTEXT_NAMESPACE "PCGExGraph"
#define PCGEX_NAMESPACE BuildDelaunayGraph2D

int32 UPCGExBuildDelaunayGraph2DSettings::GetPreferredChunkSize() const { return 32; }

PCGExData::EInit UPCGExBuildDelaunayGraph2DSettings::GetMainOutputInitMode() const { return bMarkHull ? PCGExData::EInit::DuplicateInput : PCGExData::EInit::Forward; }

FPCGExBuildDelaunayGraph2DContext::~FPCGExBuildDelaunayGraph2DContext()
{
	PCGEX_TERMINATE_ASYNC

	PCGEX_DELETE(GraphBuilder)
	PCGEX_DELETE(Delaunay)
	PCGEX_DELETE(ConvexHull)

	HullIndices.Empty();
}

TArray<FPCGPinProperties> UPCGExBuildDelaunayGraph2DSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::OutputPinProperties();
	FPCGPinProperties& PinClustersOutput = PinProperties.Emplace_GetRef(PCGExGraph::OutputEdgesLabel, EPCGDataType::Point);

#if WITH_EDITOR
	PinClustersOutput.Tooltip = FTEXT("Point data representing edges.");
#endif


	return PinProperties;
}

FName UPCGExBuildDelaunayGraph2DSettings::GetMainOutputLabel() const { return PCGExGraph::OutputVerticesLabel; }

PCGEX_INITIALIZE_ELEMENT(BuildDelaunayGraph2D)

bool FPCGExBuildDelaunayGraph2DElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(BuildDelaunayGraph2D)
	
	PCGEX_FWD(ProjectionSettings)
	Context->GraphBuilderSettings.bPruneIsolatedPoints = false;
	
	PCGEX_VALIDATE_NAME(Settings->HullAttributeName)

	return true;
}

bool FPCGExBuildDelaunayGraph2DElement::ExecuteInternal(
	FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExBuildDelaunayGraph2DElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(BuildDelaunayGraph2D)

	if (Context->IsSetup())
	{
		if (!Boot(Context)) { return true; }
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		PCGEX_DELETE(Context->GraphBuilder)
		PCGEX_DELETE(Context->Delaunay)
		PCGEX_DELETE(Context->ConvexHull)
		Context->HullIndices.Empty();

		if (!Context->AdvancePointsIO()) { Context->Done(); }
		else
		{
			if (Context->CurrentIO->GetNum() <= 4)
			{
				PCGE_LOG(Warning, GraphAndLog, FTEXT("(0) Some inputs have too few points to be processed (<= 3)."));
				return false;
			}

			if (Settings->bMarkHull)
			{
				Context->ConvexHull = new PCGExGeo::TConvexHull2();
				TArray<PCGExGeo::TFVtx<2>*> HullVertices;
				GetVerticesFromPoints(Context->CurrentIO->GetIn()->GetPoints(), HullVertices);

				if (Context->ConvexHull->Prepare(HullVertices))
				{
					if (Context->bDoAsyncProcessing) { Context->ConvexHull->StartAsyncProcessing(Context->GetAsyncManager()); }
					else { Context->ConvexHull->Generate(); }
				}
				else
				{
					PCGE_LOG(Warning, GraphAndLog, FTEXT("(1) Some inputs generates no results. Check for singularities."));
					return false;
				}
			}

			Context->SetAsyncState(PCGExGeo::State_ProcessingHull);
		}
	}

	if (Context->IsState(PCGExGeo::State_ProcessingHull))
	{
		if (Settings->bMarkHull)
		{
			if (!Context->IsAsyncWorkComplete()) { return false; }

			if (Context->bDoAsyncProcessing) { Context->ConvexHull->Finalize(); }
			Context->ConvexHull->GetHullIndices(Context->HullIndices);

			PCGEx::TFAttributeWriter<bool>* HullMarkPointWriter = new PCGEx::TFAttributeWriter<bool>(Settings->HullAttributeName, false, false);
			HullMarkPointWriter->BindAndGet(*Context->CurrentIO);

			for (int i = 0; i < Context->CurrentIO->GetNum(); i++) { HullMarkPointWriter->Values[i] = Context->HullIndices.Contains(i); }

			HullMarkPointWriter->Write();
			PCGEX_DELETE(HullMarkPointWriter)
			PCGEX_DELETE(Context->ConvexHull)
		}

		Context->Delaunay = new PCGExGeo::TDelaunayTriangulation2();
		if (Context->Delaunay->PrepareFrom(Context->CurrentIO->GetIn()->GetPoints()))
		{
			if (Context->bDoAsyncProcessing)
			{
				Context->Delaunay->Hull->StartAsyncProcessing(Context->GetAsyncManager());
			}
			else { Context->Delaunay->Hull->Generate(); }
			Context->SetAsyncState(PCGExGeo::State_ProcessingDelaunayHull);
		}
		else
		{
			Context->SetState(PCGExMT::State_ReadyForNextPoints);
			PCGE_LOG(Warning, GraphAndLog, FTEXT("(2) Some inputs generates no results. Are points coplanar? If so, use Delaunay 2D instead."));
			return false;
		}
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunayHull))
	{
		if (Context->IsAsyncWorkComplete())
		{
			Context->Delaunay->Hull->Finalize();

			if (Context->bDoAsyncProcessing)
			{
				Context->SetState(PCGExGeo::State_ProcessingDelaunayPreprocess);
			}
			else
			{
				Context->Delaunay->Generate();
				Context->SetAsyncState(PCGExGeo::State_ProcessingDelaunay);
			}
		}
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunayPreprocess))
	{
		auto PreprocessSimplex = [&](const int32 Index) { Context->Delaunay->PreprocessSimplex(Index); };
		if (!Context->Process(PreprocessSimplex, Context->Delaunay->Hull->Simplices.Num())) { return false; }

		Context->Delaunay->Cells.SetNumUninitialized(Context->Delaunay->NumFinalCells);
		Context->SetState(PCGExGeo::State_ProcessingDelaunay);
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunay))
	{
		auto ProcessSimplex = [&](const int32 Index) { Context->Delaunay->ProcessSimplex(Index); };
		if (!Context->Process(ProcessSimplex, Context->Delaunay->NumFinalCells)) { return false; }

		if (Context->Delaunay->Cells.IsEmpty())
		{
			Context->SetState(PCGExMT::State_ReadyForNextPoints);
			PCGE_LOG(Warning, GraphAndLog, FTEXT("(3) Some inputs generates no results. Are points coplanar? If so, use Delaunay 2D instead."));
			return false;
		}

		Context->GraphBuilder = new PCGExGraph::FGraphBuilder(*Context->CurrentIO, &Context->GraphBuilderSettings, 8);

		TArray<PCGExGraph::FUnsignedEdge> Edges;

		if (Settings->bUrquhart) { Context->Delaunay->GetUrquhartEdges(Edges); }
		else { Context->Delaunay->GetUniqueEdges(Edges); }

		Context->GraphBuilder->Graph->InsertEdges(Edges);

		Context->GraphBuilder->Compile(Context);
		Context->SetAsyncState(PCGExGraph::State_WritingClusters);
	}

	if (Context->IsState(PCGExGraph::State_WritingClusters))
	{
		if (!Context->IsAsyncWorkComplete()) { return false; }
		if (Context->GraphBuilder->bCompiledSuccessfully)
		{
			Context->GraphBuilder->Write(Context);
			//TODO: Mark edges, process EdgesIO from GraphBuilder
		}
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsDone())
	{
		Context->OutputPoints();
	}

	return Context->IsDone();
}

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
