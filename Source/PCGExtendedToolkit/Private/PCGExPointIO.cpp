﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "PCGExPointIO.h"
#include "PCGContext.h"

UPCGExPointIO::UPCGExPointIO(): In(nullptr), Out(nullptr), NumPoints(-1)
{
	// Initialize other members as needed
}

void UPCGExPointIO::InitializeOut(PCGEx::EIOInit InitOut)
{
	switch (InitOut)
	{
	case PCGEx::EIOInit::NoOutput:
		break;
	case PCGEx::EIOInit::NewOutput:
		Out = NewObject<UPCGPointData>();
		if (In) { Out->InitializeFromData(In); }
		break;
	case PCGEx::EIOInit::DuplicateInput:
		if (In)
		{
			Out = Cast<UPCGPointData>(In->DuplicateData(true));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Initialize::Duplicate, but no Input."));
		}
		break;
	case PCGEx::EIOInit::Forward:
		Out = In;
		break;
	default: ;
	}
	if (In) { NumPoints = In->GetPoints().Num(); }
}

void UPCGExPointIO::BuildIndices()
{
	if (bMetadataEntryDirty) { BuildMetadataEntries(); }
	if (!bIndicesDirty) { return; }
	FWriteScopeLock ScopeLock(MapLock);
	const TArray<FPCGPoint>& Points = Out->GetPoints();
	IndicesMap.Empty(NumPoints);
	for (int i = 0; i < NumPoints; i++) { IndicesMap.Add(Points[i].MetadataEntry, i); }
	bIndicesDirty = false;
}

void UPCGExPointIO::BuildMetadataEntries()
{
	if (!bMetadataEntryDirty) { return; }
	TArray<FPCGPoint>& Points = Out->GetMutablePoints();
	for (int i = 0; i < NumPoints; i++)
	{
		FPCGPoint& Point = Points[i];
		Out->Metadata->InitializeOnSet(Point.MetadataEntry, In->GetPoint(i).MetadataEntry, In->Metadata);
	}
	bMetadataEntryDirty = false;
	bIndicesDirty = true;
}

void UPCGExPointIO::BuildMetadataEntriesAndIndices()
{
	if (bMetadataEntryDirty) { BuildMetadataEntries(); }
	if (!bIndicesDirty) { return; }
	FWriteScopeLock ScopeLock(MapLock);
	TArray<FPCGPoint>& Points = Out->GetMutablePoints();
	IndicesMap.Empty(NumPoints);
	for (int i = 0; i < NumPoints; i++)
	{
		FPCGPoint& Point = Points[i];
		Out->Metadata->InitializeOnSet(Point.MetadataEntry, In->GetPoint(i).MetadataEntry, In->Metadata);
		IndicesMap.Add(Point.MetadataEntry, i);
	}
	bIndicesDirty = false;
}

void UPCGExPointIO::ClearIndices()
{
	IndicesMap.Empty();
}

int32 UPCGExPointIO::GetIndex(PCGMetadataEntryKey Key) const
{
	FReadScopeLock ScopeLock(MapLock);
	return *(IndicesMap.Find(Key));
}

template <class InitializeFunc, class ProcessElementFunc>
bool UPCGExPointIO::OutputParallelProcessing(
	FPCGContext* Context,
	InitializeFunc&& Initialize,   //TFunction<void(const UPCGExPointIO* PointIO)>
	ProcessElementFunc&& LoopBody, //TFunction<void(const FPCGPoint&, int32, UPCGExPointIO*)>
	const int32 ChunkSize)
{
	auto InnerInitialize = [this, &Initialize]()
	{
		bParallelProcessing = true;
		Initialize(this);
	};

	auto InnerBodyLoop = [this, &LoopBody](const int32 ReadIndex, const int32 WriteIndex)
	{
		FReadScopeLock ScopeLock(MapLock);
		const FPCGPoint& Point = Out->GetPoint(ReadIndex);
		if (bIndicesDirty) { LoopBody(Point, ReadIndex, this); }
		else { LoopBody(Point, *(IndicesMap.Find(Point.MetadataEntry)), this); }
		return true;
	};

	const bool bProcessingDone = FPCGAsync::AsyncProcessingOneToOneEx(&(Context->AsyncState), NumPoints, InnerInitialize, InnerBodyLoop, true, ChunkSize);

	if (bProcessingDone) { bParallelProcessing = false; }
	return bProcessingDone;
}

