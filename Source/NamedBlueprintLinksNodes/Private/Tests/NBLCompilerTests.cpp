// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestLibrary.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLDataUsage.h"
#include "K2Node_NBLExecDeclaration.h"
#include "K2Node_NBLExecUsage.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "NBLLinkResolver.h"
#include "KismetCompiler.h"
#include "Logging/TokenizedMessage.h"
#include "Kismet2/CompilerResultsLog.h"

TArray<int32> UNBLTestLibrary::Order;
int32 UNBLTestLibrary::ProbeCalls = 0;
double UNBLTestLibrary::Probe(double Value) { ++ProbeCalls; return Value; }
void UNBLTestLibrary::Record(int32 Value) { Order.Add(Value); }

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FSourcePinState
{
    UEdGraphPin* Pin;
    FEdGraphPinType Type;
    TArray<UEdGraphPin*> Links;
    bool operator==(const FSourcePinState& Other) const
    { return Pin == Other.Pin && Type == Other.Type && Links == Other.Links; }
};
struct FSourceNodeState
{
    UEdGraphNode* Node;
    FGuid Guid;
    FGuid Declaration;
    FGuid Target;
    FName Name;
    bool Resolved = false;
    TArray<FSourcePinState> Pins;
    bool operator==(const FSourceNodeState& Other) const
    {
        return Node == Other.Node && Guid == Other.Guid && Declaration == Other.Declaration &&
            Target == Other.Target && Name == Other.Name && Resolved == Other.Resolved && Pins == Other.Pins;
    }
};
TArray<FSourceNodeState> SnapshotSource(const UEdGraph& Graph)
{
    TArray<FSourceNodeState> Result;
    for (UEdGraphNode* Node : Graph.Nodes)
    {
        FSourceNodeState State;
        State.Node = Node; State.Guid = Node->NodeGuid;
        if (auto* Link = Cast<UK2Node_NBLBase>(Node))
        {
            State.Declaration = Link->DeclarationId; State.Target = Link->TargetDeclarationId;
            State.Name = Link->LinkName; State.Resolved = Link->bCompilerResolved;
        }
        for (auto* Pin : Node->Pins) State.Pins.Add({Pin,Pin->PinType,Pin->LinkedTo});
        Result.Add(MoveTemp(State));
    }
    return Result;
}
template<class T> T* Add(UEdGraph* Graph)
{
    T* Node = NewObject<T>(Graph);
    Graph->AddNode(Node, false, false);
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->AllocateDefaultPins();
    return Node;
}
UK2Node_CallFunction* Call(UEdGraph* Graph, FName Name)
{
    auto* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(UNBLTestLibrary::StaticClass()->FindFunctionByName(Name));
    Graph->AddNode(Node, false, false); Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}
