#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "OGAsync/Public/OGFuture.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureBasicTest, "OGAsync.Futures.Basic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureBasicTest::RunTest(const FString& Parameters)
{
    // This will get cleaned up when it leaves scope
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};
    
    // Test 1: Basic Promise Fulfillment
    {
        TOGPromise<int> Promise;
        
        TestFalse(TEXT("Future should not be set initially"), Promise.IsFulfilled());
        
        int Value = 42;
        Promise.Fulfill(Value);
        
        int OutValue;
        TestTrue(TEXT("Future should be set after fulfillment"),Promise.TryGet(OutValue));
        TestEqual(TEXT("Future value should match fulfilled value"), OutValue, 42);
    }

    // Test 2: Then Callback - Delayed Fulfillment
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        int ReceivedValue = 0;
        
        Promise.WeakThen(ContextObject, [&CallbackExecuted, &ReceivedValue](const int& Value) {
            CallbackExecuted = true;
            ReceivedValue = Value;
        });
        
        int Value = 123;
        Promise.Fulfill(Value);
        
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
        TestEqual(TEXT("Callback should receive correct value"), ReceivedValue, 123);
    }

    // Test 3: Then Callback - Immediate Fulfillment
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        
        int Value = 456;
        Promise.Fulfill(Value);
        
        Promise.WeakThen(ContextObject, [&CallbackExecuted](const int& Value) {
            CallbackExecuted = true;
        });
        
        TestTrue(TEXT("Callback should be executed for already fulfilled future"), CallbackExecuted);
    }

    // Test 4: Multiple Callbacks
    {
        TOGPromise<int> Promise;
        int CallbackCount = 0;
        
        Promise.Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        Promise.Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        Promise.Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        
        int Value = 789;
        Promise.Fulfill(Value);
        
        TestEqual(TEXT("All callbacks should be executed"), CallbackCount, 3);
    }

    // Test 5: WeakThen with Valid Context
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        
        TFunction<void(const int&)> Lambda = [&CallbackExecuted](const int& Value) {
            CallbackExecuted = true;
        };
        
        (void)Promise.WeakThen(ContextObject, Lambda);
        
        int Value = 42;
        Promise.Fulfill(Value);
        
        TestTrue(TEXT("WeakThen callback should execute with valid context"), CallbackExecuted);
    }

    // Test 6: Transform
    {
        TOGPromise<int> Promise;
        TOGFuture<int> Future = Promise;
        bool TransformExecuted = false;
        
        TFunction<FString(const int&)> TransformLambda = [&TransformExecuted](const int& Value) {
            TransformExecuted = true;
            return FString::Printf(TEXT("%d"), Value);
        };
        
        TOGFuture<FString> TransformedFuture = Future.WeakTransform<FString>(ContextObject, TransformLambda);
        
        int Value = 42;
        Promise.Fulfill(Value);
        
        TestTrue(TEXT("Transform should be executed"), TransformExecuted);
        TestEqual(TEXT("Transformed value should be correct"), TransformedFuture.GetSafe(), TEXT("42"));
    }

    // Test 7: Promise Move Semantics
    {
        TOGPromise<int> Promise1;
        TOGFuture<int> Future1 = Promise1;
        
        TOGPromise<int> Promise2 = MoveTemp(Promise1);
        TestFalse(TEXT("Original Promise is invalid after being moved"), Promise1.IsValid());
        int Value = 42;
        Promise2.Fulfill(Value);
        
        TestTrue(TEXT("Future from moved promise should be fulfilled"), Future1.IsFulfilled());
        TestEqual(TEXT("Future from moved promise should have correct value"), Future1.GetSafe(), 42);
    }

    // Test 8: void continuation on typed promise
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;

        Promise.WeakThen(ContextObject, [&CallbackExecuted]() {
            CallbackExecuted = true;
        });
        
        int Value = 123;
        Promise.Fulfill(Value);
        
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
    }

    // Test 9: Promise<void> completion
    {
        TOGPromise<void> Promise1;
        TOGFuture<void> Future1 = Promise1;

        bool CallbackExecuted = false;
        Promise1.WeakThen(ContextObject, [&CallbackExecuted]()
        {
            CallbackExecuted = true;
        });

        TestFalse(TEXT("Callback should not be executed"), CallbackExecuted);
        Promise1.Fulfill();
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureErrorHandlingTest, "OGAsync.Futures.ErrorHandling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureErrorHandlingTest::RunTest(const FString& Parameters)
{
    // This will get cleaned up when it leaves scope
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};

    // Test 1: Basic Error Handling
    {
        TOGPromise<int> Promise;
        bool CatchExecuted = false;
        FString CaughtReason;

        Promise.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
            CaughtReason = Reason;
        }));

        Promise.Throw(TEXT("Test Error"));

        TestTrue(TEXT("Catch callback should be executed"), CatchExecuted);
        TestEqual(TEXT("Error reason should match"), CaughtReason, TEXT("Test Error"));
    }

    // Test 2: Error Propagation Through Chain
    {
        TOGPromise<int> Promise;
        int CatchCount = 0;
        
        auto Future = Promise
            .Then(FOGFutureState::FVoidThenDelegate::CreateLambda([]() {}))
            .Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }))
            .Then(FOGFutureState::FVoidThenDelegate::CreateLambda([]() {}))
            .Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));

        Promise.Throw(TEXT("Chain Error"));
        
        TestEqual(TEXT("All catch handlers in chain should execute"), CatchCount, 2);
    }

    // Test 3: Immediate Error Handling
    {
        TOGPromise<int> Promise;
        bool CatchExecuted = false;
        
        Promise.Throw(TEXT("Immediate Error"));
        
        Promise.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
        }));
        
        TestTrue(TEXT("Catch should execute immediately for rejected promise"), CatchExecuted);
    }

    // Test 4: Error State Check
    {
        TOGPromise<int> Promise;
        
        TestFalse(TEXT("Promise should not be rejected initially"), Promise.IsRejected());
        
        Promise.Throw(TEXT("State Test Error"));
        
        TestTrue(TEXT("Promise should be rejected after throw"), Promise.IsRejected());
        TestFalse(TEXT("Rejected promise should not be pending"), Promise.IsPending());
        TestFalse(TEXT("Rejected promise should not be fulfilled"), Promise.IsFulfilled());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureContinuationTest, "OGAsync.Futures.Continuation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureContinuationTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};

    // Test 1: Basic Continuation Chain
    {
        TOGPromise<int> Promise;
        int ChainSum = 0;
        
        auto Future = Promise
            .Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ChainSum += Value;
                TestEqual(TEXT("Chain operations should execute in order : first future"), ChainSum, 5);
            }))
            .Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ChainSum += 10;
                TestEqual(TEXT("Chain operations should execute in order : second future"), ChainSum, 15);
            }));

        int Value = 5;
        Promise.Fulfill(Value);
        
        TestEqual(TEXT("Chain operations should execute in order : final"), ChainSum, 15);
    }

    // Test 2: Type Transformation Chain
    {
        TOGPromise<int> Promise;
        FString FinalResult;
        
        auto TransformedFuture = Promise
            .WeakTransform<float>(ContextObject, [](const int& Value) {
                return Value * 1.5f;
            })
            .WeakTransform<FString>(ContextObject, [](const float& Value) {
                return FString::Printf(TEXT("%.1f"), Value);
            });

        Promise.Fulfill(10);
        
        TestEqual(TEXT("Chain transformations should be applied correctly"), 
                 TransformedFuture.GetSafe(), TEXT("15.0"));
    }

    // Test 3: Mixed Continuation Types
    {
        TOGPromise<int> Promise;
        TArray<FString> ExecutionOrder;
        
        auto Future = Promise
            .Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ExecutionOrder.Add(TEXT("First"));
            }))
            .Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ExecutionOrder.Add(TEXT("Second"));
            }))
            .Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                ExecutionOrder.Add(TEXT("Error"));
            }));

        Promise.Fulfill(42);
        
        TestEqual(TEXT("First continuation executed"), ExecutionOrder[0], TEXT("First"));
        TestEqual(TEXT("Second continuation executed"), ExecutionOrder[1], TEXT("Second"));
        TestEqual(TEXT("Total continuations executed"), ExecutionOrder.Num(), 2);
    }

    // Test 4: Continuation After Error
    {
        TOGPromise<int> Promise;
        TArray<FString> ExecutionOrder;
        
        auto Future = Promise
            .Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ExecutionOrder.Add(TEXT("Then"));
            }))
            .Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                ExecutionOrder.Add(TEXT("Catch"));
            }))
            .Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ExecutionOrder.Add(TEXT("After Catch"));
            }));

        Promise.Fulfill(42);
        
        TestEqual(TEXT("Then should execute"), ExecutionOrder[0], TEXT("Then"));
        TestEqual(TEXT("After should execute"), ExecutionOrder[1], TEXT("After Catch"));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureEdgeCasesTest, "OGAsync.Futures.EdgeCases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureEdgeCasesTest::RunTest(const FString& Parameters)
{
    // This will get cleaned up when it leaves scope
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();

    if (!World)
        return false;

    // Test 1: WeakThen with Invalid Context
    {
        AActor* ContextObject = World->SpawnActor<AActor>();
        TOGPromise<int> Promise;
        const TOGFuture<int> Future = Promise;
        bool CallbackExecuted = false;
        
        const TFunction<void(const int&)> Lambda = [&CallbackExecuted](const int& Value) {
            CallbackExecuted = true;
        };
        
        (void)Future.WeakThen(ContextObject, Lambda);
        
        // Invalidate the context
        World->DestroyActor(ContextObject);
        
        int Value = 42;
        Promise.Fulfill(Value);
        
        TestFalse(TEXT("WeakThen callback should not execute with invalid context"), CallbackExecuted);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureMultipleAssignmentTest, "OGAsync.Futures.MultipleAssignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::NegativeFilter)

bool FOGFutureMultipleAssignmentTest::RunTest(const FString& Parameters)
{
    // Test 1: Multiple Fulfill Attempts
    {
        TOGPromise<int> Promise;
        const TOGFuture<int> Future = Promise;
        
        int Value1 = 42;
        Promise.Fulfill(Value1);
        
        int Value2 = 84;
        Promise.Fulfill(Value2);  // Should be ignored
        
        TestEqual(TEXT("Only first fulfillment should count"), Future.GetSafe(), 84);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureDestructionTest, "OGAsync.Futures.Destruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureDestructionTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};

    // Test 1: Promise Destruction While Pending
    {
        TOGFuture<int> Future;
        bool CatchExecuted = false;
        FString CaughtReason;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;  // Keep the future alive after promise destruction
            
            Future.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchExecuted = true;
                CaughtReason = Reason;
            }));
            
            // Promise will be destroyed here when leaving scope
        }
        
        TestTrue(TEXT("Catch should execute when promise is destroyed while pending"), CatchExecuted);
        TestTrue(TEXT("Error reason should mention destruction"), 
                CaughtReason.Contains(TEXT("destroyed")));
        TestTrue(TEXT("Future should be in rejected state"), Future.IsRejected());
    }

    // Test 2: Promise Destruction After Fulfillment
    {
        TOGFuture<int> Future;
        bool CatchExecuted = false;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;
            
            Future.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchExecuted = true;
            }));
            
            int Value = 42;
            Promise.Fulfill(Value);
            // Promise will be destroyed here
        }
        
        TestFalse(TEXT("Catch should not execute when promise is destroyed after fulfillment"), 
                 CatchExecuted);
        TestTrue(TEXT("Future should remain fulfilled"), Future.IsFulfilled());
        TestEqual(TEXT("Future should retain its value"), Future.GetSafe(), 42);
    }

    // Test 3: Promise Destruction After Rejection
    {
        TOGFuture<int> Future;
        int CatchCount = 0;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;
            
            Future.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            
            Promise.Throw(TEXT("Explicit Error"));
            // Promise will be destroyed here
        }
        
        TestEqual(TEXT("Catch should execute only once"), CatchCount, 1);
        TestTrue(TEXT("Future should remain rejected"), Future.IsRejected());
    }

    // Test 4: Multiple Futures Surviving Promise
    {
        TOGFuture<int> Future1;
        TOGFuture<int> Future2;
        int CatchCount = 0;
        
        {
            TOGPromise<int> Promise;
            Future1 = Promise;
            Future2 = Promise;
            
            Future1.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            
            Future2.Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            // Promise will be destroyed here
        }
        
        TestEqual(TEXT("Both catch handlers should execute"), CatchCount, 2);
        TestTrue(TEXT("First future should be rejected"), Future1.IsRejected());
        TestTrue(TEXT("Second future should be rejected"), Future2.IsRejected());
    }

    return true;
}