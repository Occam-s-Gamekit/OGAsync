/// Copyright Occam's Gamekit contributors 2025

#pragma once

#include "CoreMinimal.h"
#include "OGFuture.generated.h"

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

struct FOGFutureState
{
	typedef TSharedPtr<FOGFutureState> FSharedStatePtr;
	friend struct FOGFuture;
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

	FSharedStatePtr AddVoidThen(const FVoidThenDelegate& Callback)
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

	FSharedStatePtr AddCatch(const FCatchDelegate& Callback)
	{
		switch (State)
		{
		case EState::Pending:
			CatchCallbacks.Add(Callback);
			break;
		case EState::Rejected:
			Callback.ExecuteIfBound(FailureReason.GetValue());
			break;
		default:
			//Do nothing
				break;
		}
		
		return LazyGetContinuation();
	}
	
	void Throw(const FString& Reason)
	{
		if (!ensure(State == EState::Pending) && !FailureReason.IsSet()) [[unlikely]]
			return;

		FailureReason.Emplace(Reason);
		State = EState::Rejected;
		ExecuteCatchCallbacks();
	}
	
protected:

	void ExecuteCatchCallbacks();

	virtual void ClearCallbacks()
	{
        VoidThenCallbacks.Empty();
        CatchCallbacks.Empty();
	}
	
	virtual FSharedStatePtr LazyGetContinuation()
	{
		if (!ContinuationFutureState.IsValid())
		{
			ContinuationFutureState = MakeShared<FOGFutureState>();
		}
		return ContinuationFutureState;
	}
	
protected:
	EState State = EState::Pending;
	
	TOptional<FString> FailureReason;

	// Callbacks that don't need type
	TArray<FVoidThenDelegate> VoidThenCallbacks;
	TArray<FCatchDelegate> CatchCallbacks;
	
	TSharedPtr<FOGFutureState> ContinuationFutureState;
};

template<typename T>
struct TOGFutureState : FOGFutureState
{
	friend struct TOGFuture<T>;
	friend struct FOGPromise;
	friend struct TOGPromise<T>;

	DECLARE_DELEGATE_OneParam(FThenDelegate, const T&);

	TOGFutureState(){}
	
public:
	const T& GetValue() const { return ResultValue.GetValue(); }

	FSharedStatePtr AddThen(const FThenDelegate& Callback)
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
	
	void SetFutureValue(const T& Value)
	{
		if(!ensure(State == EState::Pending && !ResultValue.IsSet())) [[unlikely]]
			return;
		
		ResultValue.Emplace(Value);
		State = EState::Fulfilled;
		ExecuteThenCallbacks();
	}

private:
	// Execute all valid callbacks with the result value
	void ExecuteThenCallbacks()
	{
		//Value set but still pending will only happen while delegates are being called.
		if (!ensure(ResultValue.IsSet() && State == EState::Fulfilled)) [[unlikely]]
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
			static_cast<TOGFutureState<T>*>(ContinuationFutureState.Get())->SetFutureValue(Result);
		}
		
		ClearCallbacks();
	}

	virtual void ClearCallbacks() override
	{
		ThenCallbacks.Empty();
		FOGFutureState::ClearCallbacks();
	}

	virtual FSharedStatePtr LazyGetContinuation() override
	{
		if (!ContinuationFutureState.IsValid())
		{
			ContinuationFutureState = MakeShared<TOGFutureState>();
		}
		return ContinuationFutureState;
	}
	
private:
	// Store the actual value once fulfilled
	TOptional<T> ResultValue;
	
	// Typed callbacks
	TArray<FThenDelegate> ThenCallbacks;
};

template<>
struct TOGFutureState<void> : FOGFutureState
{
	friend struct TOGFuture<void>;
	friend struct FOGPromise;
	friend struct TOGPromise<void>;

	TOGFutureState(){}
	
public:
	FSharedStatePtr AddThen(const FVoidThenDelegate& Callback)
	{
		return AddVoidThen(Callback);
	}
	
	void SetFutureValue()
	{
		if(!ensure(State == EState::Pending)) [[unlikely]]
			return;
		
		State = EState::Fulfilled;
		ExecuteThenCallbacks();
	}

private:
	// Execute all valid callbacks with the result value
	void ExecuteThenCallbacks()
	{
		//Value set but still pending will only happen while delegates are being called.
		if (!ensure(State == EState::Fulfilled)) [[unlikely]]
			return;

		for (FVoidThenDelegate& VoidThen : VoidThenCallbacks)
		{
			(void)VoidThen.ExecuteIfBound();
		}

		if (ContinuationFutureState.IsValid())
		{
			static_cast<TOGFutureState<void>*>(ContinuationFutureState.Get())->SetFutureValue();
		}
		
		ClearCallbacks();
	}

