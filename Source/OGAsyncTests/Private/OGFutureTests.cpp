﻿/// Copyright Occam's Gamekit contributors 2025

#include "CoreMinimal.h"
#include "OGFutureUtilities.h"
#include "Misc/AutomationTest.h"
#include "OGAsync/Public/OGFuture.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureBasicTest, "OccamsGamekit.OGAsync.Futures.Basic",
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
        TestFalse(TEXT("Future should not be set initially"), Promise->IsFulfilled());
        
        int Value = 42;
        Promise->Fulfill(Value);
        
        int OutValue;
        TestTrue(TEXT("Future should be set after fulfillment"),Promise->TryGetValue(OutValue));
        TestEqual(TEXT("Future value should match fulfilled value"), OutValue, 42);
    }

    // Test 2: Then Callback - Delayed Fulfillment
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        int ReceivedValue = 0;
        
        Promise->WeakThen(ContextObject, [&CallbackExecuted, &ReceivedValue](const int& Value) {
            CallbackExecuted = true;
            ReceivedValue = Value;
        });
        
        int Value = 123;
        Promise->Fulfill(Value);
        
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
        TestEqual(TEXT("Callback should receive correct value"), ReceivedValue, 123);
    }

    // Test 3: Then Callback - Immediate Fulfillment
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        
        int Value = 456;
        Promise->Fulfill(Value);
        
        Promise->WeakThen(ContextObject, [&CallbackExecuted](const int& Value) {
            CallbackExecuted = true;
        });
        
        TestTrue(TEXT("Callback should be executed for already fulfilled future"), CallbackExecuted);
    }

    // Test 4: Multiple Callbacks
    {
        TOGPromise<int> Promise;
        int CallbackCount = 0;
        
        Promise->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        Promise->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        Promise->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&CallbackCount](const int& Value) { ++CallbackCount; }));
        
        int Value = 789;
        Promise->Fulfill(Value);
        
        TestEqual(TEXT("All callbacks should be executed"), CallbackCount, 3);
    }

    // Test 5: WeakThen with Valid Context
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;
        
        TFunction<void(const int&)> Lambda = [&CallbackExecuted](const int& Value) {
            CallbackExecuted = true;
        };
        
        (void)Promise->WeakThen(ContextObject, Lambda);
        
        int Value = 42;
        Promise->Fulfill(Value);
        
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
        
        TOGFuture<FString> TransformedFuture = Future->WeakThen<FString>(ContextObject, TransformLambda);
        
        int Value = 42;
        Promise->Fulfill(Value);
        
        TestTrue(TEXT("Transform should be executed"), TransformExecuted);
        TestEqual(TEXT("Transformed value should be correct"), TransformedFuture->GetValueSafe(), TEXT("42"));
    }

    // Test 7: Promise Move Semantics
    {
        TOGPromise<int> Promise1;
        TOGFuture<int> Future1 = Promise1;
        
        TOGPromise<int> Promise2 = MoveTemp(Promise1);
        TestFalse(TEXT("Original Promise is invalid after being moved"), Promise1.IsValid());
        int Value = 42;
        Promise2->Fulfill(Value);
        
        TestTrue(TEXT("Future from moved promise should be fulfilled"), Future1->IsFulfilled());
        TestEqual(TEXT("Future from moved promise should have correct value"), Future1->GetValueSafe(), 42);
    }

    // Test 8: void continuation on typed promise
    {
        TOGPromise<int> Promise;
        bool CallbackExecuted = false;

        Promise->WeakThen(ContextObject, [&CallbackExecuted]() {
            CallbackExecuted = true;
        });
        
        int Value = 123;
        Promise->Fulfill(Value);
        
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
    }

    // Test 9: Promise<void> completion
    {
        TOGPromise<void> Promise1;
        TOGFuture<void> Future1 = Promise1;

        bool CallbackExecuted = false;
        Promise1->WeakThen(ContextObject, [&CallbackExecuted]()
        {
            CallbackExecuted = true;
        });

        TestFalse(TEXT("Callback should not be executed"), CallbackExecuted);
        Promise1->Fulfill();
        TestTrue(TEXT("Callback should be executed"), CallbackExecuted);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureErrorHandlingTest, "OccamsGamekit.OGAsync.Futures.ErrorHandling",
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

        Promise->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
            CaughtReason = Reason;
        }));

        Promise->Throw(TEXT("Test Error"));

        TestTrue(TEXT("Catch callback should be executed"), CatchExecuted);
        TestEqual(TEXT("Error reason should match"), CaughtReason, TEXT("Test Error"));
    }

    // Test 2: Error Propagation Through Chain
    {
        TOGPromise<int> Promise;
        int CatchCount = 0;
        
        auto Future = Promise
            ->Then(FOGFutureState::FVoidThenDelegate::CreateLambda([]() {}))
            ->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }))
            ->Then(FOGFutureState::FVoidThenDelegate::CreateLambda([]() {}))
            ->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));

        Promise->Throw(TEXT("Chain Error"));
        
        TestEqual(TEXT("All catch handlers in chain should execute"), CatchCount, 2);
    }

    // Test 3: Immediate Error Handling
    {
        TOGPromise<int> Promise;
        bool CatchExecuted = false;
        
        Promise->Throw(TEXT("Immediate Error"));
        
        Promise->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
        }));
        
        TestTrue(TEXT("Catch should execute immediately for rejected promise"), CatchExecuted);
    }

    // Test 4: Error State Check
    {
        TOGPromise<int> Promise;
        
        TestFalse(TEXT("Promise should not be rejected initially"), Promise->IsRejected());
        
        Promise->Throw(TEXT("State Test Error"));
        
        TestTrue(TEXT("Promise should be rejected after throw"), Promise->IsRejected());
        TestFalse(TEXT("Rejected promise should not be pending"), Promise->IsPending());
        TestFalse(TEXT("Rejected promise should not be fulfilled"), Promise->IsFulfilled());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureContinuationTest, "OccamsGamekit.OGAsync.Futures.Continuation",
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
            ->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ChainSum += Value;
                TestEqual(TEXT("Chain operations should execute in order : first future"), ChainSum, 5);
            }))
            ->Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ChainSum += 10;
                TestEqual(TEXT("Chain operations should execute in order : second future"), ChainSum, 15);
            }));

        int Value = 5;
        Promise->Fulfill(Value);
        
        TestEqual(TEXT("Chain operations should execute in order : final"), ChainSum, 15);
    }

    // Test 2: Type Transformation Chain
    {
        TOGPromise<int> Promise;
        FString FinalResult;
        
        auto TransformedFuture = Promise
            ->WeakThen<float>(ContextObject, [](const int& Value) {
                return Value * 1.5f;
            })
            ->WeakThen<FString>(ContextObject, [](const float& Value) {
                return FString::Printf(TEXT("%.1f"), Value);
            });

        Promise->Fulfill(10);
        
        TestEqual(TEXT("Chain transformations should be applied correctly"), 
                 TransformedFuture->GetValueSafe(), TEXT("15.0"));
    }

    // Test 3: Mixed Continuation Types
    {
        TOGPromise<int> Promise;
        TArray<FString> ExecutionOrder;
        
        auto Future = Promise
            ->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ExecutionOrder.Add(TEXT("First"));
            }))
            ->Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ExecutionOrder.Add(TEXT("Second"));
            }))
            ->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                ExecutionOrder.Add(TEXT("Error"));
            }));

        Promise->Fulfill(42);
        
        TestEqual(TEXT("First continuation executed"), ExecutionOrder[0], TEXT("First"));
        TestEqual(TEXT("Second continuation executed"), ExecutionOrder[1], TEXT("Second"));
        TestEqual(TEXT("Total continuations executed"), ExecutionOrder.Num(), 2);
    }

    // Test 4: Continuation After Error
    {
        TOGPromise<int> Promise;
        TArray<FString> ExecutionOrder;
        
        auto Future = Promise
            ->Then(TOGFutureState<int>::FThenDelegate::CreateLambda([&](const int& Value) {
                ExecutionOrder.Add(TEXT("Then"));
            }))
            ->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                ExecutionOrder.Add(TEXT("Catch"));
            }))
            ->Then(FOGFutureState::FVoidThenDelegate::CreateLambda([&]() {
                ExecutionOrder.Add(TEXT("After Catch"));
            }));

        Promise->Fulfill(42);
        
        TestEqual(TEXT("Then should execute"), ExecutionOrder[0], TEXT("Then"));
        TestEqual(TEXT("After should execute"), ExecutionOrder[1], TEXT("After Catch"));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureEdgeCasesTest, "OccamsGamekit.OGAsync.Futures.EdgeCases",
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
        
        (void)Future->WeakThen(ContextObject, Lambda);
        
        // Invalidate the context
        World->DestroyActor(ContextObject);
        
        int Value = 42;
        Promise->Fulfill(Value);
        
        TestFalse(TEXT("WeakThen callback should not execute with invalid context"), CallbackExecuted);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureErrorsDoNotCrash, "OccamsGamekit.OGAsync.Futures.ErrorsDoNotCrash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::NegativeFilter)

