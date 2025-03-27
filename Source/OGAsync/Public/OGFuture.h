/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include <typeinfo>

#include "OGFuture.generated.h"

OGASYNC_API DECLARE_LOG_CATEGORY_EXTERN(LogOGFuture, Log, All);

struct FOGFutureState;
template<typename T>
struct TOGFutureState;
struct FOGFuture;
template<typename T>
struct TOGFuture;
struct FOGPromise;
template<typename T>
struct TOGPromise;

/**
 * "If you make a Promise, it's up to you to fulfill it.
 * If someone else makes you a promise, you must wait to see if they honor it in the Future."
 *
 * Futures are a convenient way to handle execution that needs to wait for some other process to finish
 * (and which may have already finished). When you have a Future, you can simply call WeakThen and pass in
 * a lambda function, and the lambda will be called when the underlying promise is fulfilled, or called
 * immediately if the promise is already fulfilled.
 * Futures cannot be created independently of promises, as they would never be completed.
 *
 * Promises are a commitment to fulfill when some process is complete. To ensure that the responsibility
 * is unambiguous, Promises cannot be copied or referenced, and if you move a Promise it will clear the
 * original. If you have a promise type, it is your responsibility to fulfill the promise. For convenience,
 * you can implicitly cast a Promise to a Future in order to pass the Future to another system or to wait for
 * the future to complete internally.
 * 
 * BasicUsage:
 *	class Foo
 *	{
 *	public:
 *		TOGFuture<int> GetFuture() {return MyPromise;}
 *
 *		void FulfillPromise(int Value) {MyPromise.Fulfill(Value);}
 * 
 *	protected:
 *		TOGPromise<int> MyPromise;
 *	};
 *
 *	class Bar : UObject
 *	{
 *		void Run(Foo* foo)
 *		{
 *			foo.GetFuture().WeakThen(this, [](const int& Result)
 *			{
 *				//Do something
 *			}
 *		}
 *	};
 * 
 * Internally, Promises and Futures are just wrappers around a shared pointer to a FutureState, which holds all
 * of the internal data and functionality that promises and futures rely on.
 */

extern OGASYNC_API TArray<TSharedPtr<FOGFutureState>> ErrorStates;

USTRUCT(BlueprintType)
struct OGASYNC_API FOGFuture
{
	GENERATED_BODY()
	FOGFuture() {}
	FOGFuture(const TSharedPtr<FOGFutureState>& FutureState) : SharedState(FutureState) {}

	static const FOGFuture EmptyFuture;

	FOGFutureState const* operator->() const;
	
	//convert a generic future into a specific type, will error if the type doesn't match the underlying data
	template<typename T>
	operator TOGFuture<T>();

	template<typename T>
	operator const TOGFuture<T>() const;

	bool IsValid() const
	{
		return SharedState.IsValid();
	}

protected:

	template<typename T>
	TOGFutureState<T>* GetTypedState() const;
	
	TSharedPtr<FOGFutureState> SharedState = nullptr;
};

template<typename T>
struct TOGFuture : FOGFuture
{
	friend struct TOGPromise<T>;
	typedef T Type;
	typedef typename TOGFutureState<T>::FThenDelegate FThenDelegate;

public:
	TOGFuture() : FOGFuture(nullptr) {}
	TOGFuture(const TSharedPtr<FOGFutureState>& FutureState) : FOGFuture(FutureState) {}

	TOGFutureState<T>* operator->() const { return GetTypedState<T>(); }
};

USTRUCT(BlueprintType)
struct OGASYNC_API FOGPromise
{
	friend class UOGFutureBP;
	
	GENERATED_BODY()
	FOGPromise() {}
	FOGPromise(const TSharedPtr<FOGFutureState>& State) : SharedState(State) {}

	static const FOGPromise EmptyPromise;

	operator FOGFuture() const
	{
		return FOGFuture(SharedState);
	}

	FOGFutureState* operator->() const { return SharedState.Get(); }
	template<typename T>
	TOGFutureState<T>* operator->() const { return GetTypedState<T>(); }

	bool IsValid() const
	{
		return SharedState.IsValid();
	}

	//convert a generic future into a specific type, will error if the type doesn't match the underlying data
	template<typename T>
	operator TOGPromise<T>();

