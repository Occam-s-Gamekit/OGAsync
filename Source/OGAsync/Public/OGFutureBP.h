/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.h"
#include "OGAsyncUtils.h"
#include "OGFutureBP.generated.h"

/**
 * Typed helper functions for blueprint to use promises/futures
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
	
	template<typename T>
	static void BindToLambda(FOGFuture InFuture, T& OutValue, FLatentActionInfo LatentInfo)
	{
		const TOGFuture<T> Future = InFuture;
		if (!Future.IsValid())
			return;
		Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const T& Result) mutable
		{
			OutValue = Result;
			FOGAsyncUtils::ExecuteLatentAction(LatentInfo);
		});
	}

	static void BindToLambda(FOGFuture InFuture, FLatentActionInfo LatentInfo)
	{
		const TOGFuture<void> Future = InFuture;
		if (!Future.IsValid())
			return;
		Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo]() mutable
		{
			FOGAsyncUtils::ExecuteLatentAction(LatentInfo);
		});
	}

public:
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseVoid()	{ return MakePromise<void>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseVoid(const FOGPromise& InPromise) { FulfillPromise(InPromise); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenVoid(const FOGFuture& InFuture, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseBool()	{ return MakePromise<bool>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseBool(const FOGPromise& InPromise, bool Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenBool(const FOGFuture& InFuture, bool& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }
	
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseBools()	{ return MakePromise<TArray<bool>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseBools(const FOGPromise& InPromise, const TArray<bool>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenBools(const FOGFuture& InFuture, TArray<bool>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }
	
	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseInt()	{ return MakePromise<int>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseInt(const FOGPromise& InPromise, int Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenInt(const FOGFuture& InFuture, int& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseInts()	{ return MakePromise<TArray<int>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseInts(const FOGPromise& InPromise, const TArray<int>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenInts(const FOGFuture& InFuture, TArray<int>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseFloat()	{ return MakePromise<float>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseFloat(const FOGPromise& InPromise, float Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenFloat(const FOGFuture& InFuture, float& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseFloats()	{ return MakePromise<TArray<float>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseFloats(const FOGPromise& InPromise, const TArray<float>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenFloats(const FOGFuture& InFuture, TArray<float>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseVector()	{ return MakePromise<FVector>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseVector(const FOGPromise& InPromise, FVector Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenVector(const FOGFuture& InFuture, FVector& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseVectors()	{ return MakePromise<TArray<FVector>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseVectors(const FOGPromise& InPromise, const TArray<FVector>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenVectors(const FOGFuture& InFuture, TArray<FVector>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseString()	{ return MakePromise<FString>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseString(const FOGPromise& InPromise, FString Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenString(const FOGFuture& InFuture, FString& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseStrings()	{ return MakePromise<TArray<FString>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseStrings(const FOGPromise& InPromise, const TArray<FString>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenStrings(const FOGFuture& InFuture, TArray<FString>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseName()	{ return MakePromise<FName>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseName(const FOGPromise& InPromise, FName Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenName(const FOGFuture& InFuture, FName& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseNames()	{ return MakePromise<TArray<FName>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseNames(const FOGPromise& InPromise, const TArray<FName>& Value) { FulfillPromise(InPromise, Value); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenNames(const FOGFuture& InFuture, TArray<FName>& OutValue, FLatentActionInfo LatentInfo) { BindToLambda(InFuture, OutValue, LatentInfo); }

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseObject()	{ return MakePromise<TWeakObjectPtr<UObject>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseObject(const FOGPromise& InPromise, UObject* Value);
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenObject(const FOGFuture& InFuture, UObject*& OutValue, FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseObjects()	{ return MakePromise<TArray<TWeakObjectPtr<UObject>>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseObjects(const FOGPromise& InPromise, const TArray<UObject*>& Value);
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenObjects(const FOGFuture& InFuture, TArray<UObject*>& OutValue, FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseActor()	{ return MakePromise<TWeakObjectPtr<AActor>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseActor(const FOGPromise& InPromise, AActor* Value);
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenActor(const FOGFuture& InFuture, AActor*& OutValue, FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintPure, Category="OGAsync|Promise")
	static FOGPromise MakePromiseActors()	{ return MakePromise<TArray<TWeakObjectPtr<AActor>>>(); }
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Promise")
	static void FulfillPromiseActors(const FOGPromise& InPromise, const TArray<AActor*>& Value);
	
	UFUNCTION(BlueprintCallable, Category="OGAsync|Future", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenActors(const FOGFuture& InFuture, TArray<AActor*>& OutValue, FLatentActionInfo LatentInfo);
};