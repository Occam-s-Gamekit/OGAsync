/// Copyright Occam's Gamekit contributors 2025


#include "OGFuture.h"

const FOGFuture FOGFuture::EmptyFuture(nullptr);

void FOGFutureState::ExecuteCatchCallbacks()
{
	if (!ensure(FailureReason.IsSet() && State == EState::Rejected)) [[unlikely]]
			return;

	const FString& Reason = FailureReason.GetValue();
	for (FCatchDelegate& Catch : CatchCallbacks)
	{
		(void)Catch.ExecuteIfBound(Reason);
	}

	if (ContinuationFutureState.IsValid())
	{
		ContinuationFutureState.Get()->Throw(Reason);
	}
        
	ClearCallbacks();
}
