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

void UOGFutureBP::FulfillPromiseObject(const FOGPromise& InPromise, UObject* Value)
{
	const TWeakObjectPtr<UObject> WeakObject = Value;
	FulfillPromise(InPromise, WeakObject);
}

void UOGFutureBP::ThenObject(const FOGFuture& InFuture, UObject*& OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TWeakObjectPtr<UObject>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TWeakObjectPtr<UObject>& Result) mutable
	{
		OutValue = Result.Get();
		ExecuteLatentAction(LatentInfo);
	});
}

void UOGFutureBP::FulfillPromiseObjects(const FOGPromise& InPromise, const TArray<UObject*>& Value)
{
	TArray<TWeakObjectPtr<UObject>> WeakObjArray;
	for (UObject* Obj : Value)
	{
		WeakObjArray.Add(Obj);
	}
	FulfillPromise<TArray<TWeakObjectPtr<UObject>>>(InPromise, WeakObjArray);
}

void UOGFutureBP::ThenObjects(const FOGFuture& InFuture, TArray<UObject*>& OutValue, FLatentActionInfo LatentInfo)
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

UE_DISABLE_OPTIMIZATION
void UOGFutureBP::FulfillPromiseActor(const FOGPromise& InPromise, AActor* Value)
{
	const TWeakObjectPtr<AActor> WeakActor = Value;
	FulfillPromise(InPromise, WeakActor);
}


void UOGFutureBP::ThenActor(const FOGFuture& InFuture, AActor*& OutValue, FLatentActionInfo LatentInfo)
{
	const TOGFuture<TWeakObjectPtr<AActor>> Future = InFuture;
	Future->WeakThen(LatentInfo.CallbackTarget.Get(), [LatentInfo, &OutValue](const TWeakObjectPtr<AActor>& Result) mutable
	{
		OutValue = Result.Get();
		ExecuteLatentAction(LatentInfo);
	});
}
UE_ENABLE_OPTIMIZATION

void UOGFutureBP::FulfillPromiseActors(const FOGPromise& InPromise, const TArray<AActor*>& Value)
{
	TArray<TWeakObjectPtr<AActor>> WeakObjArray;
	for (AActor* Obj : Value)
	{
		WeakObjArray.Add(Obj);
	}
	FulfillPromise<TArray<TWeakObjectPtr<AActor>>>(InPromise, WeakObjArray);
}

void UOGFutureBP::ThenActors(const FOGFuture& InFuture, TArray<AActor*>& OutValue, FLatentActionInfo LatentInfo)
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