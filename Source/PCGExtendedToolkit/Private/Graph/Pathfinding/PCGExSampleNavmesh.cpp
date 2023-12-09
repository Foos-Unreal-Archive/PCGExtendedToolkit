﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Pathfinding/PCGExSampleNavmesh.h"

#include "NavigationSystem.h"

#include "PCGExPointsProcessor.h"
#include "Graph/PCGExGraph.h"
#include "Graph/Pathfinding/PCGExPathfinding.h"
#include "Graph/Pathfinding/GoalPickers/PCGExGoalPickerRandom.h"
#include "Splines/SubPoints/DataBlending/PCGExSubPointsBlendInterpolate.h"

#define LOCTEXT_NAMESPACE "PCGExSampleNavmeshElement"

UPCGExSampleNavmeshSettings::UPCGExSampleNavmeshSettings(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GoalPicker = EnsureInstruction<UPCGExGoalPickerRandom>(GoalPicker);
	Blending = EnsureInstruction<UPCGExSubPointsBlendInterpolate>(Blending);
}

TArray<FPCGPinProperties> UPCGExSampleNavmeshSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	FPCGPinProperties& PinPropertySeeds = PinProperties.Emplace_GetRef(PCGExPathfinding::SourceSeedsLabel, EPCGDataType::Point, false, false);

#if WITH_EDITOR
	PinPropertySeeds.Tooltip = LOCTEXT("PCGExSourceSeedsPinTooltip", "Seeds points for pathfinding.");
#endif // WITH_EDITOR

	FPCGPinProperties& PinPropertyGoals = PinProperties.Emplace_GetRef(PCGExPathfinding::SourceGoalsLabel, EPCGDataType::Point, false, false);

#if WITH_EDITOR
	PinPropertyGoals.Tooltip = LOCTEXT("PCGExSourcGoalsPinTooltip", "Goals points for pathfinding.");
#endif // WITH_EDITOR

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGExSampleNavmeshSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& PinPathsOutput = PinProperties.Emplace_GetRef(PCGExGraph::OutputPathsLabel, EPCGDataType::Point);

#if WITH_EDITOR
	PinPathsOutput.Tooltip = LOCTEXT("PCGExOutputPathsTooltip", "Paths output.");
#endif // WITH_EDITOR

	return PinProperties;
}

void UPCGExSampleNavmeshSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	GoalPicker = EnsureInstruction<UPCGExGoalPickerRandom>(GoalPicker);
	Blending = EnsureInstruction<UPCGExSubPointsBlendInterpolate>(Blending);
	if (GoalPicker) { GoalPicker->UpdateUserFacingInfos(); }
	if (Blending) { Blending->UpdateUserFacingInfos(); }
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

PCGExPointIO::EInit UPCGExSampleNavmeshSettings::GetPointOutputInitMode() const { return PCGExPointIO::EInit::NoOutput; }
int32 UPCGExSampleNavmeshSettings::GetPreferredChunkSize() const { return 32; }

FName UPCGExSampleNavmeshSettings::GetMainPointsInputLabel() const { return PCGExPathfinding::SourceSeedsLabel; }
FName UPCGExSampleNavmeshSettings::GetMainPointsOutputLabel() const { return PCGExGraph::OutputPathsLabel; }

FPCGExSampleNavmeshContext::~FPCGExSampleNavmeshContext()
{
	PathBuffer.Empty();
	delete GoalsPoints;
	delete OutputPaths;
}

FPCGElementPtr UPCGExSampleNavmeshSettings::CreateElement() const { return MakeShared<FPCGExSampleNavmeshElement>(); }

FPCGContext* FPCGExSampleNavmeshElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGExSampleNavmeshContext* Context = new FPCGExSampleNavmeshContext();
	InitializeContext(Context, InputData, SourceComponent, Node);

	const UPCGExSampleNavmeshSettings* Settings = Context->GetInputSettings<UPCGExSampleNavmeshSettings>();
	check(Settings);

	if (TArray<FPCGTaggedData> Goals = Context->InputData.GetInputsByPin(PCGExPathfinding::SourceGoalsLabel);
		Goals.Num() > 0)
	{
		const FPCGTaggedData& GoalsSource = Goals[0];
		Context->GoalsPoints = PCGExPointIO::GetPointIO(Context, GoalsSource);
	}

	if (!Settings->NavData)
	{
		if (const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Context->World))
		{
			ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
			Context->NavData = NavData;
		}
	}

	Context->OutputPaths = new FPCGExPointIOGroup();

	Context->GoalPicker = Settings->EnsureInstruction<UPCGExGoalPickerRandom>(Settings->GoalPicker, Context);
	Context->Blending = Settings->EnsureInstruction<UPCGExSubPointsBlendInterpolate>(Settings->Blending, Context);

	Context->bAddSeedToPath = Settings->bAddSeedToPath;
	Context->bAddGoalToPath = Settings->bAddGoalToPath;

	Context->NavAgentProperties = Settings->NavAgentProperties;
	Context->bRequireNavigableEndLocation = Settings->bRequireNavigableEndLocation;
	Context->PathfindingMode = Settings->PathfindingMode;

	Context->FuseDistance = Settings->FuseDistance * Settings->FuseDistance;

	return Context;
}