	template<typename T>
	operator const TOGPromise<T>() const;

protected:
	
	template<typename T>
	TOGFutureState<T>* GetTypedState() const;
	
	//Mutable so the copy constructor can null the src
	mutable TSharedPtr<FOGFutureState> SharedState = nullptr;
};

template<typename T>
struct TOGPromise : FOGPromise
{
public:
	template<typename NoRawObjectPtr = T UE_REQUIRES(!std::is_convertible_v<T, UObject*>)>
	TOGPromise() : FOGPromise(MakeShared<TOGFutureState<T>>()) {}
	TOGPromise(const TSharedPtr<FOGFutureState>& FutureState) : FOGPromise(FutureState) {}
	~TOGPromise();
	
	//I would love to delete the copy operations to enforce only one promise per future state, but USTRUCT containing a promise will not compile if copy is deleted. 
	TOGPromise(const TOGPromise& Other) : FOGPromise(Other.SharedState)
	{
		UE_LOG(LogOGFuture, Warning, TEXT("Copying a promise is not advised, it is only supported because otherwise promises inside USTRUCTs won't compile."))
	}
	TOGPromise& operator=(const TOGPromise& Other)
	{
		UE_LOG(LogOGFuture, Warning, TEXT("Copying a promise is not advised, it is only supported because otherwise promises inside USTRUCTs won't compile."))
		SharedState = Other.SharedState;
		return *this;
	}
	
	//Promises can be moved, but it clears the Other promise
	TOGPromise(const TOGPromise&& Other) noexcept : FOGPromise(Other.SharedState)
	{
		Other.SharedState = nullptr;
	}
	TOGPromise& operator=(const TOGPromise&& Other) noexcept
	{
		if (this != &Other)
		{
			SharedState = Other.SharedState;
			Other.SharedState = nullptr;
		}
		return *this;
	}

	//implicit conversion to TOGFuture
	operator TOGFuture<T>()
	{
		return TOGFuture<T>(SharedState);
	}

	TOGFutureState<T>* operator->() const { return GetTypedState<T>(); }
};

struct OGASYNC_API FOGFutureState
{
	friend struct FOGFuture;
	friend struct FOGPromise;
	
	DECLARE_DELEGATE(FVoidThenDelegate);
	DECLARE_DELEGATE_OneParam(FCatchDelegate, const FString&);

	enum class EState
	{
		Pending,
		Fulfilled,
		Rejected
	};
	
	FOGFutureState(){}
	virtual ~FOGFutureState(){}

public:
	bool IsPending() const { return State == EState::Pending; }
	bool IsFulfilled() const { return State == EState::Fulfilled; }
	bool IsRejected() const { return State == EState::Rejected; }
	
	FOGFuture Then(const FVoidThenDelegate& Callback) const //intentionally hidden in children
	{
		return AddVoidThen(Callback);
	}

	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func>>)>
	FOGFuture WeakThen(const UObject* Context, Func&& Lambda) const
	{
		return AddVoidThen(FVoidThenDelegate::CreateWeakLambda(Context, Lambda));
	}

	template<typename ReturnsFuture UE_REQUIRES(std::is_convertible_v<TInvokeResult_T<ReturnsFuture>,FOGFuture>)>
	TOGFuture<void> WeakThen(const UObject* Context, ReturnsFuture&& AsyncLambda) const;
	
	void Throw(const FString& Reason)
	{
		if (!ensureAlways(State == EState::Pending) && !FailureReason.IsSet()) [[unlikely]]
			return;

		FailureReason.Emplace(Reason);
		State = EState::Rejected;
		ExecuteCatchCallbacks();
	}

	FOGFuture Catch(const FCatchDelegate& Callback) const //intentionally hidden in children
	{
		return AddCatch(Callback);
	}

	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func, const FString&>>)>
	FOGFuture WeakCatch(const UObject* Context, Func&& Lambda) const
	{
		return Catch(FCatchDelegate::CreateWeakLambda(Context, Lambda));
	}

	//convenience that allows you to bind then and catch in a single call
	template<typename ThenFunc, typename CatchFunc>
	FOGFuture WeakThen(const UObject* Context, ThenFunc&& ThenLambda, CatchFunc&& CatchLambda) const
	{
		WeakCatch(Context, CatchLambda);
		return WeakThen(Context, ThenLambda);
	}

	virtual void ExecuteThenCallbacks() = 0;
	void ExecuteCatchCallbacks()
	{
		//Value set but still pending will only happen while delegates are being called.
		if (!ensureAlways(State == EState::Rejected && FailureReason.IsSet())) [[unlikely]]
			return;

		const FString& Reason = FailureReason.GetValue();
		for (FCatchDelegate& Catch : CatchCallbacks)
		{
			(void)Catch.ExecuteIfBound(Reason);
		}

		if (ContinuationFutureState.IsValid())
		{
			ContinuationFutureState->Throw(Reason);
		}
		
		ClearCallbacks();
	}

	virtual void ClearCallbacks()
	{
        VoidThenCallbacks.Empty();
        CatchCallbacks.Empty();
	}
	
	virtual TSharedPtr<FOGFutureState> LazyGetContinuation() const = 0;
	
