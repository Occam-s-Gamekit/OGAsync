/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.h"
#include "OGFutureBP.generated.h"

/**
 * Type-agnostic utility functions for handling promises and futures in blueprint
 */
UCLASS()
class OGASYNC_API UOGFutureBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, meta=(DisplayName = "To Future", CompactNodeTitle = "->",BlueprintAutocast))
	static FOGFuture ConvertToFuture(FOGPromise Promise)
	{
		return Promise;
	}
};

/*
public: 
UFUNCTION(BlueprintPure, Category="OGAsync|Promise") 
static FOGPromise MakePromiseInt(){ return TOGPromise<int>(); } 

UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") 
static void FulfillPromiseInt(FOGPromise InPromise, int Value) { InPromise.Fulfill<int>(Value); } 

UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) 
static void ThenInt(FOGFuture InFuture, int& Value, FLatentActionInfo LatentInfo) 
{ 
	BindToLambda<int>(InFuture, Value, LatentInfo); 
}
*/

#define DEFINE_BP_FUTURE_LIBRARY(RESULT_TYPE_NAME, BP_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME(){ return TOGPromise<BP_TYPE>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, BP_TYPE Value) { InPromise.Fulfill<BP_TYPE>(Value); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void Then##RESULT_TYPE_NAME(FOGFuture InFuture, BP_TYPE& Value, FLatentActionInfo LatentInfo) \
	{ \
		BindToLambda<BP_TYPE>(InFuture, Value, LatentInfo); \
	}

#define DEFINE_BP_FUTURE_LIBRARY_OBJECT(RESULT_TYPE_NAME, OBJECT_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME(){ return TOGPromise<TWeakObjectPtr<OBJECT_TYPE>>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, OBJECT_TYPE* Value) { InPromise.Fulfill<TWeakObjectPtr<OBJECT_TYPE>>(Value); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void Then##RESULT_TYPE_NAME(FOGFuture InFuture, OBJECT_TYPE*& Value, FLatentActionInfo LatentInfo) \
	{ \
		const TOGFuture<TWeakObjectPtr<OBJECT_TYPE>> Future = InFuture; \
		Future.WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &Value](const TWeakObjectPtr<OBJECT_TYPE>& Result) mutable \
		{ \
			Value = Result.Get(); \
			ExecuteLatentAction(LatentInfo); \
		}); \
	}

#define DEFINE_BP_FUTURE_LIBRARY_OBJECT_ARRAY(RESULT_TYPE_NAME, OBJECT_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME(){ return TOGPromise<TArray<TWeakObjectPtr<OBJECT_TYPE>>>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, TArray<OBJECT_TYPE*> Value) \
	{ \
		TArray<TWeakObjectPtr<OBJECT_TYPE>> WeakObjArray; \
		for (OBJECT_TYPE* Obj : Value) \
		{ \
			WeakObjArray.Add(Obj); \
		} \
		InPromise.Fulfill<TArray<TWeakObjectPtr<OBJECT_TYPE>>>(WeakObjArray); \
	} \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void ThenObject(FOGFuture InFuture, TArray<OBJECT_TYPE*>& Value, FLatentActionInfo LatentInfo) \
	{ \
		const TOGFuture<TArray<TWeakObjectPtr<OBJECT_TYPE>>> Future = InFuture; \
		Future.WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &Value](const TArray<TWeakObjectPtr<OBJECT_TYPE>>& Result) mutable \
		{ \
			for (TWeakObjectPtr<OBJECT_TYPE> WeakObj : Result) \
			{ \
				Value.Add(WeakObj.Get()); \
			} \
			ExecuteLatentAction(LatentInfo); \
		}); \
	}

/**
 * Non-typed helper functions for the specific type implementations will use.
 */
UCLASS()
class UOGFutureBP_Typed : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

protected:
	static void ExecuteLatentAction(FLatentActionInfo& LatentInfo);
	
	template<typename T>
	static void BindToLambda(FOGFuture InFuture, T& OutValue, FLatentActionInfo LatentInfo)
	{
		const TOGFuture<T> Future = InFuture;
		Future.WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const T& Result) mutable
		{
			OutValue = Result;
			ExecuteLatentAction(LatentInfo);
		});
	}

public:
	DEFINE_BP_FUTURE_LIBRARY(Bool, bool);
	DEFINE_BP_FUTURE_LIBRARY(Bools, TArray<bool>);
	DEFINE_BP_FUTURE_LIBRARY(Int, int);
	DEFINE_BP_FUTURE_LIBRARY(Ints, TArray<int>);
	DEFINE_BP_FUTURE_LIBRARY(Float, float);
	DEFINE_BP_FUTURE_LIBRARY(Floats, TArray<float>);
	DEFINE_BP_FUTURE_LIBRARY(Vector, FVector);
	DEFINE_BP_FUTURE_LIBRARY(Vectors, TArray<FVector>);
	DEFINE_BP_FUTURE_LIBRARY(String, FString);
	DEFINE_BP_FUTURE_LIBRARY(Strings, TArray<FString>);
	DEFINE_BP_FUTURE_LIBRARY(Name, FName);
	DEFINE_BP_FUTURE_LIBRARY(Names, TArray<FName>);
	DEFINE_BP_FUTURE_LIBRARY_OBJECT(Object, UObject);
	DEFINE_BP_FUTURE_LIBRARY_OBJECT_ARRAY(Objects, UObject);
	DEFINE_BP_FUTURE_LIBRARY_OBJECT(Actor, AActor);
	DEFINE_BP_FUTURE_LIBRARY_OBJECT_ARRAY(Actors, AActor);
};
