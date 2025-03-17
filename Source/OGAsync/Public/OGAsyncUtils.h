/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OGAsyncUtils.generated.h"

/**
 * 
 */
USTRUCT()
struct OGASYNC_API FOGAsyncUtils
{
	GENERATED_BODY()

	static void ExecuteLatentAction(FLatentActionInfo& LatentInfo);
};
