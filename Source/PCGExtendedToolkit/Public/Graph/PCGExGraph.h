﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"

#include "Data/PCGExAttributeHelpers.h"
#include "PCGExGlobalSettings.h"
#include "PCGExMT.h"
#include "PCGExEdge.h"
#include "PCGExDetails.h"
#include "PCGExDetailsIntersection.h"
#include "Data/PCGExData.h"
#include "PCGExGraph.generated.h"

namespace PCGExGraph
{
	struct FSubGraph;
}

namespace PCGExCluster
{
	class FCluster;
}

UENUM(BlueprintType, meta=(DisplayName="[PCGEx] Graph Value Source"))
enum class EPCGExGraphValueSource : uint8
{
	Vtx  = 0 UMETA(DisplayName = "Point", Tooltip="Value is fetched from the point being evaluated."),
	Edge = 1 UMETA(DisplayName = "Edge", Tooltip="Value is fetched from the edge connecting to the point being evaluated."),
};

UENUM(BlueprintType, meta=(DisplayName="[PCGEx] Intersection Type"))
enum class EPCGExIntersectionType : uint8
{
	Unknown   = 0 UMETA(DisplayName = "Unknown", ToolTip="Unknown"),
	PointEdge = 1 UMETA(DisplayName = "Point/Edge", ToolTip="Point/Edge Intersection."),
	EdgeEdge  = 2 UMETA(DisplayName = "Edge/Edge", ToolTip="Edge/Edge Intersection."),
};

USTRUCT(BlueprintType)
struct /*PCGEXTENDEDTOOLKIT_API*/ FPCGExGraphBuilderDetails
{
	GENERATED_BODY()

	FPCGExGraphBuilderDetails()
	{
	}

	/** Don't output Clusters if they have less points than a specified amount. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bWriteEdgePosition = true;

	/** Edge position interpolation between start and end point positions. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bWriteEdgePosition"))
	double EdgePosition = 0.5;

	/** Don't output Clusters if they have less points than a specified amount. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bRemoveSmallClusters = false;

	/** Minimum points threshold */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bRemoveSmallClusters", ClampMin=2))
	int32 MinVtxCount = 3;

	/** Minimum edges threshold */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bRemoveSmallClusters", ClampMin=2))
	int32 MinEdgeCount = 3;

	/** Don't output Clusters if they have more points than a specified amount. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bRemoveBigClusters = false;

	/** Maximum points threshold */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bRemoveBigClusters", ClampMin=2))
	int32 MaxVtxCount = 500;

	/** Maximum edges threshold */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bRemoveBigClusters", ClampMin=2))
	int32 MaxEdgeCount = 500;

	/** Refresh Edge Seed. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable))
	bool bRefreshEdgeSeed = false;

	/** If the use of cached clusters is enabled, output clusters along with the graph data. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable))
	bool bBuildAndCacheClusters = GetDefault<UPCGExGlobalSettings>()->bDefaultBuildAndCacheClusters;

	/** Expands the cluster data. Takes more space in memory but can be a very effective improvement depending on the operations you're doing on the cluster. */
	UPROPERTY(BlueprintReadWrite, Category = Settings, EditAnywhere, meta = (PCG_Overridable, EditCondition="bBuildAndCacheClusters"))
	bool bExpandClusters = GetDefault<UPCGExGlobalSettings>()->bDefaultCacheExpandedClusters;

	bool IsValid(const TSharedPtr<PCGExGraph::FSubGraph>& InSubgraph) const;
};

namespace PCGExGraph
{
	const FName SourceProbesLabel = TEXT("Probes");
	const FName OutputProbeLabel = TEXT("Probe");

	const FName SourceFilterGenerators = TEXT("Generator Filters");
	const FName SourceFilterConnectables = TEXT("Connectable Filters");

	const FName SourceGraphsLabel = TEXT("In");
	const FName OutputGraphsLabel = TEXT("Out");

	const FName SourceVerticesLabel = TEXT("Vtx");
	const FName OutputVerticesLabel = TEXT("Vtx");

