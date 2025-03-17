/// Copyright Occam's Gamekit contributors 2025


#include "OGFutureUtilities.h"

FOGFuture UOGFutureUtilities::FutureAll(const UObject* Context, TArray<FOGFuture>& WaitForAll)
{
	TSharedRef<TOGFutureState<void>> FutureStateAll = MakeShared<TOGFutureState<void>>();
	if (WaitForAll.IsEmpty())
	{
		FutureStateAll->Fulfill();
		return TOGFuture<void>(FutureStateAll);
	}
	//Key = Num promises left to complete, Value = HasErrored
	TSharedRef<TPair<int,bool>> SharedData = MakeShared<TPair<int,bool>>();
	SharedData->Key = WaitForAll.Num();
	SharedData->Value = false;
	const TFunction<void()> Lambda = [FutureStateAll, SharedData]() mutable 
	{
		SharedData->Key--;
		if (SharedData->Key == 0)
		{
			FutureStateAll->Fulfill();
		}
	};
	const TFunction<void(const FString&)> CatchLambda = [FutureStateAll, SharedData](const FString& Reason)
	{
		if (!SharedData->Value)
		{
			SharedData->Value = true;
			FutureStateAll->Throw(Reason);
		}
	};
	for (const FOGFuture& InnerFuture : WaitForAll)
    {
    	(void)InnerFuture->WeakThen(Context, Lambda, CatchLambda);
    }
	return TOGFuture<void>(FutureStateAll);
}

FOGFuture UOGFutureUtilities::FutureAny(const UObject* Context, TArray<FOGFuture>& WaitForFirst)
{
	TSharedRef<TOGFutureState<void>> FutureStateAny = MakeShared<TOGFutureState<void>>();
	if (WaitForFirst.IsEmpty())
	{
		FutureStateAny->Fulfill();
		return TOGFuture<void>(FutureStateAny);
	}
	//Key = Num promises left to fail, Value = HasCompleted
	TSharedRef<TPair<int,bool>> SharedData = MakeShared<TPair<int,bool>>();
	SharedData->Key = WaitForFirst.Num();
	SharedData->Value = false;
	const TFunction<void()> Lambda = [FutureStateAny, SharedData]() mutable 
	{
		if (!SharedData->Value)
		{
			SharedData->Value = true;
			FutureStateAny->Fulfill();
		}
	};
	const TFunction<void(const FString&)> CatchLambda = [FutureStateAny, SharedData](const FString& Reason) mutable 
	{
		SharedData->Key--;
		if (SharedData->Key == 0)
		{
			FutureStateAny->Throw(TEXT("All futures were thrown, can no longer complete"));
		}
	};
	for (const FOGFuture& InnerFuture : WaitForFirst)
	{
		(void)InnerFuture->WeakThen(Context, Lambda, CatchLambda);
	}
	return TOGFuture<void>(FutureStateAny);
}