bool FPCGExSampleNavmeshElement::Validate(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Validate(InContext)) { return false; }

	const FPCGExSampleNavmeshContext* Context = static_cast<FPCGExSampleNavmeshContext*>(InContext);
	const UPCGExSampleNavmeshSettings* Settings = InContext->GetInputSettings<UPCGExSampleNavmeshSettings>();
	check(Settings);

	if (!Context->GoalsPoints || Context->GoalsPoints->GetNum() == 0)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingGoals", "Missing Input Goals."));
		return false;
	}

	if (!Context->NavData)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoNavData", "Missing Nav Data"));
		return false;
	}

	return true;
}

bool FPCGExSampleNavmeshElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExSampleNavmeshElement::Execute);

	FPCGExSampleNavmeshContext* Context = static_cast<FPCGExSampleNavmeshContext*>(InContext);

	if (Context->IsSetup())
	{
		if (!Validate(Context)) { return true; }
		Context->AdvancePointsIO();
		Context->GoalPicker->PrepareForData(Context->CurrentIO->GetIn(), Context->GoalsPoints->GetIn());
		//Context->Blending->PrepareForData(Context->CurrentIO->GetIn(), Context->GoalsPoints->GetIn());
		//TODO: Cannot prepare blending from const In. Must have Out ready first.
		Context->SetState(PCGExMT::State_ProcessingPoints);
	}

	if (Context->IsState(PCGExMT::State_ProcessingPoints))
	{
		auto ProcessSeed = [&](const int32 PointIndex, const FPCGExPointIO& PointIO)
		{
			auto NavMeshTask = [&](int32 InGoalIndex)
			{
				Context->BufferLock.WriteLock();
				PCGExSampleNavmesh::FPath& PathObject = Context->PathBuffer.Emplace_GetRef(
					PointIndex, PointIO.GetInPoint(PointIndex).Transform.GetLocation(),
					InGoalIndex, Context->GoalsPoints->GetInPoint(InGoalIndex).Transform.GetLocation());
				Context->BufferLock.WriteUnlock();

				Context->GetAsyncManager()->StartSync<FNavmeshPathTask>(PointIndex, PointIO.GetInPoint(PointIndex).MetadataEntry, Context->CurrentIO, &PathObject);
			};

			if (Context->GoalPicker->OutputMultipleGoals())
			{
				TArray<int32> GoalIndices;
				Context->GoalPicker->GetGoalIndices(PointIO.GetInPoint(PointIndex), GoalIndices);
				for (const int32 GoalIndex : GoalIndices)
				{
					if (GoalIndex < 0) { continue; }
					NavMeshTask(GoalIndex);
				}
			}
			else
			{
				const int32 GoalIndex = Context->GoalPicker->GetGoalIndex(PointIO.GetInPoint(PointIndex), PointIndex);
				if (GoalIndex < 0) { return; }
				NavMeshTask(GoalIndex);
			}
		};

		if (Context->ProcessCurrentPoints(ProcessSeed)) { Context->SetAsyncState(PCGExSampleNavmesh::State_Pathfinding); }
	}

	if (Context->IsState(PCGExSampleNavmesh::State_Pathfinding))
	{
		if (Context->IsAsyncWorkComplete()) { Context->SetState(PCGExSampleNavmesh::State_PathBlending); }
	}

	if (Context->IsState(PCGExSampleNavmesh::State_PathBlending))
	{
		auto ProcessPath = [&](const int32 Index)
		{
			PCGExSampleNavmesh::FPath& Path = Context->PathBuffer[Index];
			
			if (Path.Positions.IsEmpty()) { return; }

			FPCGExPointIO& PathPoints = Context->OutputPaths->Emplace_GetRef(Context->GetCurrentIn(), PCGExPointIO::EInit::NewOutput);

			const int32 NumPositions = Path.Positions.Num();
			const int32 LastPosition = NumPositions - 1;
			TArray<FPCGPoint>& MutablePoints = PathPoints.GetOut()->GetMutablePoints();
			MutablePoints.SetNum(NumPositions);

			const FPCGPoint& Seed = Context->CurrentIO->GetInPoint(Path.SeedIndex);
			const FPCGPoint& Goal = Context->GoalsPoints->GetInPoint(Path.GoalIndex);

			for (int i = 0; i < LastPosition; i++)
			{
				(MutablePoints[i] = Seed).Transform.SetLocation(Path.Positions[i]);
			}

			(MutablePoints[LastPosition] = Goal).Transform.SetLocation(Path.Positions[LastPosition]);

			const PCGExDataBlending::FMetadataBlender* TempBlender = Context->Blending->CreateBlender(
				PathPoints.GetOut(),
				Context->GoalsPoints->GetIn(),
				PathPoints.GetOutKeys(),
				Context->GoalsPoints->GetInKeys());


			TArrayView<FPCGPoint> View(MutablePoints);
			Context->Blending->BlendSubPoints(View, Path.Infos, TempBlender);

			delete TempBlender;

			if (!Context->bAddSeedToPath) { MutablePoints.RemoveAt(0); }
			if (!Context->bAddGoalToPath) { MutablePoints.Pop(); }
			
		};

		if (Context->Process(ProcessPath, Context->PathBuffer.Num()))
		{
			Context->SetState(PCGExMT::State_Done);
		}
	}

	if (Context->IsDone())
	{
		Context->OutputPaths->OutputTo(Context, true);
		return true;
	}

	return false;
}