	const FName SourcePathsLabel = TEXT("Paths");
	const FName OutputPathsLabel = TEXT("Paths");

	const FName Tag_PackedClusterPointCount = FName(PCGEx::PCGExPrefix + TEXT("PackedClusterPointCount"));
	const FName Tag_PackedClusterEdgeCount = FName(PCGEx::PCGExPrefix + TEXT("PackedClusterEdgeCount"));

	const FName SourceSeedsLabel = TEXT("Seeds");
	const FName SourceGoalsLabel = TEXT("Goals");
	const FName SourcePlotsLabel = TEXT("Plots");

	const FName SourceHeuristicsLabel = TEXT("Heuristics");
	const FName OutputHeuristicsLabel = TEXT("Heuristics");
	const FName OutputModifiersLabel = TEXT("Modifiers");

	PCGEX_ASYNC_STATE(State_PreparingUnion)
	PCGEX_ASYNC_STATE(State_ProcessingUnion)

	PCGEX_ASYNC_STATE(State_WritingClusters)
	PCGEX_ASYNC_STATE(State_ReadyToCompile)
	PCGEX_ASYNC_STATE(State_Compiling)

	PCGEX_ASYNC_STATE(State_ProcessingPointEdgeIntersections)
	PCGEX_ASYNC_STATE(State_ProcessingEdgeEdgeIntersections)

	PCGEX_ASYNC_STATE(State_Pathfinding)
	PCGEX_ASYNC_STATE(State_WaitingPathfinding)

	class FGraph;

#pragma region Graph Utils