protected:

	virtual const std::type_info& GetInnerTypeInfo() const {return typeid(void); }

	FOGFuture AddVoidThen(const FVoidThenDelegate& Callback) const
	{
		switch (State)
		{
		case EState::Pending:
			VoidThenCallbacks.Add(Callback);
			break;
		case EState::Fulfilled:
			(void)Callback.ExecuteIfBound();
			break;
		default:
			//do nothing
				break;
		}

		return LazyGetContinuation();
	}

	FOGFuture AddCatch(const FCatchDelegate& Callback) const
	{
		switch (State)
		{
		case EState::Pending:
			CatchCallbacks.Add(Callback);
			break;
		case EState::Rejected:
			(void)Callback.ExecuteIfBound(FailureReason.GetValue());
			break;
		default:
			//Do nothing
				break;
		}
		
		return LazyGetContinuation();
	}
	
	EState State = EState::Pending;
	
	TOptional<FString> FailureReason;

	// Callbacks that don't need type
	mutable TArray<FVoidThenDelegate> VoidThenCallbacks;
	mutable TArray<FCatchDelegate> CatchCallbacks;
	
	mutable TSharedPtr<FOGFutureState> ContinuationFutureState;
};

//Void specialization of TOGFutureState - implement this first as TOGFutureState<void> uses it
template<>
struct TOGFutureState<void> : FOGFutureState
{
	friend struct FOGFuture;
	friend struct TOGFuture<void>;
	friend struct FOGPromise;
	friend struct TOGPromise<void>;

	typedef FVoidThenDelegate FThenDelegate;
	
	TOGFutureState(){}

public:
	
	TOGFuture<void> Then(const FVoidThenDelegate& Callback) const //intentionally hiding parent function
	{
		return FOGFutureState::Then(Callback);
	}

	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func>>)>
	TOGFuture<void> WeakThen(const UObject* Context, Func&& Lambda) const
	{
		return Then(FVoidThenDelegate::CreateWeakLambda(Context, Lambda));
	}

	template<typename ReturnsFuture UE_REQUIRES(std::is_convertible_v<TInvokeResult_T<ReturnsFuture>,FOGFuture>)>
	TOGFuture<void> WeakThen(const UObject* Context, ReturnsFuture&& AsyncLambda) const
	{
		return FOGFutureState::WeakThen(Context, AsyncLambda);
	}

	TOGFuture<void> Catch(const FCatchDelegate& Callback) const //intentionally hiding parent function
	{
		return FOGFutureState::Catch(Callback);
	}

	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func, const FString&>>)>
	TOGFuture<void> WeakCatch(const UObject* Context, Func&& Lambda) const
	{
		return Catch(FCatchDelegate::CreateWeakLambda(Context, Lambda));
	}

	//convenience that allows you to bind then and catch in a single call
	template<typename ThenFunc, typename CatchFunc>
	TOGFuture<void> WeakThen(const UObject* Context, ThenFunc&& ThenLambda, CatchFunc&& CatchLambda) const
	{
		WeakCatch(Context, CatchLambda);
		return WeakThen(Context, ThenLambda);
	}
	
	void Fulfill()
	{
		if(!ensureAlways(State == EState::Pending)) [[unlikely]]
			return;
		
		State = EState::Fulfilled;
		ExecuteThenCallbacks();
	}

