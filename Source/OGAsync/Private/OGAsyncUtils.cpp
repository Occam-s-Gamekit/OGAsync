/// Copyright Occam's Gamekit contributors 2025


#include "OGAsyncUtils.h"

void FOGAsyncUtils::ExecuteLatentAction(FLatentActionInfo& LatentInfo)
{
	if (LatentInfo.Linkage != INDEX_NONE)
	{
		if (UObject* CallbackTarget = LatentInfo.CallbackTarget.Get())
		{
			if (UFunction* ExecutionFunction = CallbackTarget->FindFunction(LatentInfo.ExecutionFunction))
			{
				CallbackTarget->ProcessEvent(ExecutionFunction, &(LatentInfo.Linkage));
			}
		}
	}
}