	static bool BuildIndexedEdges(
		const TSharedPtr<PCGExData::FPointIO>& EdgeIO,
		const TMap<uint32, int32>& EndpointsLookup,
		TArray<FIndexedEdge>& OutEdges,
		const bool bStopOnError = false)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExEdge::BuildIndexedEdges-Vanilla);

		const TUniquePtr<PCGExData::TBuffer<int64>> EndpointsBuffer = MakeUnique<PCGExData::TBuffer<int64>>(EdgeIO.ToSharedRef(), Tag_EdgeEndpoints);
		if (!EndpointsBuffer->PrepareRead()) { return false; }

		const TArray<int64>& Endpoints = *EndpointsBuffer->GetInValues().Get();

		bool bValid = true;
		const int32 NumEdges = EdgeIO->GetNum();

		PCGEx::InitArray(OutEdges, NumEdges);

		if (!bStopOnError)
		{
			int32 EdgeIndex = 0;

			for (int i = 0; i < NumEdges; i++)
			{
				uint32 A;
				uint32 B;
				PCGEx::H64(Endpoints[i], A, B);

				const int32* StartPointIndexPtr = EndpointsLookup.Find(A);
				const int32* EndPointIndexPtr = EndpointsLookup.Find(B);

				if ((!StartPointIndexPtr || !EndPointIndexPtr)) { continue; }

				OutEdges[EdgeIndex] = FIndexedEdge(EdgeIndex, *StartPointIndexPtr, *EndPointIndexPtr, EdgeIndex, EdgeIO->IOIndex);
				EdgeIndex++;
			}

			PCGEx::InitArray(OutEdges, EdgeIndex);
		}
		else
		{
			for (int i = 0; i < NumEdges; i++)
			{
				uint32 A;
				uint32 B;
				PCGEx::H64(Endpoints[i], A, B);

				const int32* StartPointIndexPtr = EndpointsLookup.Find(A);
				const int32* EndPointIndexPtr = EndpointsLookup.Find(B);

				if ((!StartPointIndexPtr || !EndPointIndexPtr))
				{
					bValid = false;
					break;
				}

				OutEdges[i] = FIndexedEdge(i, *StartPointIndexPtr, *EndPointIndexPtr, i, EdgeIO->IOIndex);
			}
		}

		return bValid;
	}

	static bool BuildIndexedEdges(
		const TSharedPtr<PCGExData::FPointIO>& EdgeIO,
		const TMap<uint32, int32>& EndpointsLookup,
		TArray<FIndexedEdge>& OutEdges,
		TSet<int32>& OutNodePoints,
		const bool bStopOnError = false)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExEdge::BuildIndexedEdges-WithPoints);

		const TUniquePtr<PCGExData::TBuffer<int64>> EndpointsBuffer = MakeUnique<PCGExData::TBuffer<int64>>(EdgeIO.ToSharedRef(), Tag_EdgeEndpoints);
		if (!EndpointsBuffer->PrepareRead()) { return false; }

		const TArray<int64>& Endpoints = *EndpointsBuffer->GetInValues().Get();

		bool bValid = true;
		const int32 NumEdges = EdgeIO->GetNum();

		PCGEx::InitArray(OutEdges, NumEdges);

		if (!bStopOnError)
		{
			int32 EdgeIndex = 0;

			for (int i = 0; i < NumEdges; i++)
			{
				uint32 A;
				uint32 B;
				PCGEx::H64(Endpoints[i], A, B);

				const int32* StartPointIndexPtr = EndpointsLookup.Find(A);
				const int32* EndPointIndexPtr = EndpointsLookup.Find(B);

				if ((!StartPointIndexPtr || !EndPointIndexPtr)) { continue; }

				OutNodePoints.Add(*StartPointIndexPtr);
				OutNodePoints.Add(*EndPointIndexPtr);

				OutEdges[EdgeIndex] = FIndexedEdge(EdgeIndex, *StartPointIndexPtr, *EndPointIndexPtr, EdgeIndex, EdgeIO->IOIndex);
				EdgeIndex++;
			}

			OutEdges.SetNum(EdgeIndex);
		}
		else
		{
			for (int i = 0; i < NumEdges; i++)
			{
				uint32 A;
				uint32 B;
				PCGEx::H64(Endpoints[i], A, B);

				const int32* StartPointIndexPtr = EndpointsLookup.Find(A);
				const int32* EndPointIndexPtr = EndpointsLookup.Find(B);

				if ((!StartPointIndexPtr || !EndPointIndexPtr))
				{
					bValid = false;
					break;
				}

				OutNodePoints.Add(*StartPointIndexPtr);
				OutNodePoints.Add(*EndPointIndexPtr);

				OutEdges[i] = FIndexedEdge(i, *StartPointIndexPtr, *EndPointIndexPtr, i, EdgeIO->IOIndex);
			}
		}

		return bValid;
	}

#pragma endregion

