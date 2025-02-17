// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.h"
#include "OGFutureUtilities.generated.h"

/**
 * FutureAll takes an array of futures and returns a future that will complete when all of the futures have completed.
 * FutureAny takes an array of futures and returns a future that will complete when the first future completes.
 */

UCLASS()
class OGASYNC_API UOGFutureUtilities : public UObject
{
	GENERATED_BODY()

	UFUNCTION(Blueprintcallable, Category="OGAsync|Utility")
	FOGFuture FutureAll(const UObject* Context, TArray<FOGFuture> WaitForAll);

	UFUNCTION(Blueprintcallable, Category="OGAsync|Utility")
	FOGFuture FutureAny(const UObject* Context, TArray<FOGFuture> WaitForFirst);
};
