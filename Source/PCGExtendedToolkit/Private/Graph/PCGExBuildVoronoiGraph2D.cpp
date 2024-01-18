﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/PCGExBuildVoronoiGraph2D.h"

#include "Elements/Metadata/PCGMetadataElementCommon.h"
#include "Geometry/PCGExGeoDelaunay.h"
#include "Geometry/PCGExGeoVoronoi.h"
#include "Graph/PCGExConsolidateCustomGraph.h"
#include "Graph/PCGExCluster.h"

#define LOCTEXT_NAMESPACE "PCGExGraph"
#define PCGEX_NAMESPACE BuildVoronoiGraph2D

int32 UPCGExBuildVoronoiGraph2DSettings::GetPreferredChunkSize() const { return 32; }

PCGExData::EInit UPCGExBuildVoronoiGraph2DSettings::GetMainOutputInitMode() const { return PCGExData::EInit::NewOutput; }

FPCGExBuildVoronoiGraph2DContext::~FPCGExBuildVoronoiGraph2DContext()
{
	PCGEX_TERMINATE_ASYNC

	PCGEX_DELETE(GraphBuilder)
	PCGEX_DELETE(Voronoi)
	PCGEX_DELETE(ConvexHull)

	HullIndices.Empty();
}

TArray<FPCGPinProperties> UPCGExBuildVoronoiGraph2DSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::OutputPinProperties();
	FPCGPinProperties& PinClustersOutput = PinProperties.Emplace_GetRef(PCGExGraph::OutputEdgesLabel, EPCGDataType::Point);

#if WITH_EDITOR
	PinClustersOutput.Tooltip = FTEXT("Point data representing edges.");
#endif


	return PinProperties;
}

FName UPCGExBuildVoronoiGraph2DSettings::GetMainOutputLabel() const { return PCGExGraph::OutputVerticesLabel; }

PCGEX_INITIALIZE_ELEMENT(BuildVoronoiGraph2D)

bool FPCGExBuildVoronoiGraph2DElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(BuildVoronoiGraph2D)

	PCGEX_VALIDATE_NAME(Settings->HullAttributeName)

	return true;
}

