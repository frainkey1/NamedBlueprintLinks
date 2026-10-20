// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NBLTestLibrary.generated.h"

class AActor;

// Editor/uncooked-only fixture functions. Never needed by a packaged game.
UCLASS()
class UNBLTestLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static double Probe(double Value);
    UFUNCTION(BlueprintCallable, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static void Record(int32 Value);
    UFUNCTION(BlueprintPure, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static float ReadFloat();
    UFUNCTION(BlueprintPure, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static FVector ReadVector();
    UFUNCTION(BlueprintPure, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static FRotator ReadRotator();
    UFUNCTION(BlueprintPure, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static AActor* ReadActor();
    UFUNCTION(BlueprintCallable, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static void CaptureFloat(float Value);
    UFUNCTION(BlueprintCallable, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static void CaptureVector(FVector Value);
    UFUNCTION(BlueprintCallable, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static void CaptureRotator(FRotator Value);
    UFUNCTION(BlueprintCallable, Category="NBL Tests", meta=(BlueprintInternalUseOnly="true"))
    static void CaptureActor(AActor* Value);
    static TArray<float> Floats;
    static TArray<FVector> Vectors;
    static TArray<FRotator> Rotators;
    static TArray<AActor*> Actors;
    static void ResetCaptures();
    static TArray<int32> Order;
    static int32 ProbeCalls;
};
