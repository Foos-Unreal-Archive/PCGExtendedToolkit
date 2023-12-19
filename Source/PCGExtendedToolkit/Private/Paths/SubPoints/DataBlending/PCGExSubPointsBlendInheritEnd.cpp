﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/


#include "Paths/SubPoints/DataBlending/PCGExSubPointsBlendInheritEnd.h"
#include "Data/Blending/PCGExMetadataBlender.h"

void UPCGExSubPointsBlendInheritEnd::BlendSubPoints(
	const PCGEx::FPointRef& StartPoint,
	const PCGEx::FPointRef& EndPoint,
	TArrayView<FPCGPoint>& SubPoints,
	const PCGExMath::FPathMetrics& Metrics,
	const PCGExDataBlending::FMetadataBlender* InBlender) const
{
	const int32 NumPoints = SubPoints.Num();
	PCGExDataBlending::FPropertiesBlender LocalPropertiesBlender = PCGExDataBlending::FPropertiesBlender(PropertiesBlender);

	TArray<double> Alphas;
	TArray<FVector> Locations;

	Alphas.Reserve(NumPoints);
	Locations.Reserve(NumPoints);

	for (const FPCGPoint& Point : SubPoints)
	{
		Locations.Add(Point.Transform.GetLocation());
		Alphas.Add(1);
	}

	LocalPropertiesBlender.BlendRangeOnce(*StartPoint.Point, *EndPoint.Point, SubPoints, Alphas);
	InBlender->BlendRangeOnce(StartPoint.Index, EndPoint.Index, StartPoint.Index, NumPoints, Alphas);

	// Restore pre-blend position
	for (int i = 0; i < NumPoints; i++) { SubPoints[i].Transform.SetLocation(Locations[i]); }
}