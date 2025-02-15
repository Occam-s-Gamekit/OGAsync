/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.h"
#include "OGFutureBP.generated.h"

/**
 * 
 */

UCLASS()
class OGASYNC_API UOGFutureBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static void ExecuteLatentAction(FLatentActionInfo& LatentInfo);
	
	UFUNCTION(BlueprintPure, meta=(DisplayName = "To Future", CompactNodeTitle = "->",BlueprintAutocast))
	static FOGFuture ConvertToFuture(FOGPromise Promise)
	{
		return Promise;
	}
};

UCLASS()
class OGASYNC_API UOGFutureBP_Int : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="SimpleAsync|Promise")
	static FOGPromise MakePromiseInt(){ return TOGPromise<int>(); }

	UFUNCTION(BlueprintCallable, Category="SimpleAsync|Promise")
	static void FulfillPromiseInt(FOGPromise InPromise, int Value) { InPromise.Fulfill<int>(Value); }

	UFUNCTION(BlueprintCallable, Category="SimpleAsync|Promise", meta=(Latent, LatentInfo="LatentInfo"))
	static void ThenInt(FOGFuture InFuture, int& Value, FLatentActionInfo LatentInfo)
	{
		const TOGFuture<int> Future = InFuture;
		Future.WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &Value](const int& Result) mutable
		{
			Value = Result;
			UOGFutureBP::ExecuteLatentAction(LatentInfo);
		});
	}
};