template <class InitializeFunc, class ProcessElementFunc>
bool UPCGExPointIO::InputParallelProcessing(
	FPCGContext* Context,
	InitializeFunc&& Initialize,   //TFunction<void(UPCGExPointIO* PointIO)>
	ProcessElementFunc&& LoopBody, //TFunction<void(const FPCGPoint&, int32, UPCGExPointIO*)>
	const int32 ChunkSize)
{
	auto InnerInitialize = [this, &Initialize]()
	{
		bParallelProcessing = true;
		Initialize(this);
	};

	auto InnerBodyLoop = [this, &LoopBody](const int32 ReadIndex, const int32 WriteIndex)
	{
		FReadScopeLock ScopeLock(MapLock);
		const FPCGPoint& Point = In->GetPoint(ReadIndex);
		LoopBody(Point, ReadIndex, this);
		return true;
	};

	const bool bProcessingDone = FPCGAsync::AsyncProcessingOneToOneEx(&(Context->AsyncState), NumPoints, InnerInitialize, InnerBodyLoop, true, ChunkSize);

	if (bProcessingDone) { bParallelProcessing = false; }
	return bProcessingDone;
}

bool UPCGExPointIO::OutputTo(FPCGContext* Context, bool bEmplace)
{
	if (!Out || Out->GetPoints().Num() == 0) { return false; }

	if (!bEmplace)
	{
		if (!In)
		{
			UE_LOG(LogTemp, Error, TEXT("OutputTo, bEmplace==false but no Input."));
			return false;
		}

		FPCGTaggedData& OutputRef = Context->OutputData.TaggedData.Add_GetRef(Source);
		OutputRef.Data = Out;
		OutputRef.Pin = DefaultOutputLabel;
		Output = OutputRef;
	}
	else
	{
		FPCGTaggedData& OutputRef = Context->OutputData.TaggedData.Emplace_GetRef();
		OutputRef.Data = Out;
		OutputRef.Pin = DefaultOutputLabel;
		Output = OutputRef;
	}
	return true;
}

bool UPCGExPointIO::OutputTo(FPCGContext* Context, bool bEmplace, const int64 MinPointCount, const int64 MaxPointCount)
{
	if (!Out) { return false; }

	const int64 OutNumPoints = Out->GetPoints().Num();
	if (MinPointCount >= 0 && OutNumPoints < MinPointCount) { return false; }
	if (MaxPointCount >= 0 && OutNumPoints > MaxPointCount) { return false; }

	return OutputTo(Context, bEmplace);
}

UPCGExPointIOGroup::UPCGExPointIOGroup()
{
}

UPCGExPointIOGroup::UPCGExPointIOGroup(FPCGContext* Context, FName InputLabel, PCGEx::EIOInit InitOut)
	: UPCGExPointIOGroup::UPCGExPointIOGroup()
{
	TArray<FPCGTaggedData> Sources = Context->InputData.GetInputsByPin(InputLabel);
	Initialize(Context, Sources, InitOut);
}

UPCGExPointIOGroup::UPCGExPointIOGroup(FPCGContext* Context, TArray<FPCGTaggedData>& Sources, PCGEx::EIOInit InitOut)
	: UPCGExPointIOGroup::UPCGExPointIOGroup()
{
	Initialize(Context, Sources, InitOut);
}

void UPCGExPointIOGroup::Initialize(
	FPCGContext* Context, TArray<FPCGTaggedData>& Sources,
	PCGEx::EIOInit InitOut)
{
	Pairs.Empty(Sources.Num());
	for (FPCGTaggedData& Source : Sources)
	{
		UPCGPointData* MutablePointData = GetMutablePointData(Context, Source);
		if (!MutablePointData || MutablePointData->GetPoints().Num() == 0) { continue; }
		Emplace_GetRef(Source, MutablePointData, InitOut);
	}
}

void UPCGExPointIOGroup::Initialize(
	FPCGContext* Context, TArray<FPCGTaggedData>& Sources,
	PCGEx::EIOInit InitOut,
	const TFunction<bool(UPCGPointData*)>& ValidateFunc,
	const TFunction<void(UPCGExPointIO*)>& PostInitFunc)
{
	Pairs.Empty(Sources.Num());
	for (FPCGTaggedData& Source : Sources)
	{
		UPCGPointData* MutablePointData = GetMutablePointData(Context, Source);
		if (!MutablePointData || MutablePointData->GetPoints().Num() == 0) { continue; }
		if (!ValidateFunc(MutablePointData)) { continue; }
		UPCGExPointIO* NewPointIO = Emplace_GetRef(Source, MutablePointData, InitOut);
		PostInitFunc(NewPointIO);
	}
}

UPCGExPointIO* UPCGExPointIOGroup::Emplace_GetRef(
	const UPCGExPointIO& IO,
	const PCGEx::EIOInit InitOut)
{
	return Emplace_GetRef(IO.Source, IO.In, InitOut);
}