bool CompareFixture(FAutomationTestBase& Test, bool Routed, bool UsageFirst, bool UniversalExec=false)
{
    const FName Name = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NBLFixture"));
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UObject::StaticClass(), GetTransientPackage(),
        Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    if (!Test.TestNotNull(TEXT("Blueprint created"), BP)) return false;
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("Evaluate"),
        UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(BP, Graph, true, nullptr);
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes) if (auto* E = Cast<UK2Node_FunctionEntry>(Node)) Entry = E;
    if (!Test.TestNotNull(TEXT("Function entry"), Entry)) return false;
    UK2Node_FunctionResult* Return = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes) if (auto* R = Cast<UK2Node_FunctionResult>(Node)) Return = R;
    if (!Return) Return = Add<UK2Node_FunctionResult>(Graph);
    FEdGraphPinType Real; Real.PinCategory = UEdGraphSchema_K2::PC_Real;
    Real.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    Return->CreateUserDefinedPin(TEXT("Result"), Real, EGPD_Input);
    auto* Probe = Call(Graph, TEXT("Probe")); Probe->FindPinChecked(TEXT("Value"))->DefaultValue = TEXT("2.0");
    auto* First = Call(Graph, TEXT("Record")); First->FindPinChecked(TEXT("Value"))->DefaultValue = TEXT("1");
    auto* Second = Call(Graph, TEXT("Record")); Second->FindPinChecked(TEXT("Value"))->DefaultValue = TEXT("2");
    auto* Third = Call(Graph, TEXT("Record")); Third->FindPinChecked(TEXT("Value"))->DefaultValue = TEXT("3");
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    bool Connected = true;
    auto Wire = [&](UEdGraphPin* A, UEdGraphPin* B) { Connected &= Schema->TryCreateConnection(A,B); };
    Wire(Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), First->GetExecPin());
    Wire(Second->GetThenPin(), Third->GetExecPin()); Wire(Third->GetThenPin(), Return->GetExecPin());
    if (Routed)
    {
        UK2Node_NBLDataDeclaration* D = nullptr; UK2Node_NBLDataUsage* U = nullptr;
        UK2Node_NBLBase* ED = nullptr; UK2Node_NBLBase* EU = nullptr;
        auto MakeED = [&]() -> UK2Node_NBLBase* { return UniversalExec ? static_cast<UK2Node_NBLBase*>(Add<UK2Node_NBLDataDeclaration>(Graph)) : Add<UK2Node_NBLExecDeclaration>(Graph); };
        auto MakeEU = [&]() -> UK2Node_NBLBase* { return UniversalExec ? static_cast<UK2Node_NBLBase*>(Add<UK2Node_NBLDataUsage>(Graph)) : Add<UK2Node_NBLExecUsage>(Graph); };
        if (UsageFirst) { U=Add<UK2Node_NBLDataUsage>(Graph); EU=MakeEU(); }
        D=Add<UK2Node_NBLDataDeclaration>(Graph); ED=MakeED();
        if (!UsageFirst) { U=Add<UK2Node_NBLDataUsage>(Graph); EU=MakeEU(); }
        U->TargetDeclarationId = D->DeclarationId; EU->TargetDeclarationId = ED->DeclarationId;
        Wire(Probe->GetReturnValuePin(), D->GetInputPin()); U->RefreshDataType();
        Wire(U->GetOutputPin(), Return->FindPinChecked(TEXT("Result")));
        Wire(First->GetThenPin(), ED->GetInputPin()); Wire(EU->GetOutputPin(), Second->GetExecPin());
    }
    else
    {
        Wire(Probe->GetReturnValuePin(), Return->FindPinChecked(TEXT("Result")));
        Wire(First->GetThenPin(), Second->GetExecPin());
    }
    if (!Test.TestTrue(TEXT("Fixture connections accepted"), Connected)) return false;
    const auto SourceState = SnapshotSource(*Graph);
    // Regression for the review finding: a same-degree rewire must be detected.
    auto* FirstOut = First->GetThenPin();
    auto* OriginalNext = FirstOut->LinkedTo[0];
    FirstOut->BreakLinkTo(OriginalNext);
    FirstOut->MakeLinkTo(Return->GetExecPin());
    Test.TestTrue(TEXT("Snapshot detects changed endpoints, not just link counts"), SnapshotSource(*Graph) != SourceState);
    FirstOut->BreakLinkTo(Return->GetExecPin()); FirstOut->MakeLinkTo(OriginalNext);
    if (Routed)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Link = Cast<UK2Node_NBLBase>(Node))
            {
                const FGuid Original = Link->DeclarationId;
                Link->DeclarationId = FGuid::NewGuid();
                Test.TestTrue(TEXT("Snapshot detects identity mutation"), SnapshotSource(*Graph) != SourceState);
                Link->DeclarationId = Original;
                break;
            }
    }
    Test.TestTrue(TEXT("Fixture restored before compilation"), SnapshotSource(*Graph) == SourceState);
    for (int32 Iteration=0; Iteration<2; ++Iteration)
    {
        FKismetEditorUtilities::CompileBlueprint(BP);
        if (!Test.TestTrue(TEXT("Blueprint compiles"), BP->Status != BS_Error)) return false;
        Test.TestTrue(TEXT("Source graph, identities, types and exact links unchanged by compilation"),
            SnapshotSource(*Graph) == SourceState);
        UObject* Object = NewObject<UObject>(GetTransientPackage(), BP->GeneratedClass);
        UFunction* Function = Object->FindFunction(TEXT("Evaluate"));
        if (!Test.TestNotNull(TEXT("Compiled function"), Function)) return false;
        FStructOnScope Params(Function);
        UNBLTestLibrary::Order.Reset(); UNBLTestLibrary::ProbeCalls = 0;
        Object->ProcessEvent(Function, Params.GetStructMemory());
        auto* Property = FindFProperty<FDoubleProperty>(Function, TEXT("Result"));
        if (!Test.TestNotNull(TEXT("Result property"), Property)) return false;
        Test.TestEqual(TEXT("Original source value reaches result"), Property->GetPropertyValue_InContainer(Params.GetStructMemory()), 2.0);
        Test.TestTrue(TEXT("Execution is exactly 1,2,3"), UNBLTestLibrary::Order == TArray<int32>({1,2,3}));
        Test.TestEqual(TEXT("Single pure consumer evaluated once"), UNBLTestLibrary::ProbeCalls, 1);
    }
    return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLCompilerRoutes, "NamedBlueprintLinks.Compiler.RoutesAndPruning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNBLCompilerRoutes::RunTest(const FString&)
{
    return CompareFixture(*this,false,false) && CompareFixture(*this,true,false) && CompareFixture(*this,true,true) && CompareFixture(*this,true,false,true) && CompareFixture(*this,true,true,true);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLCompilerFailureCleanup, "NamedBlueprintLinks.Compiler.FailureCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNBLCompilerFailureCleanup::RunTest(const FString&)
{
    const FName Name = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NBLFailure"));
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UObject::StaticClass(), GetTransientPackage(),
        Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("Evaluate"),
        UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(BP, Graph, true, nullptr);
    auto* D = Add<UK2Node_NBLDataDeclaration>(Graph);
    D->LinkName = TEXT("MissingSourceTest");
    auto* U = D->CreateUsage();
    auto* Consumer = Call(Graph, TEXT("Probe"));
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Return = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* E = Cast<UK2Node_FunctionEntry>(Node)) Entry = E;
        if (auto* R = Cast<UK2Node_FunctionResult>(Node)) Return = R;
    }
    if (!Return) Return = Add<UK2Node_FunctionResult>(Graph);
    FEdGraphPinType Real;
    Real.PinCategory = UEdGraphSchema_K2::PC_Real;
    Real.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    Return->CreateUserDefinedPin(TEXT("Result"), Real, EGPD_Input);
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    if (!TestNotNull(TEXT("Function entry exists"), Entry)) return false;
    Schema->TryCreateConnection(Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), Return->GetExecPin());
    Schema->TryCreateConnection(Consumer->GetReturnValuePin(), Return->FindPinChecked(TEXT("Result")));
    if (!TestTrue(TEXT("Consumed link without source constructed"),
        U && Schema->TryCreateConnection(U->GetOutputPin(), Consumer->FindPinChecked(TEXT("Value"))))) return false;
    // Unrelated execution and unresolved data endpoints must not receive blame.
    Add<UK2Node_NBLExecDeclaration>(Graph)->CreateUsage();
    Add<UK2Node_NBLDataDeclaration>(Graph)->CreateUsage();
    const auto Before = SnapshotSource(*Graph);
    UEdGraph* Intermediate = DuplicateObject<UEdGraph>(Graph, BP,
        MakeUniqueObjectName(BP, UEdGraph::StaticClass(), TEXT("CompilerCopy")));
    TArray<UK2Node_NBLBase*> Endpoints;
    for (UEdGraphNode* Node : Intermediate->Nodes)
        if (auto* Link = Cast<UK2Node_NBLBase>(Node)) Endpoints.Add(Link);
    FCompilerResultsLog Results;
    Results.bSilentMode = true;
    FKismetCompilerOptions Options;
    {
        FKismetCompilerContext Context(BP, Results, Options);
        // Match the compiler invoking ExpandNode on every endpoint after a failure.
        for (auto* Link : Endpoints) Link->ExpandNode(Context, Intermediate);
    }
    TestEqual(TEXT("One root failure is reported once across all endpoint visits"), Results.NumErrors, 1);
    if (!Results.Messages.IsEmpty())
        TestTrue(TEXT("Diagnostic identifies the user-named offending link"),
            Results.Messages[0]->ToText().ToString().Contains(TEXT("MissingSourceTest")));
    for (auto* Link : Endpoints)
    {
        TestTrue(TEXT("Failed intermediate endpoints cannot reach backend as unexpected nodes"),
            Link->bCompilerResolved && Link->IsNodeSafeToIgnore());
        for (auto* Pin : Link->Pins)
            TestTrue(TEXT("Failed intermediate routing is disconnected"), Pin->LinkedTo.IsEmpty());
    }
    TestTrue(TEXT("Failure cleanup leaves the entire asset graph unchanged"), SnapshotSource(*Graph) == Before);
    // Exercise the real compiler too: no backend cascades or wildcard errors,
    // and a failed compile must not poison the next compile after repair.
    FCompilerResultsLog FailedCompile;
    FailedCompile.bSilentMode = true;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &FailedCompile);
    TestTrue(TEXT("Invalid source graph remains a compile failure"), BP->Status == BS_Error);
    TestEqual(TEXT("Full compiler reports exactly one root error"), FailedCompile.NumErrors, 1);
    TestTrue(TEXT("Full failed compile preserves source graph"), SnapshotSource(*Graph) == Before);
    auto* Source = Call(Graph, TEXT("Probe"));
    Source->FindPinChecked(TEXT("Value"))->DefaultValue = TEXT("7.0");
    TestTrue(TEXT("Source can be restored"), Schema->TryCreateConnection(Source->GetReturnValuePin(), D->GetInputPin()));
    FCompilerResultsLog RecoveredCompile;
    RecoveredCompile.bSilentMode = true;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &RecoveredCompile);
    TestEqual(TEXT("Repaired compile has no errors"), RecoveredCompile.NumErrors, 0);
    TestTrue(TEXT("Repaired Blueprint compiles successfully"), BP->Status != BS_Error);
    return true;
}
#endif
