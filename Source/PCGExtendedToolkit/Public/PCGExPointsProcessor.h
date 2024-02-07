﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGPin.h"
#include "Elements/PCGPointProcessingElementBase.h"
#include "PCGEx.h"
#include "PCGExMT.h"
#include "Data/PCGExAttributeHelpers.h"
#include "Data/PCGExPointIO.h"
#include "PCGExOperation.h"

#include "PCGExPointsProcessor.generated.h"
#define PCGEX_NODE_INFOS(_SHORTNAME, _NAME, _TOOLTIP)\
virtual FName GetDefaultNodeName() const override { return FName(TEXT(#_SHORTNAME)); } \
virtual FName AdditionalTaskName() const override{ return bCacheResult ? FName(FString("* ")+GetDefaultNodeTitle().ToString()) : FName(GetDefaultNodeTitle().ToString()); }\
virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEx" #_SHORTNAME, "NodeTitle", "PCGEx | " _NAME);} \
virtual FText GetNodeTooltipText() const override{ return NSLOCTEXT("PCGEx" #_SHORTNAME "Tooltip", "NodeTooltip", _TOOLTIP); }

#define PCGEX_NODE_INFOS_CUSTOM_TASKNAME(_SHORTNAME, _NAME, _TOOLTIP, _TASK_NAME)\
virtual FName GetDefaultNodeName() const override { return FName(TEXT(#_SHORTNAME)); } \
virtual FName AdditionalTaskName() const override{ return _TASK_NAME; }\
virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEx" #_SHORTNAME, "NodeTitle", "PCGEx | " _NAME);} \
virtual FText GetNodeTooltipText() const override{ return NSLOCTEXT("PCGEx" #_SHORTNAME "Tooltip", "NodeTooltip", _TOOLTIP); }

#define PCGEX_INITIALIZE_CONTEXT(_NAME)\
FPCGContext* FPCGEx##_NAME##Element::Initialize( const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)\
{	FPCGEx##_NAME##Context* Context = new FPCGEx##_NAME##Context();	return InitializeContext(Context, InputData, SourceComponent, Node); }
#define PCGEX_INITIALIZE_ELEMENT(_NAME)\
PCGEX_INITIALIZE_CONTEXT(_NAME)\
FPCGElementPtr UPCGEx##_NAME##Settings::CreateElement() const{	return MakeShared<FPCGEx##_NAME##Element>();}
#define PCGEX_CONTEXT(_NAME) FPCGEx##_NAME##Context* Context = static_cast<FPCGEx##_NAME##Context*>(InContext);
#define PCGEX_SETTINGS(_NAME) const UPCGEx##_NAME##Settings* Settings = Context->GetInputSettings<UPCGEx##_NAME##Settings>();	check(Settings);
#define PCGEX_SETTINGS_LOCAL(_NAME) const UPCGEx##_NAME##Settings* Settings = GetInputSettings<UPCGEx##_NAME##Settings>();	check(Settings);
#define PCGEX_CONTEXT_AND_SETTINGS(_NAME) PCGEX_CONTEXT(_NAME) PCGEX_SETTINGS(_NAME)
#define PCGEX_OPERATION_DEFAULT(_NAME, _TYPE)  // _NAME = NewObject<_TYPE>(this, TEXT(#_NAME)); //ObjectInitializer.CreateDefaultSubobject<_TYPE>(this, TEXT(#_NAME)); // if(!_NAME){_NAME = NewObject<_TYPE>(); _NAME->UpdateUserFacingInfos();}
#define PCGEX_OPERATION_VALIDATE(_NAME) if(!Settings->_NAME){PCGE_LOG(Error, GraphAndLog, FTEXT("No operation selected for : "#_NAME)); return false;}
#define PCGEX_OPERATION_BIND(_NAME, _TYPE) PCGEX_OPERATION_VALIDATE(_NAME) Context->_NAME = Context->RegisterOperation<_TYPE>(Settings->_NAME);
#define PCGEX_VALIDATE_NAME(_NAME) if (!FPCGMetadataAttributeBase::IsValidName(_NAME) || _NAME.IsNone()){	PCGE_LOG(Error, GraphAndLog, FTEXT("Invalid user-defined attribute name for " #_NAME)); return false;	}
#define PCGEX_FWD(_NAME) Context->_NAME = Settings->_NAME;
#define PCGEX_TERMINATE_ASYNC PCGEX_DELETE(AsyncManager)

struct FPCGExPointsProcessorContext;

namespace PCGEx
{
	struct PCGEXTENDEDTOOLKIT_API FAPointLoop
	{
		virtual ~FAPointLoop() = default;

		FAPointLoop()
		{
		}

		FPCGExPointsProcessorContext* Context = nullptr;

		PCGExData::FPointIO* PointIO = nullptr;

		int32 NumIterations = -1;
		int32 ChunkSize = 32;

		int32 CurrentIndex = -1;
		bool bAsyncEnabled = true;

		inline PCGExData::FPointIO& GetPointIO() const;

		virtual bool Advance(const TFunction<void(PCGExData::FPointIO&)>&& Initialize, const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) = 0;
		virtual bool Advance(const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) = 0;

	protected:
		int32 GetCurrentChunkSize() const
		{
			return FMath::Min(ChunkSize, NumIterations - CurrentIndex);
		}
	};

	struct PCGEXTENDEDTOOLKIT_API FPointLoop : public FAPointLoop
	{
		FPointLoop()
		{
		}

		virtual bool Advance(const TFunction<void(PCGExData::FPointIO&)>&& Initialize, const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
		virtual bool Advance(const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
	};

	struct PCGEXTENDEDTOOLKIT_API FBulkPointLoop : public FPointLoop
	{
		FBulkPointLoop()
		{
		}

		TArray<FPointLoop> SubLoops;

		virtual void Init();
		virtual bool Advance(const TFunction<void(PCGExData::FPointIO&)>&& Initialize, const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
		virtual bool Advance(const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
	};

	struct PCGEXTENDEDTOOLKIT_API FAsyncPointLoop : public FPointLoop
	{
		FAsyncPointLoop()
		{
		}

		virtual bool Advance(const TFunction<void(PCGExData::FPointIO&)>&& Initialize, const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
		virtual bool Advance(const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
	};

	struct PCGEXTENDEDTOOLKIT_API FBulkAsyncPointLoop : public FAsyncPointLoop
	{
		FBulkAsyncPointLoop()
		{
		}

		TArray<FAsyncPointLoop> SubLoops;

		virtual void Init();
		virtual bool Advance(const TFunction<void(PCGExData::FPointIO&)>&& Initialize, const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
		virtual bool Advance(const TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody) override;
	};
}


/**
 * A Base node to process a set of point using GraphParams.
 */
UCLASS(Abstract, BlueprintType, ClassGroup = (Procedural))
class PCGEXTENDEDTOOLKIT_API UPCGExPointsProcessorSettings : public UPCGSettings
{
	GENERATED_BODY()

	friend struct FPCGExPointsProcessorContext;
	friend class FPCGExPointsProcessorElementBase;

public:
	UPCGExPointsProcessorSettings(const FObjectInitializer& ObjectInitializer);

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual bool OnlyPassThroughOneEdgeWhenDisabled() const override;
	//~End UPCGSettings interface

	//~Begin UObject interface
#if WITH_EDITOR

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End UObject interface

	//~Begin UPCGExPointsProcessorSettings interface
public:
	virtual FName GetMainInputLabel() const;
	virtual FName GetMainOutputLabel() const;
	virtual bool GetMainAcceptMultipleData() const;
	virtual PCGExData::EInit GetMainOutputInitMode() const;

	/** Forces execution on main thread. Work is still chunked. Turning this off ensure linear order of operations, and, in most case, determinism.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Performance")
	bool bDoAsyncProcessing = true;

	/** Chunk size for parallel processing. <1 switches to preferred node value.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Performance", meta=(ClampMin=-1, ClampMax=8196))
	int32 ChunkSize = -1;

	/** Cache the results of this node. Can yield unexpected result in certain cases.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Performance")
	bool bCacheResult = false;

	template <typename T>
	static T* EnsureOperation(UPCGExOperation* Operation) { return Operation ? static_cast<T*>(Operation) : NewObject<T>(); }

protected:
	virtual int32 GetPreferredChunkSize() const;
	//~End UPCGExPointsProcessorSettings interface
};

struct PCGEXTENDEDTOOLKIT_API FPCGExPointsProcessorContext : public FPCGContext
{
	friend class FPCGExPointsProcessorElementBase;

	virtual ~FPCGExPointsProcessorContext() override;

	UWorld* World = nullptr;

	mutable FRWLock ContextLock;
	PCGExData::FPointIOGroup* MainPoints = nullptr;

	PCGExData::FPointIO* CurrentIO = nullptr;

	const UPCGPointData* GetCurrentIn() const { return CurrentIO->GetIn(); }
	UPCGPointData* GetCurrentOut() const { return CurrentIO->GetOut(); }

	virtual bool AdvancePointsIO();
	PCGExMT::AsyncState GetState() const { return CurrentState; }
	bool IsState(const PCGExMT::AsyncState OperationId) const { return CurrentState == OperationId; }
	bool IsSetup() const { return IsState(PCGExMT::State_Setup); }
	bool IsDone() const { return IsState(PCGExMT::State_Done); }
	virtual void Done();

	FPCGExAsyncManager* GetAsyncManager();
	void SetAsyncState(const PCGExMT::AsyncState WaitState) { SetState(WaitState, false); }

	virtual void SetState(PCGExMT::AsyncState OperationId, bool bResetAsyncWork = true);
	virtual void Reset();

	int32 ChunkSize = 0;
	bool bDoAsyncProcessing = true;

	void OutputPoints() { MainPoints->OutputTo(this); }

	bool BulkProcessMainPoints(TFunction<void(PCGExData::FPointIO&)>&& Initialize, TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody);
	bool ProcessCurrentPoints(TFunction<void(PCGExData::FPointIO&)>&& Initialize, TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody, bool bForceSync = false);
	bool ProcessCurrentPoints(TFunction<void(const int32, const PCGExData::FPointIO&)>&& LoopBody, bool bForceSync = false);

	template <class InitializeFunc, class LoopBodyFunc>
	bool Process(InitializeFunc&& Initialize, LoopBodyFunc&& LoopBody, const int32 NumIterations, const bool bForceSync = false)
	{
		AsyncLoop.NumIterations = NumIterations;
		AsyncLoop.bAsyncEnabled = bDoAsyncProcessing && !bForceSync;
		return AsyncLoop.Advance(Initialize, LoopBody);
	}

	template <class LoopBodyFunc>
	bool Process(LoopBodyFunc&& LoopBody, const int32 NumIterations, const bool bForceSync = false)
	{
		AsyncLoop.NumIterations = NumIterations;
		AsyncLoop.bAsyncEnabled = bDoAsyncProcessing && !bForceSync;
		return AsyncLoop.Advance(LoopBody);
	}

	void Output(FPCGTaggedData& OutTaggedData, UPCGData* OutData, const FName OutputLabel);
	FPCGTaggedData* Output(UPCGData* OutData, const FName OutputLabel);
	void Output(PCGExData::FPointIO& PointIO);

	template <typename T>
	T MakeLoop()
	{
		T Loop = T{};
		Loop.Context = this;
		Loop.ChunkSize = ChunkSize;
		Loop.bAsyncEnabled = bDoAsyncProcessing;
		return Loop;
	}

	template <typename T>
	T* RegisterOperation(UPCGExOperation* Operation = nullptr)
	{
		T* RetValue;
		if (!Operation)
		{
			RetValue = NewObject<T>();
			OwnedProcessorOperations.Add(RetValue);
		}
		else
		{
			RetValue = static_cast<T*>(Operation);
		}
		RetValue->BindContext(this);
		return RetValue;
	}

protected:
	PCGExMT::FAsyncParallelLoop AsyncLoop;
	FPCGExAsyncManager* AsyncManager = nullptr;

	PCGEx::FPointLoop ChunkedPointLoop;
	PCGEx::FAsyncPointLoop AsyncPointLoop;
	PCGEx::FBulkAsyncPointLoop BulkAsyncPointLoop;

	PCGExMT::AsyncState CurrentState;
	int32 CurrentPointsIndex = -1;

	TArray<UPCGExOperation*> ProcessorOperations;
	TSet<UPCGExOperation*> OwnedProcessorOperations;

	void CleanupOperations();
	virtual void ResetAsyncWork();

public:
	virtual bool IsAsyncWorkComplete();
};

class PCGEXTENDEDTOOLKIT_API FPCGExPointsProcessorElementBase : public FPCGPointProcessingElementBase
{
public:
	virtual FPCGContext* Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node) override;

#if WITH_EDITOR
	virtual bool ShouldLog() const override { return false; }
#endif

	virtual bool IsCacheable(const UPCGSettings* InSettings) const override
	{
		const UPCGExPointsProcessorSettings* Settings = static_cast<const UPCGExPointsProcessorSettings*>(InSettings);
		return Settings->bCacheResult;
	}

	virtual void DisabledPassThroughData(FPCGContext* Context) const override;

protected:
	virtual FPCGContext* InitializeContext(FPCGExPointsProcessorContext* InContext, const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node) const;
	virtual bool Boot(FPCGContext* InContext) const;
};