protected:
	TOGFuture<void> AddThen(const FVoidThenDelegate& Callback) const
	{
		return AddVoidThen(Callback);
	}

	// Execute all valid callbacks with the result value
	virtual void ExecuteThenCallbacks() override
	{
		//Value set but still pending will only happen while delegates are being called.
		if (!ensureAlways(State == EState::Fulfilled)) [[unlikely]]
			return;

		for (FVoidThenDelegate& VoidThen : VoidThenCallbacks)
		{
			(void)VoidThen.ExecuteIfBound();
		}

		if (ContinuationFutureState.IsValid())
		{
			static_cast<TOGFutureState<void>*>(ContinuationFutureState.Get())->Fulfill();
		}
		
		ClearCallbacks();
	}

	virtual TSharedPtr<FOGFutureState> LazyGetContinuation() const override
	{
		if (!ContinuationFutureState.IsValid())
		{
			ContinuationFutureState = MakeShared<TOGFutureState>();
		}
		return ContinuationFutureState;
	}

	static TOGFutureState* GetErrorState()
	{
		for (TSharedPtr<FOGFutureState> SharedError : ErrorStates)
		{
			if (TOGFutureState* TypedState = static_cast<TOGFutureState*>(SharedError.Get()))
				return TypedState;
		}
		const TSharedPtr<TOGFutureState> ErrorState = MakeShared<TOGFutureState>();
		ErrorState->Throw(TEXT("Promise/Future access error, data is either invalid or the wrong type."));
		ErrorStates.Add(ErrorState);
		return ErrorState.Get();
	}
};

template<typename T>
struct TOGFutureState : FOGFutureState
{
	friend struct FOGFuture;
	friend struct TOGFuture<T>;
	friend struct FOGPromise;
	friend struct TOGPromise<T>;
	
	DECLARE_DELEGATE_OneParam(FThenDelegate, const T&);

	TOGFutureState(){}
	
public:
	const T& GetValueSafe() const { return ResultValue.GetValue(); }

