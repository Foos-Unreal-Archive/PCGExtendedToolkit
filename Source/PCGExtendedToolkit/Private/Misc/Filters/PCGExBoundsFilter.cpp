﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/Filters/PCGExBoundsFilter.h"


#define LOCTEXT_NAMESPACE "PCGExBoundsFilterDefinition"
#define PCGEX_NAMESPACE PCGExBoundsFilterDefinition

bool UPCGExBoundsFilterFactory::Init(FPCGExContext* InContext)
{
	if (!Super::Init(InContext)) { return false; }
	if (const TSharedPtr<PCGExData::FPointIO> BoundsIO = PCGExData::TryGetSingleInput(InContext, FName("Bounds"), true))
	{
		BoundsDataFacade = MakeShared<PCGExData::FFacade>(BoundsIO.ToSharedRef());
	}
	else
	{
		return false;
	}

	return true;
}

TSharedPtr<PCGExPointFilter::FFilter> UPCGExBoundsFilterFactory::CreateFilter() const
{
	return MakeShared<PCGExPointsFilter::TBoundsFilter>(this);
}

void UPCGExBoundsFilterFactory::BeginDestroy()
{
	Super::BeginDestroy();
}

bool PCGExPointsFilter::TBoundsFilter::Init(const FPCGContext* InContext, const TSharedPtr<PCGExData::FFacade> InPointDataFacade)
{
	if (!FFilter::Init(InContext, InPointDataFacade)) { return false; }
	if (!Cloud) { return false; }

	BoundsTarget = TypedFilterFactory->Config.BoundsTarget;

#define PCGEX_FOREACH_BOUNDTYPE(_NAME)\
	switch (TypedFilterFactory->Config.BoundsSource) { default: \
	case EPCGExPointBoundsSource::ScaledBounds: BoundCheck = [&](const FPCGPoint& Point) { return Cloud->_NAME<EPCGExPointBoundsSource::ScaledBounds>(Point); }; break;\
	case EPCGExPointBoundsSource::DensityBounds: BoundCheck = [&](const FPCGPoint& Point) { return Cloud->_NAME<EPCGExPointBoundsSource::DensityBounds>(Point); }; break;\
	case EPCGExPointBoundsSource::Bounds: BoundCheck = [&](const FPCGPoint& Point) { return Cloud->_NAME<EPCGExPointBoundsSource::Bounds>(Point); }; break;}

	switch (TypedFilterFactory->Config.CheckType)
	{
	default:
	case EPCGExBoundsCheckType::Intersects:
		PCGEX_FOREACH_BOUNDTYPE(Intersect)
		break;
	case EPCGExBoundsCheckType::IsInside:
		PCGEX_FOREACH_BOUNDTYPE(IsInside)
		break;
	case EPCGExBoundsCheckType::IsInsideOrOn:
		PCGEX_FOREACH_BOUNDTYPE(IsInsideOrOn)
		break;
	case EPCGExBoundsCheckType::IsInsideOrIntersects:
		PCGEX_FOREACH_BOUNDTYPE(IsInsideOrIntersects)
		break;
	}

#undef PCGEX_FOREACH_BOUNDTYPE

	return true;
}

TArray<FPCGPinProperties> UPCGExBoundsFilterProviderSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	PCGEX_PIN_POINT(FName("Bounds"), TEXT("Points which bounds will be used for testing"), Required, {})
	return PinProperties;
}

PCGEX_CREATE_FILTER_FACTORY(Bounds)

#if WITH_EDITOR
FString UPCGExBoundsFilterProviderSettings::GetDisplayName() const
{
	switch (Config.CheckType)
	{
	default:
	case EPCGExBoundsCheckType::Intersects: return TEXT("Intersects");
	case EPCGExBoundsCheckType::IsInside: return TEXT("Is Inside");
	case EPCGExBoundsCheckType::IsInsideOrOn: return TEXT("Is Inside or On");
	case EPCGExBoundsCheckType::IsInsideOrIntersects: return TEXT("Is Inside or Intersects");
	}
}
#endif

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