bool FNavmeshPathTask::ExecuteTask()
{
	
	FPCGExSampleNavmeshContext* Context = Manager->GetContext<FPCGExSampleNavmeshContext>();

	bool bSuccess = false;

	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Context->World))
	{
		const FPCGPoint& StartPoint = PointIO->GetInPoint(TaskInfos.Index);
		const FPCGPoint& EndPoint = Context->GoalsPoints->GetInPoint(Path->GoalIndex);
		const FVector StartLocation = StartPoint.Transform.GetLocation();
		const FVector EndLocation = EndPoint.Transform.GetLocation();

		// Find the path
		FPathFindingQuery PathFindingQuery = FPathFindingQuery(
			Context->World, *Context->NavData,
			StartLocation, EndLocation, nullptr, nullptr,
			TNumericLimits<FVector::FReal>::Max(),
			Context->bRequireNavigableEndLocation);

		PathFindingQuery.NavAgentProperties = Context->NavAgentProperties;

		PCGEX_ASYNC_LIFE_CHECK
		
		const FPathFindingResult Result = NavSys->FindPathSync(
			Context->NavAgentProperties, PathFindingQuery,
			Context->PathfindingMode == EPCGExNavmeshPathfindingMode::Regular ? EPathFindingMode::Type::Regular : EPathFindingMode::Type::Hierarchical);

		PCGEX_ASYNC_LIFE_CHECK

		if (Result.Result == ENavigationQueryResult::Type::Success)
		{
			const TArray<FNavPathPoint>& Points = Result.Path->GetPathPoints();
			TArray<FVector> PathLocations;
			PathLocations.Reserve(Points.Num());

			PathLocations.Add(StartLocation);
			for (FNavPathPoint PathPoint : Points) { PathLocations.Add(PathPoint.Location); }
			PathLocations.Add(EndLocation);

			PCGExMath::FPathInfos PathHelper = PCGExMath::FPathInfos(StartLocation);
			int32 FuseCountReduce = Context->bAddGoalToPath ? 2 : 1;
			for (int i = Context->bAddSeedToPath; i < PathLocations.Num(); i++)
			{
				FVector CurrentLocation = PathLocations[i];
				if (i > 0 && i < (PathLocations.Num() - FuseCountReduce))
				{
					if (PathHelper.IsLastWithinRange(CurrentLocation, Context->FuseDistance))
					{
						// Fuse
						PathLocations.RemoveAt(i);
						i--;
						continue;
					}
				}

				PathHelper.Add(CurrentLocation);
			}

			if (PathLocations.Num() <= 2) // include start and end
			{
				bSuccess = false;
			}
			else
			{
				PCGEX_ASYNC_LIFE_CHECK
				Path->Positions.Reserve(PathLocations.Num());
				for (FVector Location : PathLocations) { Path->Add(Location); }				
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

#undef LOCTEXT_NAMESPACE
