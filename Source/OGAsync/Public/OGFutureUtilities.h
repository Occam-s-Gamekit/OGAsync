/// Copyright Occam's Gamekit contributors 2025

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

public:
	UFUNCTION(Blueprintcallable, Category="OGAsync|Utility")
	static FOGFuture FutureAll(const UObject* Context, TArray<FOGFuture>& WaitForAll);

	UFUNCTION(Blueprintcallable, Category="OGAsync|Utility")
	static FOGFuture FutureAny(const UObject* Context, TArray<FOGFuture>& WaitForFirst);
};