#pragma region Graph

	struct /*PCGEXTENDEDTOOLKIT_API*/ FGraphMetadataDetails
	{
#define PCGEX_FOREACH_POINTPOINT_METADATA(MACRO)\
		MACRO(IsPointUnion, PointUnionData.bWriteIsUnion, PointUnionData.IsUnion, TEXT("bIsUnion"))\
		MACRO(PointUnionSize, PointUnionData.bWriteUnionSize, PointUnionData.UnionSize, TEXT("UnionSize"))\
		MACRO(IsEdgeUnion, EdgeUnionData.bWriteIsUnion, EdgeUnionData.IsUnion, TEXT("bIsUnion"))\
		MACRO(EdgeUnionSize, EdgeUnionData.bWriteUnionSize, EdgeUnionData.UnionSize, TEXT("UnionSize"))

#define PCGEX_FOREACH_POINTEDGE_METADATA(MACRO)\
		MACRO(IsIntersector, bWriteIsIntersector, IsIntersector,TEXT("bIsIntersector"))

#define PCGEX_FOREACH_EDGEEDGE_METADATA(MACRO)\
		MACRO(Crossing, bWriteCrossing, Crossing,TEXT("bCrossing"))

#define PCGEX_GRAPH_META_DECL(_NAME, _ACCESSOR, _ACCESSOR2, _DEFAULT)	bool bWrite##_NAME = false; FName _NAME##AttributeName = _DEFAULT;
		PCGEX_FOREACH_POINTPOINT_METADATA(PCGEX_GRAPH_META_DECL);
		PCGEX_FOREACH_POINTEDGE_METADATA(PCGEX_GRAPH_META_DECL);
		PCGEX_FOREACH_EDGEEDGE_METADATA(PCGEX_GRAPH_META_DECL);

		bool bFlagCrossing = false;
		FName FlagA = NAME_None;
		FName FlagB = NAME_None;

#define PCGEX_GRAPH_META_FWD(_NAME, _ACCESSOR, _ACCESSOR2, _DEFAULT)	bWrite##_NAME = InDetails._ACCESSOR; _NAME##AttributeName = InDetails._ACCESSOR2##AttributeName; PCGEX_SOFT_VALIDATE_NAME(bWrite##_NAME, _NAME##AttributeName, Context)

		void Grab(const FPCGContext* Context, const FPCGExPointPointIntersectionDetails& InDetails)
		{
			PCGEX_FOREACH_POINTPOINT_METADATA(PCGEX_GRAPH_META_FWD);
		}

		void Grab(const FPCGContext* Context, const FPCGExPointEdgeIntersectionDetails& InDetails)
		{
			PCGEX_FOREACH_POINTEDGE_METADATA(PCGEX_GRAPH_META_FWD);
		}

		void Grab(const FPCGContext* Context, const FPCGExEdgeEdgeIntersectionDetails& InDetails)
		{
			PCGEX_FOREACH_EDGEEDGE_METADATA(PCGEX_GRAPH_META_FWD);
		}

#undef PCGEX_FOREACH_POINTPOINT_METADATA
#undef PCGEX_GRAPH_META_FWD
	};

	struct /*PCGEXTENDEDTOOLKIT_API*/ FGraphNodeMetadata
	{
		EPCGExIntersectionType Type = EPCGExIntersectionType::PointEdge;
		int32 NodeIndex;
		int32 UnionSize = 0; // Fuse size
		bool IsUnion() const { return UnionSize > 1; }

		explicit FGraphNodeMetadata(const int32 InNodeIndex)
			: NodeIndex(InNodeIndex)
		{
		}

		bool IsIntersector() const { return Type == EPCGExIntersectionType::PointEdge; }
		bool IsCrossing() const { return Type == EPCGExIntersectionType::EdgeEdge; }

		static FGraphNodeMetadata& GetOrCreate(const int32 NodeIndex, TMap<int32, FGraphNodeMetadata>& InMetadata)
		{
			if (FGraphNodeMetadata* MetadataPtr = InMetadata.Find(NodeIndex)) { return *MetadataPtr; }
			return InMetadata.Add(NodeIndex, FGraphNodeMetadata(NodeIndex));
		}
	};

	struct /*PCGEXTENDEDTOOLKIT_API*/ FGraphEdgeMetadata
	{
		int32 EdgeIndex;
		int32 ParentIndex;
		int32 RootIndex;
		EPCGExIntersectionType Type = EPCGExIntersectionType::Unknown;

		int32 UnionSize = 0; // Fuse size
		bool IsUnion() const { return UnionSize > 1; }

		explicit FGraphEdgeMetadata(const int32 InEdgeIndex, const FGraphEdgeMetadata* Parent)
			: EdgeIndex(InEdgeIndex), ParentIndex(Parent ? Parent->EdgeIndex : InEdgeIndex), RootIndex(Parent ? Parent->RootIndex : InEdgeIndex)
		{
		}

		FORCEINLINE static FGraphEdgeMetadata& GetOrCreate(const int32 EdgeIndex, const FGraphEdgeMetadata* Parent, TMap<int32, FGraphEdgeMetadata>& InMetadata)
		{
			if (FGraphEdgeMetadata* MetadataPtr = InMetadata.Find(EdgeIndex)) { return *MetadataPtr; }
			return InMetadata.Add(EdgeIndex, FGraphEdgeMetadata(EdgeIndex, Parent));
		}
	};

	struct /*PCGEXTENDEDTOOLKIT_API*/ FNode
	{
		FNode()
		{
		}

		FNode(const int32 InNodeIndex, const int32 InPointIndex):
			NodeIndex(InNodeIndex), PointIndex(InPointIndex)
		{
			Adjacency.Empty();
		}

		bool bValid = true;

		int32 NodeIndex = -1;  // Index in the context of the list that helds the node
		int32 PointIndex = -1; // Index in the context of the UPCGPointData that helds the vtx
		int32 NumExportedEdges = 0;

		TArray<uint64> Adjacency;

		FORCEINLINE void SetAdjacency(const TSet<uint64>& InAdjacency) { Adjacency = InAdjacency.Array(); }
		FORCEINLINE void Add(const int32 EdgeIndex) { Adjacency.AddUnique(EdgeIndex); }

		~FNode() = default;
	};

	struct /*PCGEXTENDEDTOOLKIT_API*/ FSubGraph
	{
		int64 Id = -1;
		FGraph* ParentGraph = nullptr;
		TSet<int32> Nodes;
		TSet<int32> Edges;
		TSet<int32> EdgesInIOIndices;
		TSharedPtr<PCGExData::FFacade> VtxDataFacade;
		TSharedPtr<PCGExData::FFacade> EdgesDataFacade;
		TArray<FIndexedEdge> FlattenedEdges;
		int64 UID = 0;

		FSubGraph()
		{
			PCGEX_LOG_CTR(FSubGraph)
		}

		~FSubGraph()
		{
			PCGEX_LOG_DTR(FSubGraph)
		}

		FORCEINLINE void Add(const FIndexedEdge& Edge, FGraph* InGraph)
		{
			Nodes.Add(Edge.Start);
			Nodes.Add(Edge.End);
			Edges.Add(Edge.EdgeIndex);
			if (Edge.IOIndex >= 0) { EdgesInIOIndices.Add(Edge.IOIndex); }
		}

		void Invalidate(FGraph* InGraph);
		TSharedPtr<PCGExCluster::FCluster> CreateCluster(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) const;
		int32 GetFirstInIOIndex();
	};

	class /*PCGEXTENDEDTOOLKIT_API*/ FGraph
	{
		mutable FRWLock GraphLock;
		const int32 NumEdgesReserve;

	public:
		bool bBuildClusters = false;
		bool bExpandClusters = false;

		TArray<FNode> Nodes;
		TMap<int32, FGraphNodeMetadata> NodeMetadata;
		TMap<int32, FGraphEdgeMetadata> EdgeMetadata;

		TArray<FIndexedEdge> Edges;

		TSet<uint64> UniqueEdges;

		TArray<TSharedPtr<FSubGraph>> SubGraphs;

		bool bWriteEdgePosition = true;
		double EdgePosition = 0.5;

		bool bRefreshEdgeSeed = false;

		explicit FGraph(const int32 InNumNodes, const int32 InNumEdgesReserve = 10)
			: NumEdgesReserve(InNumEdgesReserve)
		{
			PCGEX_LOG_CTR(FGraph)

			PCGEx::InitArray(Nodes, InNumNodes);

			for (int i = 0; i < InNumNodes; i++)
			{
				FNode& Node = Nodes[i];
				Node.NodeIndex = Node.PointIndex = i;
				Node.Adjacency.Reserve(NumEdgesReserve);
			}
		}

		void ReserveForEdges(const int32 UpcomingAdditionCount);

		bool InsertEdgeUnsafe(int32 A, int32 B, FIndexedEdge& OutEdge, int32 IOIndex);
		bool InsertEdge(const int32 A, const int32 B, FIndexedEdge& OutEdge, const int32 IOIndex = -1);

		bool InsertEdgeUnsafe(const FIndexedEdge& Edge);
		bool InsertEdge(const FIndexedEdge& Edge);

		void InsertEdgesUnsafe(const TSet<uint64>& InEdges, int32 InIOIndex);
		void InsertEdges(const TSet<uint64>& InEdges, int32 InIOIndex);

		void InsertEdges(const TArray<uint64>& InEdges, int32 InIOIndex);
		int32 InsertEdges(const TArray<FIndexedEdge>& InEdges);

		FORCEINLINE FGraphNodeMetadata* FindNodeMetadata(const int32 NodeIndex) { return NodeMetadata.Find(NodeIndex); }
		FORCEINLINE FGraphEdgeMetadata* FindEdgeMetadata(const int32 EdgeIndex) { return EdgeMetadata.Find(EdgeIndex); }
		FORCEINLINE FGraphEdgeMetadata* FindRootEdgeMetadata(const int32 EdgeIndex)
		{
			const FGraphEdgeMetadata* BaseEdge = EdgeMetadata.Find(EdgeIndex);
			return BaseEdge ? EdgeMetadata.Find(BaseEdge->RootIndex) : nullptr;
		}

		TArrayView<FNode> AddNodes(const int32 NumNewNodes);

		void BuildSubGraphs(const FPCGExGraphBuilderDetails& Limits);

		~FGraph()
		{
			PCGEX_LOG_DTR(FGraph)
		}

		void GetConnectedNodes(int32 FromIndex, TArray<int32>& OutIndices, int32 SearchDepth) const;
	};

	class /*PCGEXTENDEDTOOLKIT_API*/ FGraphBuilder : public TSharedFromThis<FGraphBuilder>
	{
	protected:
		TSharedPtr<PCGExMT::FTaskManager> AsyncManager;
		FGraphMetadataDetails* MetadataDetailsPtr = nullptr;
		bool bWriteVtxDataFacadeWithCompile = false;

	public:
		const FPCGExGraphBuilderDetails* OutputDetails = nullptr;

		using CompilationEndCallback = std::function<void(const TSharedRef<FGraphBuilder>& InBuilder, const bool bSuccess)>;
		CompilationEndCallback OnCompilationEndCallback;

		int64 PairId;
		FString PairIdStr;

		TSharedPtr<FGraph> Graph;

		TSharedRef<PCGExData::FFacade> NodeDataFacade;

		TSharedPtr<PCGExData::FPointIOCollection> EdgesIO;
		TSharedPtr<PCGExData::FPointIOCollection> SourceEdgesIO;

		bool bCompiledSuccessfully = false;

		FGraphBuilder(const TSharedRef<PCGExData::FFacade>& InNodeDataFacade, const FPCGExGraphBuilderDetails* InDetails, const int32 NumEdgeReserve = 6, const TSharedPtr<PCGExData::FPointIOCollection>& InSourceEdges = nullptr)
			: OutputDetails(InDetails), NodeDataFacade(InNodeDataFacade), SourceEdgesIO(InSourceEdges)
		{
			PCGEX_LOG_CTR(FGraphBuilder)

			PairId = NodeDataFacade->Source->GetOutIn()->UID;
			NodeDataFacade->Source->Tags->Add(TagStr_ClusterPair, PairId, PairIdStr);

			const int32 NumNodes = NodeDataFacade->Source->GetOutInNum();

			Graph = MakeShared<FGraph>(NumNodes, NumEdgeReserve);
			Graph->bBuildClusters = InDetails->bBuildAndCacheClusters;
			Graph->bExpandClusters = InDetails->bExpandClusters;
			Graph->bWriteEdgePosition = OutputDetails->bWriteEdgePosition;
			Graph->EdgePosition = OutputDetails->EdgePosition;
			Graph->bRefreshEdgeSeed = OutputDetails->bRefreshEdgeSeed;

			EdgesIO = MakeShared<PCGExData::FPointIOCollection>(NodeDataFacade->Source->GetContext());
			EdgesIO->DefaultOutputLabel = OutputEdgesLabel;
		}

		void CompileAsync(const TSharedPtr<PCGExMT::FTaskManager>& InAsyncManager, const bool bWriteNodeFacade, FGraphMetadataDetails* MetadataDetails = nullptr);
		void Compile(const TSharedPtr<PCGExMT::FTaskManager>& InAsyncManager, const bool bWriteNodeFacade, FGraphMetadataDetails* MetadataDetails = nullptr);

		void OutputEdgesToContext() const;

		~FGraphBuilder()
		{
			PCGEX_LOG_DTR(FGraphBuilder)
		}
	};

	static bool BuildEndpointsLookup(
		const TSharedPtr<PCGExData::FPointIO>& InPointIO,
		TMap<uint32, int32>& OutIndices,
		TArray<int32>& OutAdjacency)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExGraph::BuildLookupTable);

		PCGEx::InitArray(OutAdjacency, InPointIO->GetNum());
		OutIndices.Empty();

		const TUniquePtr<PCGExData::TBuffer<int64>> IndexBuffer = MakeUnique<PCGExData::TBuffer<int64>>(InPointIO.ToSharedRef(), Tag_VtxEndpoint);
		if (!IndexBuffer->PrepareRead()) { return false; }

		const TArray<int64>& Indices = *IndexBuffer->GetInValues().Get();

		OutIndices.Reserve(Indices.Num());
		for (int i = 0; i < Indices.Num(); i++)
		{
			uint32 A;
			uint32 B;
			PCGEx::H64(Indices[i], A, B);

			OutIndices.Add(A, i);
			OutAdjacency[i] = B;
		}

		return true;
	}

