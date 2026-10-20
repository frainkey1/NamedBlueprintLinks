// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestFixture.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLDataUsage.h"
#include "K2Node_Knot.h"
#include "K2Node_Self.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FOutcome
{
    int32 Evaluations=0;
    TArray<float> Floats;
    TArray<FVector> Vectors;
    TArray<FRotator> Rotators;
    TArray<AActor*> Actors;
};
bool RunTyped(FAutomationTestBase& Test, const FName Read, const FName Capture, bool Routed, FOutcome& Outcome)
{
    NBLTests::FFixture F;
    if (!Test.TestNotNull(TEXT("Function entry exists"),F.Entry)) return false;
    auto* Source=NBLTests::Call(F.Graph,Read);
    const auto* Schema=GetDefault<UEdGraphSchema_K2>();
    TArray<UEdGraphPin*> Values;
    if (Routed)
    {
        auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
        auto* U=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph);
        auto* V=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph);
        U->TargetDeclarationId=D->DeclarationId; V->TargetDeclarationId=D->DeclarationId;
        if (!Test.TestTrue(TEXT("Source connects to Declaration"),Schema->TryCreateConnection(Source->GetReturnValuePin(),D->GetInputPin()))) return false;
        U->RefreshDataType(); V->RefreshDataType();
        const auto Type=Source->GetReturnValuePin()->PinType;
        Test.TestTrue(TEXT("Declaration exact source type"),D->GetOutputPin()->PinType==Type);
        Test.TestTrue(TEXT("Both Usages exact source type"),U->GetOutputPin()->PinType==Type && V->GetOutputPin()->PinType==Type);
        Values={D->GetOutputPin(),U->GetOutputPin(),V->GetOutputPin()};
    }
    else Values={Source->GetReturnValuePin(),Source->GetReturnValuePin(),Source->GetReturnValuePin()};
    UEdGraphPin* Previous=F.Entry->GetThenPin();
    for (auto* Value : Values)
    {
        auto* Sink=NBLTests::Call(F.Graph,Capture);
        if (!Test.TestTrue(TEXT("Data consumer connects"),Schema->TryCreateConnection(Value,Sink->FindPinChecked(TEXT("Value"))))) return false;
        if (!Test.TestTrue(TEXT("Execution consumer connects"),Schema->TryCreateConnection(Previous,Sink->GetExecPin()))) return false;
        Previous=Sink->GetThenPin();
    }
    if (!Test.TestTrue(TEXT("Return connects"),Schema->TryCreateConnection(Previous,F.Return->GetExecPin()))) return false;
    FKismetEditorUtilities::CompileBlueprint(F.BP);
    if (!Test.TestTrue(TEXT("Typed graph compiles"),F.BP->Status!=BS_Error)) return false;
    UObject* Object=NewObject<UObject>(GetTransientPackage(),F.BP->GeneratedClass);
    UFunction* Function=Object->FindFunction(TEXT("Evaluate"));
    if (!Test.TestNotNull(TEXT("Typed Evaluate function"),Function)) return false;
    UNBLTestLibrary::ResetCaptures(); Object->ProcessEvent(Function,nullptr);
    Outcome={UNBLTestLibrary::ProbeCalls,UNBLTestLibrary::Floats,UNBLTestLibrary::Vectors,
        UNBLTestLibrary::Rotators,UNBLTestLibrary::Actors};
    return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLTypedValues,"NamedBlueprintLinks.Types.ValuesAndPureEvaluation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLTypedValues::RunTest(const FString&)
{
    for (const FString Type : {TEXT("Float"),TEXT("Vector"),TEXT("Rotator"),TEXT("Actor")})
    {
        const FName Read(*(FString(TEXT("Read"))+Type));
        const FName Capture(*(FString(TEXT("Capture"))+Type));
        FOutcome Direct,Routed;
        if (!RunTyped(*this,Read,Capture,false,Direct) || !RunTyped(*this,Read,Capture,true,Routed)) return false;
        TestEqual(TEXT("Pure-source evaluation count matches direct wires"),Routed.Evaluations,Direct.Evaluations);
        TestTrue(TEXT("Three consumer values match baseline"),Routed.Floats==Direct.Floats && Routed.Vectors==Direct.Vectors &&
            Routed.Rotators==Direct.Rotators && Routed.Actors==Direct.Actors);
        if (Type==TEXT("Float")) TestTrue(TEXT("Three exact float values"),Routed.Floats==TArray<float>({2,2,2}));
        if (Type==TEXT("Vector")) TestTrue(TEXT("Three exact vector values"),Routed.Vectors==TArray<FVector>({{100,100,1},{100,100,1},{100,100,1}}));
        if (Type==TEXT("Rotator")) TestTrue(TEXT("Three exact rotator values"),Routed.Rotators==TArray<FRotator>({{0,-45,0},{0,-45,0},{0,-45,0}}));
        if (Type==TEXT("Actor")) TestTrue(TEXT("Three identical Actor references"),Routed.Actors==TArray<AActor*>({GetMutableDefault<AActor>(),GetMutableDefault<AActor>(),GetMutableDefault<AActor>()}));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLTypeConstraints,"NamedBlueprintLinks.Types.InferenceAndDisconnect",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLTypeConstraints::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    auto* U=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph); U->TargetDeclarationId=D->DeclarationId;
    auto* Sink=NBLTests::Call(F.Graph,TEXT("CaptureVector"));
    const auto* Schema=GetDefault<UEdGraphSchema_K2>();
    TestTrue(TEXT("Initially all pins wildcard"),D->GetInputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard &&
        D->GetOutputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard && U->GetOutputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard);
    TestTrue(TEXT("Destination-first wire accepted"),Schema->TryCreateConnection(U->GetOutputPin(),Sink->FindPinChecked(TEXT("Value"))));
    TestTrue(TEXT("Consumer infers Declaration type"),D->GetInputPin()->PinType==Sink->FindPinChecked(TEXT("Value"))->PinType);
    auto* Source=NBLTests::Call(F.Graph,TEXT("ReadVector"));
    TestTrue(TEXT("Compatible source accepted"),Schema->TryCreateConnection(Source->GetReturnValuePin(),D->GetInputPin()));
    auto* Wrong=NBLTests::Call(F.Graph,TEXT("ReadActor"));
    TestFalse(TEXT("Incompatible source rejected without a cast"),Schema->TryCreateConnection(Wrong->GetReturnValuePin(),D->GetInputPin()));
    Schema->BreakPinLinks(*D->GetInputPin(),true);
    TestTrue(TEXT("Remaining consumer retains vector constraint"),D->GetInputPin()->PinType==Sink->FindPinChecked(TEXT("Value"))->PinType);
    Schema->BreakPinLinks(*U->GetOutputPin(),true);
    TestTrue(TEXT("No remaining constraints returns Declaration and Usage to wildcard"),
        D->GetInputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard &&
        D->GetOutputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard &&
        U->GetOutputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLTypeQualifiers,"NamedBlueprintLinks.Types.CompletePinType",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLTypeQualifiers::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    auto* U=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph); U->TargetDeclarationId=D->DeclarationId;
    auto* Source=NBLTests::Add<UK2Node_Knot>(F.Graph);
    for (const auto Container : {EPinContainerType::Array,EPinContainerType::Set,EPinContainerType::Map})
    {
        FEdGraphPinType Type;
        Type.PinCategory=UEdGraphSchema_K2::PC_Name; Type.ContainerType=Container;
        Type.bIsReference=true; Type.bIsConst=true;
        if (Container==EPinContainerType::Map)
        { Type.PinValueType.TerminalCategory=UEdGraphSchema_K2::PC_Object; Type.PinValueType.TerminalSubCategoryObject=AActor::StaticClass(); }
        Source->GetOutputPin()->PinType=Type;
        Source->GetOutputPin()->MakeLinkTo(D->GetInputPin()); D->RefreshDataType();
        TestTrue(TEXT("All pin qualifiers propagate unchanged"),D->GetInputPin()->PinType==Type &&
            D->GetOutputPin()->PinType==Type && U->GetOutputPin()->PinType==Type);
        TestTrue(TEXT("Input wildcard permits containers"),D->DoesWildcardPinAcceptContainer(D->GetInputPin()));
        TestTrue(TEXT("Output wildcard permits containers"),U->DoesWildcardPinAcceptContainer(U->GetOutputPin()));
        Source->GetOutputPin()->BreakAllPinLinks();
    }
    // Qualifier propagation is tested here; container runtime semantics are not claimed.
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLTypeVisualNotification,"NamedBlueprintLinks.Types.VisualRefreshNotification",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLTypeVisualNotification::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    auto* U=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph); U->TargetDeclarationId=D->DeclarationId;
    auto* Source=NBLTests::Add<UK2Node_Knot>(F.Graph);
    int32 Notifications=0;
    const FDelegateHandle Handle=F.Graph->AddOnGraphChangedHandler(
        FOnGraphChanged::FDelegate::CreateLambda([&](const FEdGraphEditAction&) { ++Notifications; }));
    Source->GetOutputPin()->MakeLinkTo(D->GetInputPin());
    for (const FName Category : {UEdGraphSchema_K2::PC_Exec,UEdGraphSchema_K2::PC_Boolean,UEdGraphSchema_K2::PC_Wildcard})
    {
        Source->GetOutputPin()->PinType.PinCategory=Category;
        const int32 Before=Notifications;
        D->RefreshDataType();
        TestTrue(TEXT("Type change notifies graph widgets without compiling"),Notifications>Before);
        TestTrue(TEXT("Usage adopts source category"),U->GetOutputPin()->PinType.PinCategory==Category);
        TestEqual(TEXT("Local output hidden only for execution"),bool(D->GetOutputPin()->bHidden),Category==UEdGraphSchema_K2::PC_Exec);
        const int32 After=Notifications;
        D->RefreshDataType();
        TestEqual(TEXT("Unchanged type does not repeatedly notify"),Notifications,After);
    }
    F.Graph->RemoveOnGraphChangedHandler(Handle);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLSelfReference,"NamedBlueprintLinks.Types.SelfReference",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLSelfReference::RunTest(const FString&)
{
    // Removing Blueprint class context from either production compatibility
    // check must fail these real Self-to-Object routes, not just color checks.
    for (int32 Route=0; Route<3; ++Route)
    {
        NBLTests::FFixture F;
        auto* Self=NBLTests::Add<UK2Node_Self>(F.Graph);
        auto* Name=NewObject<UK2Node_CallFunction>(F.Graph);
        Name->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("GetDisplayName")));
        F.Graph->AddNode(Name,false,false); Name->CreateNewGuid(); Name->AllocateDefaultPins();
        const auto* Schema=GetDefault<UEdGraphSchema_K2>();
        UEdGraphPin* Value=Self->FindPinChecked(UEdGraphSchema_K2::PN_Self);
        if (Route>0)
        {
            auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
            auto* U=NBLTests::Add<UK2Node_NBLDataUsage>(F.Graph);
            U->TargetDeclarationId=D->DeclarationId;
            if (!TestTrue(TEXT("Self connects to Declaration"),Schema->TryCreateConnection(Value,D->GetInputPin()))) return false;
            Value=Route==1 ? D->GetOutputPin() : U->GetOutputPin();
            if (!TestTrue(TEXT("Self route connects to Object consumer"),Schema->TryCreateConnection(Value,Name->FindPinChecked(TEXT("Object"))))) return false;
            D->RefreshDataType();
            TestTrue(TEXT("Self subcategory survives downstream Object constraint"),
                D->GetOutputPin()->PinType.PinSubCategory==UEdGraphSchema_K2::PSC_Self &&
                U->GetOutputPin()->PinType==D->GetOutputPin()->PinType);
        }
        else if (!TestTrue(TEXT("Direct Self baseline connects"),Schema->TryCreateConnection(Value,Name->FindPinChecked(TEXT("Object"))))) return false;
        FEdGraphPinType StringType; StringType.PinCategory=UEdGraphSchema_K2::PC_String;
        F.Return->CreateUserDefinedPin(TEXT("Result"),StringType,EGPD_Input);
        if (!TestTrue(TEXT("Display name returned"),Schema->TryCreateConnection(Name->GetReturnValuePin(),F.Return->FindPinChecked(TEXT("Result"))))) return false;
        if (!TestTrue(TEXT("Function execution returns"),Schema->TryCreateConnection(F.Entry->GetThenPin(),F.Return->GetExecPin()))) return false;
        FKismetEditorUtilities::CompileBlueprint(F.BP);
        if (!TestTrue(FString::Printf(TEXT("Self route %d compiles"),Route),F.BP->Status!=BS_Error)) return false;
        UObject* Object=NewObject<UObject>(GetTransientPackage(),F.BP->GeneratedClass);
        UFunction* Function=Object->FindFunction(TEXT("Evaluate"));
        if (!TestNotNull(TEXT("Self Evaluate function exists"),Function)) return false;
        struct { FString Result; } Params;
        Object->ProcessEvent(Function,&Params);
        TestEqual(TEXT("Routed display name equals direct native result"),Params.Result,UKismetSystemLibrary::GetDisplayName(Object));
    }
    return true;
}
#endif