	bool TryGetValue(T& OutValue) const
	{
		if (!IsFulfilled())
			return false;
		OutValue = GetValueSafe();
		return true;
	}

	
	TOGFuture<T> Then(const FVoidThenDelegate& Callback) const //intentionally hiding parent function
	{
		return FOGFutureState::Then(Callback);
	}
	
	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func>>)>
	TOGFuture<T> WeakThen(const UObject* Context, Func&& Lambda) const
	{
		return Then(FVoidThenDelegate::CreateWeakLambda(Context, Lambda));
	}
	
	template<typename ReturnsFuture UE_REQUIRES(std::is_convertible_v<TInvokeResult_T<ReturnsFuture>,FOGFuture>)>
	TOGFuture<void> WeakThen(const UObject* Context, ReturnsFuture&& AsyncLambda) const
	{
		return FOGFutureState::WeakThen(Context, AsyncLambda);
	}
	
	TOGFuture<T> Then(const FThenDelegate& Callback) const //intentionally hiding parent function
	{
		return AddThen(Callback);
	}

	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func, const T&>>)>
	TOGFuture<T> WeakThen(const UObject* Context, Func&& LambdaWithParam) const
	{
		return Then(FThenDelegate::CreateWeakLambda(Context, LambdaWithParam));
	}

	template<typename U, typename ReturnsU UE_REQUIRES(std::is_same_v<TInvokeResult_T<ReturnsU, const T&>, U>)>
	TOGFuture<U> WeakThen(const UObject* Context, ReturnsU&& TransformLambda) const
	{
		TSharedRef<TOGFutureState<U>> TransformedState = MakeShared<TOGFutureState<U>>();
		TOGFuture<U> TransformFuture(TransformedState);

		WeakThen(Context, [TransformedState, TransformLambda](const T& Value) mutable
		{
			U TransformedValue = TransformLambda(Value);
			TransformedState->Fulfill(TransformedValue);
		});
		
		WeakCatch(Context, [TransformedState](const FString& Reason) mutable { TransformedState->Throw(Reason); });
		return TransformFuture;
	}

	template<typename ReturnsFutureU, typename U = typename TInvokeResult_T<ReturnsFutureU, const T&>::Type UE_REQUIRES(std::is_same_v<TInvokeResult_T<ReturnsFutureU, const T&>, TOGFuture<U>>)>
	TOGFuture<U> WeakThen(const UObject* Context, ReturnsFutureU&& AsyncTransformLambda) const
	{
		TSharedRef<TOGFutureState<U>> TransformNextState = MakeShared<TOGFutureState<U>>();
		TOGFuture<U> TransformNextFuture(TransformNextState);
		
		WeakThen(Context, [Context, TransformNextState, AsyncTransformLambda](const T& Value) mutable
		{
			AsyncTransformLambda(Value)->WeakThen(Context, [TransformNextState](const U& TransformedValue) mutable
			{
				TransformNextState->Fulfill(TransformedValue);
			}, [TransformNextState](const FString& Reason) mutable { TransformNextState->Throw(Reason); });
		}, [TransformNextState](const FString& Reason) mutable { TransformNextState->Throw(Reason); });
		return TransformNextFuture;
	}

	TOGFuture<T> Catch(const FCatchDelegate& Callback) const //intentionally hiding parent function
	{
		return FOGFutureState::Catch(Callback);
	}
	
	template<typename Func UE_REQUIRES(std::is_void_v<TInvokeResult_T<Func, const FString&>>)>
	TOGFuture<T> WeakCatch(const UObject* Context, Func&& Lambda) const
	{
		return Catch(FCatchDelegate::CreateWeakLambda(Context, Lambda));
	}

	//convenience that allows you to bind then and catch in a single call
	template<typename ThenFunc, typename CatchFunc>
	TOGFuture<T> WeakThen(const UObject* Context, ThenFunc&& ThenLambda, CatchFunc&& CatchLambda) const
	{
		WeakCatch(Context, CatchLambda);
		return WeakThen(Context, ThenLambda);
	}

	void Fulfill(const T& Value)
	{
		if(!ensureAlways(State == EState::Pending && !ResultValue.IsSet())) [[unlikely]]
			return;
		
		ResultValue.Emplace(Value);
		State = EState::Fulfilled;
		ExecuteThenCallbacks();
	}
	
protected:
	virtual const std::type_info& GetInnerTypeInfo() const override {return typeid(T); }
	
	TOGFuture<T> AddThen(const FThenDelegate& Callback) const
	{
		switch (State)
		{
		case EState::Pending:
			ThenCallbacks.Add(Callback);
			break;
		case EState::Fulfilled:
			Callback.ExecuteIfBound(ResultValue.GetValue());
			break;
		default:
			//Do nothing
			break;
		}
		
		return LazyGetContinuation();
	}
	
	// Execute all valid callbacks with the result value
	virtual void ExecuteThenCallbacks() override
	{
		//Value set but still pending will only happen while delegates are being called.
		if (!ensureAlways(ResultValue.IsSet() && State == EState::Fulfilled)) [[unlikely]]
			return;

		T Result = ResultValue.GetValue();
		for (FThenDelegate& Then : ThenCallbacks)
		{
			Then.ExecuteIfBound(Result);
		}

		for (FVoidThenDelegate& VoidThen : VoidThenCallbacks)
		{
			(void)VoidThen.ExecuteIfBound();
		}

		if (ContinuationFutureState.IsValid())
		{
			static_cast<TOGFutureState<T>*>(ContinuationFutureState.Get())->Fulfill(Result);
		}
		
		ClearCallbacks();
	}

	virtual void ClearCallbacks() override
	{
		ThenCallbacks.Empty();
		FOGFutureState::ClearCallbacks();
	}

	virtual TSharedPtr<FOGFutureState> LazyGetContinuation() const override
	{
		if (!ContinuationFutureState.IsValid())
		{
			ContinuationFutureState = MakeShared<TOGFutureState>();
		}
		return ContinuationFutureState;
	}

	static TOGFutureState* GetErrorState()
	{
		for (TSharedPtr<FOGFutureState> SharedError : ErrorStates)
		{
			if (TOGFutureState* TypedState = static_cast<TOGFutureState*>(SharedError.Get()))
				return TypedState;
		}
		const TSharedPtr<TOGFutureState> ErrorState = MakeShared<TOGFutureState>();
		ErrorState->Throw(TEXT("Promise/Future access error, data is either invalid or the wrong type."));
		ErrorStates.Add(ErrorState);
		return ErrorState.Get();
	}
	
