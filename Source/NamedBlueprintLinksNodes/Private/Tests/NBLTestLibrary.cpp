// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestLibrary.h"
#include "GameFramework/Actor.h"

TArray<float> UNBLTestLibrary::Floats;
TArray<FVector> UNBLTestLibrary::Vectors;
TArray<FRotator> UNBLTestLibrary::Rotators;
TArray<AActor*> UNBLTestLibrary::Actors;
float UNBLTestLibrary::ReadFloat() { ++ProbeCalls; return 2.0f; }
FVector UNBLTestLibrary::ReadVector() { ++ProbeCalls; return FVector(100,100,1); }
FRotator UNBLTestLibrary::ReadRotator() { ++ProbeCalls; return FRotator(0,-45,0); }
AActor* UNBLTestLibrary::ReadActor() { ++ProbeCalls; return GetMutableDefault<AActor>(); }
void UNBLTestLibrary::CaptureFloat(float Value) { Floats.Add(Value); }
void UNBLTestLibrary::CaptureVector(FVector Value) { Vectors.Add(Value); }
void UNBLTestLibrary::CaptureRotator(FRotator Value) { Rotators.Add(Value); }
void UNBLTestLibrary::CaptureActor(AActor* Value) { Actors.Add(Value); }
void UNBLTestLibrary::ResetCaptures()
{ Floats.Reset(); Vectors.Reset(); Rotators.Reset(); Actors.Reset(); ProbeCalls=0; }