	virtual FSharedStatePtr LazyGetContinuation() override
	{
		if (!ContinuationFutureState.IsValid())
		{
			ContinuationFutureState = MakeShared<TOGFutureState>();
		}
		return ContinuationFutureState;
	}
};

// Define shared methods that both Future and Promise will implement
#define FUTURE_METHODS() \
public: \
    bool IsValid() const \
    { \
        return SharedState.IsValid(); \
    } \
    \
    bool IsPending() const \
    { \
        return IsValid() && SharedState->IsPending(); \
    } \
    \
    bool IsFulfilled() const \
    { \
        return IsValid() && SharedState->IsFulfilled(); \
    } \
    \
    bool IsRejected() const \
    { \
        return IsValid() && SharedState->IsRejected(); \
    } \
    \
	const FOGFuture Then(const typename FOGFutureState::FVoidThenDelegate& Callback) const \
	{ \
		if (!IsValid()) [[unlikely]]\
			return FOGFuture::EmptyFuture; \
		return FOGFuture(SharedState->AddVoidThen(Callback)); \
	} \
	const FOGFuture WeakThen(const UObject* Context, TFunction<void()> Lambda) const \
	{ \
		return Then(typename FOGFutureState::FVoidThenDelegate::CreateWeakLambda(Context, Lambda)); \
	} \
    const FOGFuture Catch(const typename FOGFutureState::FCatchDelegate& Callback) const \
    { \
        if (!IsValid()) \
            return FOGFuture::EmptyFuture; \
        return FOGFuture(SharedState->AddCatch(Callback)); \
    } \
    \
    const FOGFuture WeakCatch(const UObject* Context, TFunction<void(const FString&)> Lambda) const \
    { \
		return Catch(typename FOGFutureState::FCatchDelegate::CreateWeakLambda(Context, Lambda)); \
    } \
    \
protected: \
    template<typename T> \
    TOGFutureState<T>* GetTypedState() const \
    { \
        if (!IsValid()) \
            return nullptr; \
        TOGFutureState<T>* TypedState = static_cast<TOGFutureState<T>*>(SharedState.Get()); \
        ensureAlwaysMsgf(TypedState, TEXT("Tried to access FutureState with the wrong type")); \
        return TypedState; \
    }

// Define shared methods for templated Future/Promise types
#define TYPED_FUTURE_METHODS() \
public: \
	\
	T GetSafe() const \
	{ \
		if (!IsValid() || !IsFulfilled()) \
			return T(); \
		TOGFutureState<T>* StatePtr = GetTypedState<T>(); \
		if (!StatePtr) [[unlikely]]\
			return T(); \
		return StatePtr->GetValue(); \
	} \
	\
    bool TryGet(T& OutValue) const \
    { \
        if (!IsValid() || !IsFulfilled()) \
            return false; \
        TOGFutureState<T>* StatePtr = GetTypedState<T>(); \
        if (!StatePtr) [[unlikely]]\
            return false; \
        OutValue = StatePtr->GetValue(); \
        return true; \
    } \
    \
	const TOGFuture<T> Then(const typename FOGFutureState::FVoidThenDelegate& Callback) const \
	{ \
	if (!IsValid()) [[unlikely]]\
		return FOGFuture::EmptyFuture; \
	return TOGFuture<T>(SharedState->AddVoidThen(Callback)); \
	} \
	const TOGFuture<T> WeakThen(const UObject* Context, TFunction<void()> Lambda) const \
	{ \
		return Then(typename FOGFutureState::FVoidThenDelegate::CreateWeakLambda(Context, Lambda)); \
	} \
	\
    const TOGFuture<T> Then(const typename TOGFutureState<T>::FThenDelegate& Callback) const \
    { \
        if (!IsValid()) [[unlikely]]\
            return FOGFuture::EmptyFuture; \
        TOGFutureState<T>* StatePtr = GetTypedState<T>(); \
        if (!StatePtr) [[unlikely]]\
            return FOGFuture::EmptyFuture; \
        return TOGFuture<T>(StatePtr->AddThen(Callback)); \
    } \
    \
    const TOGFuture<T> WeakThen(const UObject* Context, TFunction<void(const T&)> Lambda) const \
    { \
        return Then(typename TOGFutureState<T>::FThenDelegate::CreateWeakLambda(Context, Lambda)); \
    } \
    \
    template<typename U> \
    const TOGFuture<U> WeakTransform(const UObject* Context, TFunction<U(const T&)> TransformLambda) const \
    { \
        if (!IsValid()) [[unlikely]]\
            return FOGFuture::EmptyFuture; \
        \
        TSharedRef<TOGFutureState<U>> TransformedState = MakeShared<TOGFutureState<U>>(); \
        TOGFuture<U> TransformFuture(TransformedState); \
        \
        TFunction<void(const T&)> Lambda = [TransformedState, TransformLambda](const T& Value) mutable \
        { \
            U TransformedValue = TransformLambda(Value); \
            TransformedState->SetFutureValue(TransformedValue); \
        }; \
        \
        WeakThen(Context, Lambda); \
        return TransformFuture; \
    }