#pragma endregion

	static bool IsPointDataVtxReady(const UPCGMetadata* Metadata)
	{
		constexpr int16 I64 = static_cast<uint16>(EPCGMetadataTypes::Integer64);
		//constexpr int16 I32 = static_cast<uint16>(EPCGMetadataTypes::Integer32);

		const FPCGMetadataAttributeBase* EndpointAttribute = Metadata->GetConstAttribute(Tag_VtxEndpoint);
		if (!EndpointAttribute || EndpointAttribute->GetTypeId() != I64) { return false; }

		const FPCGMetadataAttributeBase* ClusterIdAttribute = Metadata->GetConstAttribute(Tag_ClusterId);
		if (!ClusterIdAttribute || ClusterIdAttribute->GetTypeId() != I64) { return false; }

		return true;
	}

	static bool IsPointDataEdgeReady(const UPCGMetadata* Metadata)
	{
		constexpr int16 I64 = static_cast<uint16>(EPCGMetadataTypes::Integer64);
		constexpr int16 I32 = static_cast<uint16>(EPCGMetadataTypes::Integer32);

		const FPCGMetadataAttributeBase* EndpointAttribute = Metadata->GetConstAttribute(Tag_EdgeEndpoints);
		if (!EndpointAttribute || EndpointAttribute->GetTypeId() != I64) { return false; }

		const FPCGMetadataAttributeBase* ClusterIdAttribute = Metadata->GetConstAttribute(Tag_ClusterId);
		if (!ClusterIdAttribute || ClusterIdAttribute->GetTypeId() != I64) { return false; }

		return true;
	}

	static bool GetReducedVtxIndices(const TSharedPtr<PCGExData::FPointIO>& InEdges, const TMap<uint32, int32>* NodeIndicesMap, TArray<int32>& OutVtxIndices, int32& OutEdgeNum)
	{
		const TUniquePtr<PCGExData::TBuffer<int64>> EndpointsBuffer = MakeUnique<PCGExData::TBuffer<int64>>(InEdges.ToSharedRef(), Tag_EdgeEndpoints);
		if (!EndpointsBuffer->PrepareRead()) { return false; }

		const TArray<int64>& Endpoints = *EndpointsBuffer->GetInValues().Get();

		OutEdgeNum = Endpoints.Num();

		OutVtxIndices.Empty();

		TSet<int32> UniqueVtx;
		UniqueVtx.Reserve(OutEdgeNum * 2);

		for (int i = 0; i < OutEdgeNum; i++)
		{
			uint32 A;
			uint32 B;
			PCGEx::H64(Endpoints[i], A, B);

			const int32* NodeStartPtr = NodeIndicesMap->Find(A);
			const int32* NodeEndPtr = NodeIndicesMap->Find(B);

			if (!NodeStartPtr || !NodeEndPtr || (*NodeStartPtr == *NodeEndPtr)) { continue; }

			UniqueVtx.Add(*NodeStartPtr);
			UniqueVtx.Add(*NodeEndPtr);
		}

		OutVtxIndices.Append(UniqueVtx.Array());
		UniqueVtx.Empty();

		return true;
	}

	static void CleanupVtxData(const TSharedPtr<PCGExData::FPointIO>& PointIO)
	{
		UPCGMetadata* Metadata = PointIO->GetOut()->Metadata;
		PointIO->Tags->Remove(TagStr_ClusterPair);
		Metadata->DeleteAttribute(Tag_VtxEndpoint);
		Metadata->DeleteAttribute(Tag_EdgeEndpoints);
	}
}