bool FPCGExBuildVoronoiGraph2DElement::ExecuteInternal(
	FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExBuildVoronoiGraph2DElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(BuildVoronoiGraph2D)

	if (Context->IsSetup())
	{
		if (!Boot(Context)) { return true; }
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		PCGEX_DELETE(Context->GraphBuilder)
		PCGEX_DELETE(Context->Voronoi)
		PCGEX_DELETE(Context->ConvexHull)
		Context->HullIndices.Empty();

		if (!Context->AdvancePointsIO()) { Context->Done(); }
		else
		{
			if (Context->CurrentIO->GetNum() <= 4)
			{
				Context->SetState(PCGExMT::State_ReadyForNextPoints);
				PCGE_LOG(Warning, GraphAndLog, FTEXT("(0) Some inputs have too few points to be processed (<= 3)."));
				return false;
			}

			if constexpr (false) //Settings->bMarkHull)
			{
				Context->ConvexHull = new PCGExGeo::TConvexHull2();
				TArray<PCGExGeo::TFVtx<2>*> HullVertices;
				GetVerticesFromPoints(Context->CurrentIO->GetIn()->GetPoints(), HullVertices);

				if (Context->ConvexHull->Prepare(HullVertices))
				{
					if (Context->bDoAsyncProcessing) { Context->ConvexHull->StartAsyncProcessing(Context->GetAsyncManager()); }
					else { Context->ConvexHull->Generate(); }
					Context->SetAsyncState(PCGExGeo::State_ProcessingHull);
				}
				else
				{
					PCGE_LOG(Warning, GraphAndLog, FTEXT("(1) Some inputs generates no results. Are points coplanar? If so, use Voronoi 2D instead."));
					return false;
				}
			}

			Context->SetAsyncState(PCGExGeo::State_ProcessingHull);
		}
	}

	if (Context->IsState(PCGExGeo::State_ProcessingHull))
	{
		if constexpr (false) //Settings->bMarkHull)
		{
			if (Context->IsAsyncWorkComplete())
			{
				if (Context->bDoAsyncProcessing) { Context->ConvexHull->Finalize(); }
				Context->ConvexHull->GetHullIndices(Context->HullIndices);

				PCGEx::TFAttributeWriter<bool>* HullMarkPointWriter = new PCGEx::TFAttributeWriter<bool>(Settings->HullAttributeName, false, false);
				HullMarkPointWriter->BindAndGet(*Context->CurrentIO);

				for (int i = 0; i < Context->CurrentIO->GetNum(); i++) { HullMarkPointWriter->Values[i] = Context->HullIndices.Contains(i); }

				HullMarkPointWriter->Write();
				PCGEX_DELETE(HullMarkPointWriter)
				PCGEX_DELETE(Context->ConvexHull)
			}
			else
			{
				return false;
			}
		}

		Context->Voronoi = new PCGExGeo::TVoronoiMesh2();
		Context->Voronoi->CellCenter = Settings->Method;
		Context->Voronoi->BoundsExtension = Settings->BoundsCutoff;

		if (Context->Voronoi->PrepareFrom(Context->CurrentIO->GetIn()->GetPoints()))
		{
			Context->Voronoi->Delaunay->ConvexHullIndices = &Context->HullIndices;

			if (Context->bDoAsyncProcessing)
			{
				Context->Voronoi->Delaunay->Hull->StartAsyncProcessing(Context->GetAsyncManager());
				Context->SetAsyncState(PCGExGeo::State_ProcessingDelaunayHull);
			}
			else
			{
				Context->Voronoi->Generate();
				Context->SetState(PCGExGeo::State_ProcessingVoronoi);
			}
		}
		else
		{
			Context->SetState(PCGExMT::State_ReadyForNextPoints);
			PCGE_LOG(Warning, GraphAndLog, FTEXT("(2) Some inputs generates no results."));
			return false;
		}
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunayHull))
	{
		if (!Context->IsAsyncWorkComplete()) { return false; }

		Context->Voronoi->Delaunay->Hull->Finalize();
		Context->SetState(PCGExGeo::State_ProcessingDelaunayPreprocess);
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunayPreprocess))
	{
		auto PreprocessSimplex = [&](const int32 Index) { Context->Voronoi->Delaunay->PreprocessSimplex(Index); };

		if (!Context->Process(PreprocessSimplex, Context->Voronoi->Delaunay->Hull->Simplices.Num())) { return false; }

		Context->Voronoi->Delaunay->Cells.SetNumUninitialized(Context->Voronoi->Delaunay->NumFinalCells);
		Context->SetState(PCGExGeo::State_ProcessingDelaunay);
	}

	if (Context->IsState(PCGExGeo::State_ProcessingDelaunay))
	{
		auto ProcessSimplex = [&](const int32 Index) { Context->Voronoi->Delaunay->ProcessSimplex(Index); };

		if (!Context->Process(ProcessSimplex, Context->Voronoi->Delaunay->NumFinalCells)) { return false; }

		if (Context->Voronoi->Delaunay->Cells.IsEmpty())
		{
			Context->SetState(PCGExMT::State_ReadyForNextPoints);
			PCGE_LOG(Warning, GraphAndLog, FTEXT("(3) Some inputs generates no results."));
			return false;
		}

		Context->Voronoi->PrepareVoronoi();

		if (Context->bDoAsyncProcessing) { Context->Voronoi->StartAsyncPreprocessing(Context->GetAsyncManager()); }
		Context->SetState(PCGExGeo::State_ProcessingVoronoi);
	}

	if (Context->IsState(PCGExGeo::State_ProcessingVoronoi))
	{
		if (Context->bDoAsyncProcessing && !Context->IsAsyncWorkComplete()) { return false; }

		if (Context->Voronoi->Regions.IsEmpty())
		{
			PCGE_LOG(Warning, GraphAndLog, FTEXT("(2) Some inputs generates no results. Are points coplanar? If so, use Voronoi 2D instead."));
			Context->SetState(PCGExMT::State_ReadyForNextPoints);
			return false;
		}

		// Write Edges

		TArray<FPCGPoint>& Centroids = Context->CurrentIO->GetOut()->GetMutablePoints();
		Context->Voronoi->GetVoronoiPoints(Centroids, Settings->Method);

		Context->GraphBuilder = new PCGExGraph::FGraphBuilder(*Context->CurrentIO, 8);

		TArray<PCGExGraph::FUnsignedEdge> Edges;
		Context->Voronoi->GetUniqueEdges(Edges, Settings->bPruneOutsideBounds && Settings->Method != EPCGExCellCenter::Balanced);
		Context->GraphBuilder->Graph->InsertEdges(Edges);

		//

		Context->GraphBuilder->Compile(Context);
		Context->SetAsyncState(PCGExGraph::State_WritingClusters);
	}

	if (Context->IsState(PCGExGraph::State_WritingClusters))
	{
		if (!Context->IsAsyncWorkComplete()) { return false; }

		/*
		//Mark hull
		if (Settings->bMarkHull)
		{
			int32 GraphIndex = 0;
			for (PCGExGraph::FSubGraph* SubGraph : Context->GraphBuilder->Graph->SubGraphs)
			{
				PCGEx::TFAttributeWriter<bool>* HullMarkWriter = nullptr;
				HullMarkWriter = new PCGEx::TFAttributeWriter<bool>(Settings->HullAttributeName, false);
				HullMarkWriter->BindAndGet((*Context->GraphBuilder->EdgesIO)[GraphIndex++]);

				for (int32 NodeIndex : SubGraph->Nodes)
				{
					PCGExGraph::FNode& Node = SubGraph->Nodes[NodeIndex];
					//Nodes are from
				}

				for (int i = 0; i < Edges.Num(); i++)
				{
					const PCGExGraph::FUnsignedEdge& Edge = Edges[i];
					const bool bStartOnHull = HullIndices.Contains(Edge.Start);
					const bool bEndOnHull = HullIndices.Contains(Edge.End);
					HullMarkWriter->Values[i] = Settings->bMarkEdgeOnTouch ? bStartOnHull || bEndOnHull : bStartOnHull && bEndOnHull;
				}

				HullMarkWriter->Write();
				PCGEX_DELETE(HullMarkWriter)
			}
		}
		*/

		if (Context->GraphBuilder->bCompiledSuccessfully) { Context->GraphBuilder->Write(Context); }
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