private:
	// Store the actual value once fulfilled
	TOptional<T> ResultValue;
	
	// Typed callbacks
	mutable TArray<FThenDelegate> ThenCallbacks;
};

template <typename T>
FOGFuture::operator TOGFuture<T>()
{
	if (!IsValid())
		return TOGFuture<T>(nullptr);
	if (!ensureAlwaysMsgf(GetTypedState<T>(), TEXT("Trying to type a future to the wrong type.")))
	{
		return TOGFuture<T>(nullptr);
	}
	
	return TOGFuture<T>(SharedState);
}

template <typename T>
FOGFuture::operator const TOGFuture<T>() const
{
	if (!IsValid())
		return TOGFuture<T>(nullptr);
	if (!ensureAlwaysMsgf(GetTypedState<T>(), TEXT("Trying to type a future to the wrong type.")))
	{
		return TOGFuture<T>(nullptr);
	}
	
	return TOGFuture<T>(SharedState);
}

template <typename T>
TOGFutureState<T>* FOGFuture::GetTypedState() const
{
	if (!IsValid()) [[unlikely]]
		return TOGFutureState<T>::GetErrorState();
	if(!ensureAlwaysMsgf(typeid(T) == SharedState.Get()->GetInnerTypeInfo(), TEXT("Tried to access FutureState with the wrong type"))) [[unlikely]]
		return TOGFutureState<T>::GetErrorState();
	return static_cast<TOGFutureState<T>*>(SharedState.Get());
}

template <typename T>
TOGPromise<T>::~TOGPromise()
{
	if (IsValid() && SharedState->IsPending())
	{
		SharedState->Throw(TEXT("Promise was destroyed before it was fulfilled or failed"));
	}
}

template <typename T>
FOGPromise::operator TOGPromise<T>()
{
	if (!IsValid())
		return TOGPromise<T>(nullptr);
	if (!ensureAlwaysMsgf(GetTypedState<T>(), TEXT("Trying to type a promise to the wrong type.")))
	{
		return TOGPromise<T>(nullptr);
	}
	return TOGPromise<T>(SharedState);
}

template <typename T>
FOGPromise::operator const TOGPromise<T>() const
{
	if (!IsValid())
		return TOGPromise<T>(nullptr);
	if (!ensureAlwaysMsgf(GetTypedState<T>(), TEXT("Trying to type a promise to the wrong type.")))
	{
		return TOGPromise<T>(nullptr);
	}
	return TOGPromise<T>(SharedState);
}

template <typename T>
TOGFutureState<T>* FOGPromise::GetTypedState() const
{
	if (!IsValid()) [[unlikely]]
		return TOGFutureState<T>::GetErrorState();
	if(!ensureAlwaysMsgf(typeid(T) == SharedState.Get()->GetInnerTypeInfo(), TEXT("Tried to access FutureState with the wrong type"))) [[unlikely]]
		return TOGFutureState<T>::GetErrorState();
	return static_cast<TOGFutureState<T>*>(SharedState.Get());
}

template<typename ReturnsFuture UE_REQUIRES(std::is_convertible_v<TInvokeResult_T<ReturnsFuture>,FOGFuture>)>
	TOGFuture<void> FOGFutureState::WeakThen(const UObject* Context, ReturnsFuture&& AsyncLambda) const
{
	//Create a raw promise to avoid complications related to lambda capture and deleted copy constructor
	TSharedPtr<TOGFutureState<void>> NextState = MakeShared<TOGFutureState<void>>();
	TOGFuture<void> NextFuture(NextState);
		
	WeakThen(Context,
	[Context, NextState, AsyncLambda]() mutable
		{
			AsyncLambda()->WeakThen(Context,
				[NextState]() mutable
				{
					NextState->Fulfill();
				},
				[NextState](const FString& Reason) mutable
				{
					NextState->Throw(Reason);
				});
		},
	[NextState](const FString& Reason) mutable
	{
		NextState->Throw(Reason);
	});
	return NextFuture;
}