USTRUCT(BlueprintType)
struct OGASYNC_API FOGFuture
{
	GENERATED_BODY()
	FOGFuture() {}
	FOGFuture(const FOGFutureState::FSharedStatePtr& FutureState) : SharedState(FutureState) {}

	static const FOGFuture EmptyFuture;
	
	//convert a generic future into a specific type, will error if the type doesn't match the underlying data
	template<typename T>
	operator TOGFuture<T>();

	template<typename T>
	operator const TOGFuture<T>() const;

	//common implementations between FFuture and FPromise
	FUTURE_METHODS()

protected:
	TSharedPtr<FOGFutureState> SharedState = nullptr;
};

template<typename T>
struct TOGFuture : FOGFuture
{
	friend struct TOGPromise<T>;

public:
	//Futures are created via other futures or an existing future state.
	TOGFuture() : FOGFuture(nullptr) {}
	TOGFuture(const FOGFutureState::FSharedStatePtr& FutureState) : FOGFuture(FutureState) {}

	//common implementations between TFuture and TPromise
	TYPED_FUTURE_METHODS()
};

template<>
struct TOGFuture<void> : FOGFuture
{
	friend struct TOGPromise<void>;

public:
	//Futures are created via other futures or an existing future state.
	TOGFuture() = delete;
	TOGFuture(const FOGFutureState::FSharedStatePtr& FutureState) : FOGFuture(FutureState) {}
};

USTRUCT(BlueprintType)
struct OGASYNC_API FOGPromise
{
	GENERATED_BODY()
	FOGPromise() {}
	FOGPromise(const FOGFutureState::FSharedStatePtr& State) : SharedState(State) {}

	operator FOGFuture() const
	{
		return FOGFuture(SharedState);
	}

	template<typename T>
	void Fulfill(const T& Value)
	{
		TOGFutureState<T>* TypedState = GetTypedState<T>();
		if (!TypedState) [[unlikely]]
			return;
		TypedState->SetFutureValue(Value);
	}

	void Throw(const FString& Reason) const
	{
		if(!IsValid())
			return;
		SharedState->Throw(Reason);
	}

	FUTURE_METHODS()

protected:
	//Mutable so the copy constructor can null the src
	mutable TSharedPtr<FOGFutureState> SharedState = nullptr;
};

template<typename T>
struct TOGPromise : FOGPromise
{
public:
	typedef TOGFuture<T> TFutureType;
	
	TOGPromise() : FOGPromise(MakeShared<TOGFutureState<T>>()) {}
	~TOGPromise()
	{
		if (IsValid() && IsPending())
		{
			Throw(TEXT("Promise was deleted before it was fulfilled or failed"));
		}
	}

	//Promises can not be copied, this is to ensure only one system is expected to fulfill the promise.
	TOGPromise(const TOGPromise&) = delete;
	TOGPromise& operator=(const TOGPromise&) = delete;
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

	void Fulfill(const T& Value)
	{
		if (!ensureAlwaysMsgf(IsValid(), TEXT("Trying to fulfill an empty promise"))) [[unlikely]]
			return;
		TOGFutureState<T>* StatePtr = GetTypedState<T>();
		if (!StatePtr) [[unlikely]]
			return;
		StatePtr->SetFutureValue(Value);
	}

	TYPED_FUTURE_METHODS()
};

template<>
struct TOGPromise<void> : FOGPromise
{
public:
	typedef TOGFuture<void> TFutureType;
	
	TOGPromise() : FOGPromise(MakeShared<TOGFutureState<void>>()) {}

	//Promises can not be copied, this is to ensure only one system is expected to fulfill the promise.
	TOGPromise(const TOGPromise&) = delete;
	TOGPromise& operator=(const TOGPromise&) = delete;
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
	operator TOGFuture<void>() const
	{
		return TOGFuture<void>(SharedState);
	}

	void Fulfill() const
	{
		if (!ensureAlwaysMsgf(IsValid(), TEXT("Trying to fulfill an empty promise"))) [[unlikely]]
			return;
		TOGFutureState<void>* StatePtr = GetTypedState<void>();
		if (!StatePtr) [[unlikely]]
			return;
		StatePtr->SetFutureValue();
	}
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