bool FOGFutureErrorsDoNotCrash::RunTest(const FString& Parameters)
{
    // Test 1: Multiple Fulfill Attempts
    {
        TOGPromise<int> Promise;
        const TOGFuture<int> Future = Promise;
        
        int Value1 = 42;
        Promise->Fulfill(Value1);
        
        int Value2 = 84;
        Promise->Fulfill(Value2);  // Should be ignored
        
        TestEqual(TEXT("Only first fulfillment should count"), Future->GetValueSafe(), 84);
    }

    // Test 2: Try to use promise after it's been moved
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2 = MoveTemp(Promise1);

        //Fulfill should fail, but not crash
        Promise1->Fulfill(5);
    }

    // Test 3: Create future as one type, then convert to another
    {
        TOGPromise<bool> Promise;
        FOGFuture Future = Promise;
        TOGFuture<int> FutureInt = Future;
        int Result = FutureInt->GetValueSafe();
    }
    
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureDestructionTest, "OccamsGamekit.OGAsync.Futures.Destruction",
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
        //manually initialize to empty future 
        TOGFuture<int> Future(nullptr);
        bool CatchExecuted = false;
        FString CaughtReason;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;  // Keep the future alive after promise destruction
            
            Future->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchExecuted = true;
                CaughtReason = Reason;
            }));
            
            // Promise will be destroyed here when leaving scope
        }
        
        TestTrue(TEXT("Catch should execute when promise is destroyed while pending"), CatchExecuted);
        TestTrue(TEXT("Error reason should mention destruction"), 
                CaughtReason.Contains(TEXT("destroyed")));
        TestTrue(TEXT("Future should be in rejected state"), Future->IsRejected());
    }

    // Test 2: Promise Destruction After Fulfillment
    {
        TOGFuture<int> Future(nullptr);
        bool CatchExecuted = false;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;
            
            Future->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchExecuted = true;
            }));
            
            int Value = 42;
            Promise->Fulfill(Value);
            // Promise will be destroyed here
        }
        
        TestFalse(TEXT("Catch should not execute when promise is destroyed after fulfillment"), 
                 CatchExecuted);
        TestTrue(TEXT("Future should remain fulfilled"), Future->IsFulfilled());
        TestEqual(TEXT("Future should retain its value"), Future->GetValueSafe(), 42);
    }

    // Test 3: Promise Destruction After Rejection
    {
        TOGFuture<int> Future(nullptr);
        int CatchCount = 0;
        
        {
            TOGPromise<int> Promise;
            Future = Promise;
            
            Future->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            
            Promise->Throw(TEXT("Explicit Error"));
            // Promise will be destroyed here
        }
        
        TestEqual(TEXT("Catch should execute only once"), CatchCount, 1);
        TestTrue(TEXT("Future should remain rejected"), Future->IsRejected());
    }

    // Test 4: Multiple Futures Surviving Promise
    {
        TOGFuture<int> Future1(nullptr);
        TOGFuture<int> Future2(nullptr);
        int CatchCount = 0;
        
        {
            TOGPromise<int> Promise;
            Future1 = Promise;
            Future2 = Promise;
            
            Future1->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            
            Future2->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                CatchCount++;
            }));
            // Promise will be destroyed here
        }
        
        TestEqual(TEXT("Both catch handlers should execute"), CatchCount, 2);
        TestTrue(TEXT("First future should be rejected"), Future1->IsRejected());
        TestTrue(TEXT("Second future should be rejected"), Future2->IsRejected());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureAsyncChainTest, "OccamsGamekit.OGAsync.Futures.AsyncChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureAsyncChainTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};

    // Test 1: Basic Future Chain
    {
        TOGPromise<int> InitialPromise;
        TOGPromise<int> SecondPromise;
        TOGPromise<int> ThirdPromise;
        TArray<FString> ExecutionOrder;
        
        auto Future = InitialPromise->WeakThen(ContextObject, [&]()->FOGPromise
        {
            ExecutionOrder.Add(TEXT("First"));
            return SecondPromise;
        })
        ->WeakThen(ContextObject, [&]() -> FOGFuture
        {
            ExecutionOrder.Add(TEXT("Second"));
            return ThirdPromise;
        })->WeakThen(ContextObject, [&]() {
            ExecutionOrder.Add(TEXT("Third"));
        });
        
        // Start the chain
        InitialPromise->Fulfill(1);
        
        TestEqual(TEXT("Only first step should execute"), ExecutionOrder.Num(), 1);
        TestEqual(TEXT("First step executed"), ExecutionOrder[0], TEXT("First"));
        
        // Complete second promise
        SecondPromise->Fulfill(2);
        
        TestEqual(TEXT("Two steps should be complete"), ExecutionOrder.Num(), 2);
        TestEqual(TEXT("Second step executed"), ExecutionOrder[1], TEXT("Second"));
        
        // Complete final promise
        ThirdPromise->Fulfill(3);
        
        TestEqual(TEXT("All steps complete"), ExecutionOrder.Num(), 3);
        TestEqual(TEXT("Third step executed"), ExecutionOrder[2], TEXT("Third"));
    }

    // Test 2: Error Handling in Chain
    {
        TOGPromise<int> FirstPromise;
        TOGPromise<int> SecondPromise;
        
        TArray<FString> ExecutionOrder;
        
        auto Future = FirstPromise
            ->WeakThen(ContextObject, [&]() -> FOGFuture {
                ExecutionOrder.Add(TEXT("First"));
                return SecondPromise;
            })
            ->WeakThen(ContextObject, [&]() {
                ExecutionOrder.Add(TEXT("Should Not Execute"));
            })
            ->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
                ExecutionOrder.Add(TEXT("Catch"));
            }));
        
        FirstPromise->Fulfill(1);
        SecondPromise->Throw(TEXT("Test Error"));
        
        TestEqual(TEXT("Only first and catch should execute"), ExecutionOrder.Num(), 2);
        TestEqual(TEXT("First step executed"), ExecutionOrder[0], TEXT("First"));
        TestEqual(TEXT("Catch executed"), ExecutionOrder[1], TEXT("Catch"));
    }

    // Test 3: Nested Future Chains
    {
        TOGPromise<int> OuterPromise;
        TOGPromise<int> InnerPromise1;
        TOGPromise<int> InnerPromise2;
        
        TArray<FString> ExecutionOrder;
        
        auto Future = OuterPromise->WeakThen(ContextObject, [&]() -> FOGFuture {
            ExecutionOrder.Add(TEXT("Outer"));
            return InnerPromise1->WeakThen(ContextObject, [&]() -> FOGFuture {
                ExecutionOrder.Add(TEXT("Inner1"));
                return InnerPromise2;
            })->WeakThen(ContextObject, [&]() {
                ExecutionOrder.Add(TEXT("Inner2"));
            });
        });
        
        OuterPromise->Fulfill(1);
        TestEqual(TEXT("Outer executed"), ExecutionOrder[0], TEXT("Outer"));
        
        InnerPromise1->Fulfill(2);
        TestEqual(TEXT("Inner1 executed"), ExecutionOrder[1], TEXT("Inner1"));
        
        InnerPromise2->Fulfill(3);
        TestEqual(TEXT("Inner2 executed"), ExecutionOrder[2], TEXT("Inner2"));
        
        TestEqual(TEXT("All steps executed in order"), ExecutionOrder.Num(), 3);
    }

    // Test 4: Multiple Async Operations
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        int Sum = 0;
        
        auto Future = Promise1->WeakThen(ContextObject, [&]() -> FOGFuture {
            Sum += 1;
            return Promise2;
        })->WeakThen(ContextObject, [&]() -> FOGFuture {
            Sum += 2;
            return Promise3;
        })->WeakThen(ContextObject, [&]() {
            Sum += 3;
        });
        
        Promise1->Fulfill(1);
        TestEqual(TEXT("First addition"), Sum, 1);
        
        Promise3->Fulfill(3);  // Complete out of order
        TestEqual(TEXT("No change when completing future step early"), Sum, 1);
        
        Promise2->Fulfill(2);
        TestEqual(TEXT("All steps complete"), Sum, 6);
    }

    // Test 5: Transform Future Chain
    {
        TOGPromise<int> InitialPromise;
        TOGPromise<FString> SecondPromise;
        TOGPromise<float> ThirdPromise;
        TArray<FString> ExecutionOrder;
        
        auto Future = InitialPromise->WeakThen(ContextObject, [&](const int& Value)->TOGFuture<FString>
        {
            ExecutionOrder.Add(TEXT("First"));
            return SecondPromise;
        })
        ->WeakThen(ContextObject, [&](const FString& Value) -> TOGFuture<float>
        {
            ExecutionOrder.Add(TEXT("Second"));
            return ThirdPromise;
        })->WeakThen(ContextObject, [&]() {
            ExecutionOrder.Add(TEXT("Third"));
        });
        
        // Start the chain
        InitialPromise->Fulfill(1);
        
        TestEqual(TEXT("Only first step should execute"), ExecutionOrder.Num(), 1);
        TestEqual(TEXT("First step executed"), ExecutionOrder[0], TEXT("First"));
        
        // Complete second promise
        SecondPromise->Fulfill(TEXT("2"));
        
        TestEqual(TEXT("Two steps should be complete"), ExecutionOrder.Num(), 2);
        TestEqual(TEXT("Second step executed"), ExecutionOrder[1], TEXT("Second"));
        
        // Complete final promise
        ThirdPromise->Fulfill(3.0);
        
        TestEqual(TEXT("All steps complete"), ExecutionOrder.Num(), 3);
        TestEqual(TEXT("Third step executed"), ExecutionOrder[2], TEXT("Third"));
    }
    
    // Test 6: Chain with Invalid Context
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        
        TArray<FString> ExecutionOrder;
        
        auto Future = Promise1->WeakThen(ContextObject, [&]() -> FOGFuture {
            ExecutionOrder.Add(TEXT("First"));
            return Promise2;
        })->WeakThen(ContextObject, [&]() {
            ExecutionOrder.Add(TEXT("Second"));
        });
        
        Promise1->Fulfill(1);
        TestEqual(TEXT("First step executed"), ExecutionOrder[0], TEXT("First"));
        
        // Invalidate context
        ContextObject->Destroy();
        
        Promise2->Fulfill(2);
        TestEqual(TEXT("Second step should not execute with invalid context"), 
                 ExecutionOrder.Num(), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGFutureCombineTest, "OccamsGamekit.OGAsync.Futures.Combine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOGFutureCombineTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!World)
        return false;
    
    AActor* ContextObject = World->SpawnActor<AActor>();
    ON_SCOPE_EXIT{ContextObject->Destroy();};

    // Test 1: FutureAll Basic Completion
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        TArray<FOGFuture> Futures;
        Futures.Add(Promise1);
        Futures.Add(Promise2);
        Futures.Add(Promise3);
        
        bool AllComplete = false;
        auto CombinedFuture = UOGFutureUtilities::FutureAll(ContextObject, Futures);
        CombinedFuture->WeakThen(ContextObject, [&]() {
            AllComplete = true;
        });
        
        Promise1->Fulfill(1);
        TestFalse(TEXT("All futures not complete yet"), AllComplete);
        
        Promise2->Fulfill(2);
        TestFalse(TEXT("All futures not complete yet"), AllComplete);
        
        Promise3->Fulfill(3);
        TestTrue(TEXT("All futures now complete"), AllComplete);
    }

    // Test 2: FutureAll Error Handling
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        TArray<FOGFuture> Futures;
        Futures.Add(Promise1);
        Futures.Add(Promise2);
        Futures.Add(Promise3);
        
        bool CatchExecuted = false;
        FString CaughtReason;
        
        auto CombinedFuture = UOGFutureUtilities::FutureAll(ContextObject, Futures);
        CombinedFuture->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
            CaughtReason = Reason;
        }));
        
        Promise1->Fulfill(1);
        Promise2->Throw(TEXT("Test Error"));
        Promise3->Fulfill(3);
        
        TestTrue(TEXT("Catch should execute when any future fails"), CatchExecuted);
        TestTrue(TEXT("Error message should be preserved"), 
                CaughtReason.Contains(TEXT("Test Error")));
    }

    // Test 3: FutureAny Basic Completion
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        TArray<FOGFuture> Futures;
        Futures.Add(Promise1);
        Futures.Add(Promise2);
        Futures.Add(Promise3);
        
        bool AnyComplete = false;
        auto CombinedFuture = UOGFutureUtilities::FutureAny(ContextObject, Futures);
        CombinedFuture->WeakThen(ContextObject, [&]() {
            AnyComplete = true;
        });
        
        Promise2->Fulfill(2);
        TestTrue(TEXT("Should complete when any future completes"), AnyComplete);
    }

    // Test 4: FutureAny Error Handling
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        TArray<FOGFuture> Futures;
        Futures.Add(Promise1);
        Futures.Add(Promise2);
        Futures.Add(Promise3);
        
        bool CatchExecuted = false;
        
        auto CombinedFuture = UOGFutureUtilities::FutureAny(ContextObject, Futures);
        CombinedFuture->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            CatchExecuted = true;
        }));
        
        Promise1->Throw(TEXT("Error 1"));
        TestFalse(TEXT("FutureAny shouldn't fail with one error"), CatchExecuted);
        
        Promise2->Throw(TEXT("Error 2"));
        TestFalse(TEXT("FutureAny shouldn't fail with two errors"), CatchExecuted);
        
        Promise3->Throw(TEXT("Error 3"));
        TestTrue(TEXT("FutureAny should fail when all futures fail"), CatchExecuted);
    }

    // Test 5: Empty Futures Array
    {
        TArray<FOGFuture> EmptyFutures;
        
        bool AllComplete = false;
        bool AnyComplete = false;
        
        UOGFutureUtilities::FutureAll(ContextObject, EmptyFutures)->WeakThen(ContextObject, [&]() {
            AllComplete = true;
        });
        
        UOGFutureUtilities::FutureAny(ContextObject, EmptyFutures)->WeakThen(ContextObject, [&]() {
            AnyComplete = true;
        });
        
        TestTrue(TEXT("FutureAll should complete immediately with empty array"), AllComplete);
        TestTrue(TEXT("FutureAny should complete immediately with empty array"), AnyComplete);
    }

    // Test 6: Mixed Completion and Errors
    {
        TOGPromise<int> Promise1;
        TOGPromise<int> Promise2;
        TOGPromise<int> Promise3;
        
        TArray<FOGFuture> Futures;
        Futures.Add(Promise1);
        Futures.Add(Promise2);
        Futures.Add(Promise3);
        
        bool AnyCatchExecuted = false;
        bool AllCatchExecuted = false;
        
        UOGFutureUtilities::FutureAny(ContextObject, Futures)->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            AnyCatchExecuted = true;
        }));
        
        UOGFutureUtilities::FutureAll(ContextObject, Futures)->Catch(FOGFutureState::FCatchDelegate::CreateLambda([&](const FString& Reason) {
            AllCatchExecuted = true;
        }));
        
        Promise1->Fulfill(1);
        Promise2->Throw(TEXT("Error"));
        Promise3->Fulfill(3);
        
        TestFalse(TEXT("FutureAny shouldn't fail with mixed results"), AnyCatchExecuted);
        TestTrue(TEXT("FutureAll should fail with any error"), AllCatchExecuted);
    }

    return true;
}