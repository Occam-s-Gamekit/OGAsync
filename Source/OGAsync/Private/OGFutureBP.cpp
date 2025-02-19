/// Copyright Occam's Gamekit contributors 2025


#include "OGFutureBP.h"

void UOGFutureBP::ExecuteLatentAction(FLatentActionInfo& LatentInfo)
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

void UOGFutureBP::ThenObject(FOGFuture InFuture, UObject* OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TWeakObjectPtr<UObject>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TWeakObjectPtr<UObject>& Result) mutable
	{
		OutValue = Result.Get();
		ExecuteLatentAction(LatentInfo);
	});
}

void UOGFutureBP::FulfillPromiseObjects(FOGPromise InPromise, const TArray<UObject*>& Value)
{
	TArray<TWeakObjectPtr<UObject>> WeakObjArray;
	for (UObject* Obj : Value)
	{
		WeakObjArray.Add(Obj);
	}
	FulfillPromise<TArray<TWeakObjectPtr<UObject>>>(InPromise, WeakObjArray);
}

void UOGFutureBP::ThenObjects(FOGFuture InFuture, TArray<UObject*>& OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TArray<TWeakObjectPtr<UObject>>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TArray<TWeakObjectPtr<UObject>>& Result) mutable
	{
		for (TWeakObjectPtr<UObject> WeakObj : Result)
		{
			OutValue.Add(WeakObj.Get());
		}
		ExecuteLatentAction(LatentInfo);
	});
}

void UOGFutureBP::ThenActor(FOGFuture InFuture, AActor* OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TWeakObjectPtr<AActor>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TWeakObjectPtr<AActor>& Result) mutable
	{
		OutValue = Result.Get();
		ExecuteLatentAction(LatentInfo);
	});
}

void UOGFutureBP::FulfillPromiseActors(FOGPromise InPromise, const TArray<AActor*>& Value)
{
	TArray<TWeakObjectPtr<AActor>> WeakObjArray;
	for (AActor* Obj : Value)
	{
		WeakObjArray.Add(Obj);
	}
	FulfillPromise<TArray<TWeakObjectPtr<AActor>>>(InPromise, WeakObjArray);
}

void UOGFutureBP::ThenActors(FOGFuture InFuture, TArray<AActor*>& OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TArray<TWeakObjectPtr<AActor>>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TArray<TWeakObjectPtr<AActor>>& Result) mutable
	{
		for (TWeakObjectPtr<AActor> WeakObj : Result)
		{
			OutValue.Add(WeakObj.Get());
		}
		ExecuteLatentAction(LatentInfo);
	});
}