namespace PCGExGraphTask
{
#pragma region Graph tasks

	static void WriteSubGraphEdges(
		const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager,
		const TSharedPtr<PCGExGraph::FSubGraph>& SubGraph,
		const PCGExGraph::FGraphMetadataDetails* MetadataDetails);

	class /*PCGEXTENDEDTOOLKIT_API*/ FWriteSubGraphCluster final : public PCGExMT::FPCGExTask
	{
	public:
		FWriteSubGraphCluster(const TSharedPtr<PCGExData::FPointIO>& InPointIO,
		                      const TSharedPtr<PCGExGraph::FSubGraph>& InSubGraph)
			: FPCGExTask(InPointIO),
			  SubGraph(InSubGraph)
		{
		}

		TSharedPtr<PCGExGraph::FSubGraph> SubGraph;
		virtual bool ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) override;
	};

	class /*PCGEXTENDEDTOOLKIT_API*/ FCompileGraph final : public PCGExMT::FPCGExTask
	{
	public:
		FCompileGraph(
			const TSharedPtr<PCGExData::FPointIO>& InPointIO,
			const TSharedPtr<PCGExGraph::FGraphBuilder>& InGraphBuilder,
			const bool bInWriteNodeFacade,
			PCGExGraph::FGraphMetadataDetails* InMetadataDetails = nullptr)
			: FPCGExTask(InPointIO),
			  Builder(InGraphBuilder),
			  bWriteNodeFacade(bInWriteNodeFacade),
			  MetadataDetails(InMetadataDetails)
		{
		}

		TSharedPtr<PCGExGraph::FGraphBuilder> Builder;
		const bool bWriteNodeFacade = false;
		PCGExGraph::FGraphMetadataDetails* MetadataDetails = nullptr;

		virtual bool ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) override;
	};

	class /*PCGEXTENDEDTOOLKIT_API*/ FCopyGraphToPoint final : public PCGExMT::FPCGExTask
	{
	public:
		FCopyGraphToPoint(const TSharedPtr<PCGExData::FPointIO>& InPointIO,
		                  const TSharedPtr<PCGExGraph::FGraphBuilder>& InGraphBuilder,
		                  const TSharedPtr<PCGExData::FPointIOCollection>& InVtxCollection,
		                  const TSharedPtr<PCGExData::FPointIOCollection>& InEdgeCollection,
		                  FPCGExTransformDetails* InTransformDetails) :
			FPCGExTask(InPointIO),
			GraphBuilder(InGraphBuilder),
			VtxCollection(InVtxCollection),
			EdgeCollection(InEdgeCollection),
			TransformDetails(InTransformDetails)
		{
		}

		TSharedPtr<PCGExGraph::FGraphBuilder> GraphBuilder;

		TSharedPtr<PCGExData::FPointIOCollection> VtxCollection;
		TSharedPtr<PCGExData::FPointIOCollection> EdgeCollection;

		FPCGExTransformDetails* TransformDetails = nullptr;

		virtual bool ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) override;
	};

#pragma endregion
}
