/// Copyright Occam's Gamekit contributors 2025


#include "OGFuture.h"

DEFINE_LOG_CATEGORY(LogOGFuture);

TArray<TSharedPtr<FOGFutureState>> ErrorStates;
const FOGFuture FOGFuture::EmptyFuture(nullptr);
const FOGPromise FOGPromise::EmptyPromise(nullptr);

FOGFutureState const* FOGFuture::operator->() const
{
	if (!SharedState.IsValid()) [[unlikely]]
			return TOGFutureState<void>::GetErrorState();
	return SharedState.Get();
}
