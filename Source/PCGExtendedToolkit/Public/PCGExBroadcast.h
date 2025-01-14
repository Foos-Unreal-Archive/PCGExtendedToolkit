﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once
#include "PCGEx.h"
#include "PCGExMath.h"

namespace PCGEx
{
#pragma region Conversions

#pragma region Broadcast from bool

	template <typename T>
	FORCEINLINE static T Broadcast(const bool Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value ? 1 : 0; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value ? 1 : 0); }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value ? 1 : 0); }
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			const double D = Value ? 1 : 0;
			return FVector4(D, D, D, D);
		}
		else if constexpr (std::is_same_v<T, FQuat>)
		{
			const double D = Value ? 180 : 0;
			return FRotator(D, D, D).Quaternion();
		}
		else if constexpr (std::is_same_v<T, FRotator>)
		{
			const double D = Value ? 180 : 0;
			return FRotator(D, D, D);
		}
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return FString::Printf(TEXT("%s"), Value ? TEXT("true") : TEXT("false")); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(FString::Printf(TEXT("%s"), Value ? TEXT("true") : TEXT("false"))); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from Integer32

	template <typename T>
	FORCEINLINE static T Broadcast(const int32 Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value); }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value, Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value, Value, Value).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return FString::Printf(TEXT("%d"), Value); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(FString::Printf(TEXT("%d"), Value)); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from Integer64

	template <typename T>
	FORCEINLINE static T Broadcast(const int64 Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value); }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value, Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value, Value, Value).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return FString::Printf(TEXT("%lld"), Value); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(FString::Printf(TEXT("(%lld)"), Value)); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from Float

	template <typename T>
	FORCEINLINE static T Broadcast(const float Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value); }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value, Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value, Value, Value).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return FString::Printf(TEXT("%f"), Value); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(FString::Printf(TEXT("(%f)"), Value)); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from Double

	template <typename T>
	FORCEINLINE static T Broadcast(const double Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value); }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value, Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value, Value, Value).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value, Value, Value); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return FString::Printf(TEXT("%f"), Value); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(FString::Printf(TEXT("(%f)"), Value)); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FVector2D

	template <typename T>
	FORCEINLINE static T Broadcast(const FVector2D& Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value.X > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value.X; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector>) { return FVector(Value.X, Value.Y, 0); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value.X, Value.Y, 0, 0); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value.X, Value.Y, 0).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value.X, Value.Y, 0); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FVector

	template <typename T>
	FORCEINLINE static T Broadcast(const FVector& Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value.X > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value.X; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value.X, Value.Y); }
		else if constexpr (std::is_same_v<T, FVector>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value.X, Value.Y, Value.Z, 0); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value.X, Value.Y, Value.Z).Quaternion(); /* TODO : Handle axis selection */ }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value.X, Value.Y, Value.Z); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; /* TODO : Handle axis selection */ }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FVector4

	template <typename T>
	FORCEINLINE static T Broadcast(const FVector4& Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value.X > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value.X > 0; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return FVector2D(Value.X, Value.Y); }
		else if constexpr (std::is_same_v<T, FVector>) { return Value; }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value.X, Value.Y, Value.Z, Value.W); }
		else if constexpr (std::is_same_v<T, FQuat>) { return FRotator(Value.X, Value.Y, Value.Z).Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return FRotator(Value.X, Value.Y, Value.Z); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform::Identity; /* TODO : Handle axis */ }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FQuat

	template <typename T>
	FORCEINLINE static T Broadcast(const FQuat& Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return PCGExMath::GetDirection(Value, EPCGExAxis::Forward).X > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return PCGExMath::GetDirection(Value, EPCGExAxis::Forward).X; }
		else if constexpr (std::is_same_v<T, FVector2D>)
		{
			const FVector Dir = PCGExMath::GetDirection(Value, EPCGExAxis::Forward);
			return FVector2D(Dir.X, Dir.Y);
		}
		else if constexpr (std::is_same_v<T, FVector>) { return PCGExMath::GetDirection(Value, EPCGExAxis::Forward); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(PCGExMath::GetDirection(Value, EPCGExAxis::Forward), 0); }
		else if constexpr (std::is_same_v<T, FQuat>) { return Value; }
		else if constexpr (std::is_same_v<T, FRotator>) { return Value.Rotator(); }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform(Value, FVector::ZeroVector, FVector::OneVector); }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FRotator

	template <typename T>
	FORCEINLINE static T Broadcast(const FRotator& Value)
	{
		if constexpr (std::is_same_v<T, bool>) { return Value.Pitch > 0; }
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_same_v<T, float> || std::is_same_v<T, double>) { return Value.Pitch > 0; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return Broadcast<T>(Value.Quaternion()); }
		else if constexpr (std::is_same_v<T, FVector>) { return Broadcast<T>(Value.Quaternion()); }
		else if constexpr (std::is_same_v<T, FVector4>) { return FVector4(Value.Euler(), 0); /* TODO : Handle axis */ }
		else if constexpr (std::is_same_v<T, FQuat>) { return Value.Quaternion(); }
		else if constexpr (std::is_same_v<T, FRotator>) { return Value; }
		else if constexpr (std::is_same_v<T, FTransform>) { return FTransform(Value.Quaternion(), FVector::ZeroVector, FVector::OneVector); }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FTransform

	template <typename T>
	FORCEINLINE static T Broadcast(const FTransform& Value)
	{
		if constexpr (
			std::is_same_v<T, bool> ||
			std::is_same_v<T, int32> ||
			std::is_same_v<T, int64> ||
			std::is_same_v<T, float> ||
			std::is_same_v<T, double> ||
			std::is_same_v<T, FVector2D> ||
			std::is_same_v<T, FVector> ||
			std::is_same_v<T, FVector4> ||
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return Broadcast<T>(Value.GetLocation());
		}
		else if constexpr (std::is_same_v<T, FTransform>) { return Value; }
		else if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FString

	template <typename T>
	FORCEINLINE static T Broadcast(const FString& Value)
	{
		if constexpr (std::is_same_v<T, FString>) { return Value; }
		else if constexpr (std::is_same_v<T, FName>) { return FName(Value); }
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { return FSoftClassPath(Value); }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { return FSoftObjectPath(Value); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FName

	template <typename T>
	FORCEINLINE static T Broadcast(const FName& Value)
	{
		if constexpr (std::is_same_v<T, FString>) { return Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return Value; }
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { return FSoftClassPath(Value.ToString()); }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { return FSoftObjectPath(Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FSoftClassPath

	template <typename T>
	FORCEINLINE static T Broadcast(const FSoftClassPath& Value)
	{
		if constexpr (std::is_same_v<T, FString>) { return Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(Value.ToString()); }
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { return Value; }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { return FSoftObjectPath(Value.ToString()); }
		else { return T{}; }
	}

#pragma endregion

#pragma region Broadcast from FSoftObjectPath

	template <typename T>
	FORCEINLINE static T Broadcast(const FSoftObjectPath& Value)
	{
		if constexpr (std::is_same_v<T, FString>) { return *Value.ToString(); }
		else if constexpr (std::is_same_v<T, FName>) { return FName(*Value.ToString()); }
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { return FSoftClassPath(Value.ToString()); }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { return Value; }
		else { return T{}; }
	}

#pragma endregion

#pragma endregion
}