UPCGExPointIO* UPCGExPointIOGroup::Emplace_GetRef(
	const FPCGTaggedData& Source, UPCGPointData* In,
	const PCGEx::EIOInit InitOut)
{
	//FWriteScopeLock ScopeLock(PairsLock);

	UPCGExPointIO* Pair = NewObject<UPCGExPointIO>();
	Pairs.Add(Pair);

	Pair->DefaultOutputLabel = DefaultOutputLabel;
	Pair->Source = Source;
	Pair->In = In;
	Pair->NumPoints = Pair->In->GetPoints().Num();

	Pair->InitializeOut(InitOut);
	return Pair;
}

/**
 * Write valid outputs to Context' tagged data
 * @param Context
 * @param bEmplace Emplace will create a new entry no matter if a Source is set, otherwise will match the In.Source. 
 */
void UPCGExPointIOGroup::OutputTo(FPCGContext* Context, bool bEmplace)
{
	for (UPCGExPointIO* Pair
	     :
	     Pairs
	)
	{
		Pair->OutputTo(Context, bEmplace);
	}
}

/**
 * Write valid outputs to Context' tagged data
 * @param Context
 * @param bEmplace Emplace will create a new entry no matter if a Source is set, otherwise will match the In.Source.
 * @param MinPointCount
 * @param MaxPointCount 
 */
void UPCGExPointIOGroup::OutputTo(FPCGContext* Context, bool bEmplace, const int64 MinPointCount, const int64 MaxPointCount)
{
	for (UPCGExPointIO* Pair
	     :
	     Pairs
	)
	{
		Pair->OutputTo(Context, bEmplace, MinPointCount, MaxPointCount);
	}
}

void UPCGExPointIOGroup::ForEach(const TFunction<void(UPCGExPointIO*, const int32)>& BodyLoop)
{
	for (int i = 0; i < Pairs.Num(); i++)
	{
		UPCGExPointIO* PIOPair = Pairs[i];
		BodyLoop(PIOPair, i);
	}
}

UPCGPointData* UPCGExPointIOGroup::GetMutablePointData(FPCGContext* Context, const FPCGTaggedData& Source)
{
	const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Source.Data);
	if (!SpatialData) { return nullptr; }

	const UPCGPointData* PointData = SpatialData->ToPointData(Context);
	if (!PointData) { return nullptr; }

	return const_cast<UPCGPointData*>(PointData);
}

template <typename InitializeFunc, typename ProcessElementFunc>
bool UPCGExPointIOGroup::OutputsParallelProcessing(
	FPCGContext* Context,
	InitializeFunc&& Initialize,   //TFunction<void(UPCGExPointIO*)>
	ProcessElementFunc&& LoopBody, //TFunction<void(const FPCGPoint&, int32, UPCGExPointIO*)>
	int32 ChunkSize)
{
	const int32 NumPairs = Pairs.Num();

	if (!bProcessing)
	{
		bProcessing = true;
		PairProcessingStatuses.Empty(NumPairs);
		for (int i = 0; i < NumPairs; i++) { PairProcessingStatuses.Add(false); }
	}

	int32 NumPairsDone = 0;

	for (int i = 0; i < NumPairs; i++)
	{
		bool bState = PairProcessingStatuses[i];
		if (!bState)
		{
			bState = Pairs[i]->OutputParallelProcessing(Context, Initialize, LoopBody, ChunkSize);
			if (bState) { PairProcessingStatuses[i] = bState; }
		}
		if (bState) { NumPairsDone++; }
	}

	if (NumPairs == NumPairsDone)
	{
		bProcessing = false;
		return true;
	}
	else
	{
		return false;
	}
}

template <typename InitializeFunc, typename ProcessElementFunc>
bool UPCGExPointIOGroup::InputsParallelProcessing(
	FPCGContext* Context,
	InitializeFunc&& Initialize,   //TFunction<void(UPCGExPointIO*)>
	ProcessElementFunc&& LoopBody, //TFunction<void(const FPCGPoint&, int32, UPCGExPointIO*)>
	int32 ChunkSize)
{
	const int32 NumPairs = Pairs.Num();

	if (!bProcessing)
	{
		bProcessing = true;
		PairProcessingStatuses.Empty(NumPairs);
		for (int i = 0; i < NumPairs; i++) { PairProcessingStatuses.Add(false); }
	}

	int32 NumPairsDone = 0;

	for (int i = 0; i < NumPairs; i++)
	{
		bool bState = PairProcessingStatuses[i];
		if (!bState)
		{
			bState = Pairs[i]->InputParallelProcessing(Context, Initialize, LoopBody, ChunkSize);
			if (bState) { PairProcessingStatuses[i] = bState; }
		}
		if (bState) { NumPairsDone++; }
	}

	if (NumPairs == NumPairsDone)
	{
		bProcessing = false;
		return true;
	}
	else
	{
		return false;
	}
}
