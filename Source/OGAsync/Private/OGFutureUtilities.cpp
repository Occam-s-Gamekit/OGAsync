// Fill out your copyright notice in the Description page of Project Settings.


#include "OGFutureUtilities.h"

FOGFuture UOGFutureUtilities::FutureAll(const UObject* Context, TArray<FOGFuture> WaitForAll)
{
	TSharedRef<TOGFutureState<void>> FutureStateAll;
	int WaitingForN = WaitForAll.Num();
	bool ErrorReceived = false;
	const TFunction<void()> Lambda = [FutureStateAll, &WaitingForN]() mutable 
	{
		WaitingForN--;
		if (WaitingForN == 0)
		{
			FutureStateAll->Fulfill();
		}
	};
	const TFunction<void(const FString&)> CatchLambda = [FutureStateAll, &ErrorReceived](const FString& Reason)
	{
		if (!ErrorReceived)
		{
			ErrorReceived = true;
			FutureStateAll->Throw(Reason);
		}
	};
	for (const FOGFuture& InnerFuture : WaitForAll)
    {
    	(void)InnerFuture->WeakThen(Context, Lambda, CatchLambda);
    }
	return TOGFuture<void>(FutureStateAll);
}

FOGFuture UOGFutureUtilities::FutureAny(const UObject* Context, TArray<FOGFuture> WaitForFirst)
{
	TSharedRef<TOGFutureState<void>> FutureStateAny;
	bool Complete = false;
	int WaitingForNErrors = WaitForFirst.Num();
	const TFunction<void()> Lambda = [FutureStateAny, &Complete]() mutable 
	{
		if (!Complete)
		{
			Complete = true;
			FutureStateAny->Fulfill();
		}
	};
	const TFunction<void(const FString&)> CatchLambda = [FutureStateAny, &WaitingForNErrors](const FString& Reason) mutable 
	{
		WaitingForNErrors--;
		if (WaitingForNErrors == 0)
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
