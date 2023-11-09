﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "..\PCGExCommon.h"

#define PCGEX_COMPARE_1(FIELD) \
Result = Compare(A.FIELD, B.FIELD, Tolerance, Comp); \
if(Result != 0){return Result;}
#define PCGEX_COMPARE_2(FIELD_A, FIELD_B) \
PCGEX_COMPARE_1(FIELD_A) \
PCGEX_COMPARE_1(FIELD_B)
#define PCGEX_COMPARE_3(FIELD_A, FIELD_B, FIELD_C) \
PCGEX_COMPARE_2(FIELD_A, FIELD_B) \
PCGEX_COMPARE_1(FIELD_C)

class PCGEXTENDEDTOOLKIT_API FPCGExCompare
{
public:
	template <typename T, typename dummy = void>
	static int Compare(const T& A, const T& B, const double Tolerance = 0.0001f, EPCGExComponentSelection Comp = EPCGExComponentSelection::X)
	{
		return FMath::IsNearlyEqual(static_cast<double>(A), static_cast<double>(B), Tolerance) ? 0 : A < B ? -1 : 1;
	}

	/*
	template <typename dummy = void>
	static int Compare(const int32& A, const int32& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return A == B ? 0 : A < B ? -1 : 1;
	}

	template <typename dummy = void>
	static int Compare(const int64& A, const int64& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return A == B ? 0 : A < B ? -1 : 1;
	}
	*/

	template <typename dummy = void>
	static int Compare(const FVector2D& A, const FVector2D& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		int Result = 0;
		switch (Comp)
		{
		case EPCGExComponentSelection::X:
			PCGEX_COMPARE_1(X)
			break;
		case EPCGExComponentSelection::Y:
		case EPCGExComponentSelection::Z:
		case EPCGExComponentSelection::W:
			PCGEX_COMPARE_1(Y)
			break;
		case EPCGExComponentSelection::XYZ:
		case EPCGExComponentSelection::XZY:
		case EPCGExComponentSelection::ZXY:
			PCGEX_COMPARE_2(X, Y)
			break;
		case EPCGExComponentSelection::YXZ:
		case EPCGExComponentSelection::YZX:
		case EPCGExComponentSelection::ZYX:
			PCGEX_COMPARE_2(Y, X)
			break;
		case EPCGExComponentSelection::Length:
			PCGEX_COMPARE_1(SquaredLength())
			break;
		default: ;
		}
		return Result;
	}

	template <typename dummy = void>
	static int Compare(const FVector& A, const FVector& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		int Result = 0;
		switch (Comp)
		{
		case EPCGExComponentSelection::X:
			PCGEX_COMPARE_1(X)
			break;
		case EPCGExComponentSelection::Y:
			PCGEX_COMPARE_1(Y)
			break;
		case EPCGExComponentSelection::Z:
		case EPCGExComponentSelection::W:
			PCGEX_COMPARE_1(Z)
			break;
		case EPCGExComponentSelection::XYZ:
			PCGEX_COMPARE_3(X, Y, Z)
			break;
		case EPCGExComponentSelection::XZY:
			PCGEX_COMPARE_3(X, Z, Y)
			break;
		case EPCGExComponentSelection::YXZ:
			PCGEX_COMPARE_3(Y, X, Z)
			break;
		case EPCGExComponentSelection::YZX:
			PCGEX_COMPARE_3(Y, Z, X)
			break;
		case EPCGExComponentSelection::ZXY:
			PCGEX_COMPARE_3(Z, X, Y)
			break;
		case EPCGExComponentSelection::ZYX:
			PCGEX_COMPARE_3(Z, Y, X)
			break;
		case EPCGExComponentSelection::Length:
			PCGEX_COMPARE_1(SquaredLength())
			break;
		default: ;
		}
		return Result;
	}

	template <typename dummy = void>
	static int Compare(const FVector4& A, const FVector4& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		if (Comp == EPCGExComponentSelection::W) { return Compare(A.W, B.W, Tolerance, Comp); }
		return Compare(FVector{A}, FVector{B}, Tolerance, Comp);
	}

	template <typename dummy = void>
	static int Compare(const FRotator& A, const FRotator& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return Compare(FVector{A.Euler()}, FVector{B.Euler()}, Tolerance, Comp);
	}

	template <typename dummy = void>
	static int Compare(const FQuat& A, const FQuat& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return Compare(FVector{A.Euler()}, FVector{B.Euler()}, Tolerance, Comp);
	}

	template <typename dummy = void>
	static int Compare(const FString& A, const FString& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return A < B ? -1 : A == B ? 0 : 1;
	}

	template <typename dummy = void>
	static int Compare(const FName& A, const FName& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return Compare(A.ToString(), B.ToString(), Tolerance, Comp);
	}

	template <typename dummy = void>
	static int Compare(const FTransform& A, const FTransform& B, const double Tolerance, EPCGExComponentSelection Comp)
	{
		return Compare(A.GetLocation(), B.GetLocation(), Tolerance, Comp);
	}
};

#undef PCGEX_COMPARE_1
#undef PCGEX_COMPARE_2
#undef PCGEX_COMPARE_3
