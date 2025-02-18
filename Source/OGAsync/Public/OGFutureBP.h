/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.h"
#include "OGFutureBP.generated.h"

#define DEFINE_BP_FUTURE_LIBRARY(RESULT_TYPE_NAME, BP_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME() \
		{ return UOGFutureBP::MakePromise<BP_TYPE>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, BP_TYPE Value) \
		{ UOGFutureBP::FulfillPromise(InPromise, Value); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void Then##RESULT_TYPE_NAME(FOGFuture InFuture, BP_TYPE& Value, FLatentActionInfo LatentInfo) \
		{ BindToLambda<BP_TYPE>(InFuture, Value, LatentInfo); }

#define DEFINE_BP_FUTURE_LIBRARY_OBJECT(RESULT_TYPE_NAME, OBJECT_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME() \
		{ return UOGFutureBP::MakePromise<TWeakObjectPtr<OBJECT_TYPE>>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, OBJECT_TYPE* Value) \
		{ UOGFutureBP::FulfillPromise<TWeakObjectPtr<OBJECT_TYPE>>(InPromise, Value); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void Then##RESULT_TYPE_NAME(FOGFuture InFuture, OBJECT_TYPE*& Value, FLatentActionInfo LatentInfo) \
	{ \
		const TOGFuture<TWeakObjectPtr<OBJECT_TYPE>> Future = InFuture; \
		Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &Value](const TWeakObjectPtr<OBJECT_TYPE>& Result) mutable \
		{ \
			Value = Result.Get(); \
			ExecuteLatentAction(LatentInfo); \
		}); \
	}

#define DEFINE_BP_FUTURE_LIBRARY_OBJECT_ARRAY(RESULT_TYPE_NAME, OBJECT_TYPE) \
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise") \
	static FOGPromise MakePromise##RESULT_TYPE_NAME() \
		{ return UOGFutureBP::MakePromise<TArray<TWeakObjectPtr<OBJECT_TYPE>>>(); } \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise") \
	static void FulfillPromise##RESULT_TYPE_NAME(FOGPromise InPromise, TArray<OBJECT_TYPE*> Value) \
	{ \
		TArray<TWeakObjectPtr<OBJECT_TYPE>> WeakObjArray; \
		for (OBJECT_TYPE* Obj : Value) \
		{ \
			WeakObjArray.Add(Obj); \
		} \
		UOGFutureBP::FulfillPromise<TArray<TWeakObjectPtr<OBJECT_TYPE>>>(InPromise, WeakObjArray); \
	} \
	\
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo")) \
	static void ThenObject(FOGFuture InFuture, TArray<OBJECT_TYPE*>& Value, FLatentActionInfo LatentInfo) \
	{ \
		const TOGFuture<TArray<TWeakObjectPtr<OBJECT_TYPE>>> Future = InFuture; \
		Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &Value](const TArray<TWeakObjectPtr<OBJECT_TYPE>>& Result) mutable \
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
class UOGFutureBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta=(DisplayName = "To Future", CompactNodeTitle = "->",BlueprintAutocast))
	static FOGFuture ConvertToFuture(FOGPromise Promise)
	{
		return Promise;
	}

	/**
	 * This creates a FOGPromise that has the some underlying data as a TOGPromise<T>.
	 * This is only intended for Blueprint as it strips away some of the protections in TOGPromise<T>
	 * @tparam T The type of value that the promise will store
	 * @return A FOGPromise
	 */
	template<typename T>
	static FOGPromise MakePromise()
	{
		return FOGPromise(MakeShared<TOGFutureState<T>>());
	}

	template<typename T>
	static void FulfillPromise(const FOGPromise& InPromise, const T& Value)
	{
		TOGFutureState<T>* Inner = InPromise.GetTypedState<T>();
		if (!Inner)
			return;
		Inner->Fulfill(Value);
	}
	
	static void FulfillPromise(const FOGPromise& InPromise)
	{
		TOGFutureState<void>* Inner = InPromise.GetTypedState<void>();
		if (!Inner)
			return;
		Inner->Fulfill();
	}
	
	static void ExecuteLatentAction(FLatentActionInfo& LatentInfo);
	
	template<typename T>
	static void BindToLambda(FOGFuture InFuture, T& OutValue, FLatentActionInfo LatentInfo)
	{
		const TOGFuture<T> Future = InFuture;
		if (!Future.IsValid())
			return;
		Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const T& Result) mutable
		{
			OutValue = Result;
			ExecuteLatentAction(LatentInfo);
		});
	}

public:
	//Typed promise functions
	
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
