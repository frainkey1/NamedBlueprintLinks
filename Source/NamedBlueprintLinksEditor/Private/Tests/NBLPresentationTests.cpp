// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLPresentation.h"
#include "SNBLGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLExecDeclaration.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "SGraphPin.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLPinPresentation,"NamedBlueprintLinks.Presentation.LocalPinState",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLPinPresentation::RunTest(const FString&)
{
    auto* Graph=NewObject<UEdGraph>(); Graph->Schema=UEdGraphSchema_K2::StaticClass();
    auto* D=NewObject<UK2Node_NBLDataDeclaration>(Graph);
    Graph->AddNode(D,false,false); D->PostPlacedNewNode(); D->AllocateDefaultPins();
    const auto Widget=SNew(SNBLGraphNode,D);
    Widget->SlatePrepass();
    TestTrue(TEXT("Compact widget lays out"),Widget->GetDesiredSize().X>0);
    TestTrue(TEXT("Wildcard clearly darker than execution white"),NBLPresentation::PinColor(*D->GetInputPin()).R<0.5f);
    TestFalse(TEXT("Wildcard input hollow"),NBLPresentation::IsPinFilled(*D->GetInputPin()));
    TestFalse(TEXT("Wildcard output hollow"),NBLPresentation::IsPinFilled(*D->GetOutputPin()));
    auto* Source=NewObject<UK2Node_NBLDataDeclaration>(Graph); Graph->AddNode(Source,false,false); Source->AllocateDefaultPins();
    auto* U=D->CreateUsage();
    if (!TestNotNull(TEXT("Usage exists"),U)) return false;
    TestFalse(TEXT("Hidden Usage association leaves Declaration output hollow"),NBLPresentation::IsPinFilled(*D->GetOutputPin()));
    D->GetInputPin()->PinType.PinCategory=UEdGraphSchema_K2::PC_Boolean;
    Source->GetOutputPin()->MakeLinkTo(D->GetInputPin());
    TestTrue(TEXT("Wired input filled"),NBLPresentation::IsPinFilled(*D->GetInputPin()));
    const auto NativePin=SNew(SGraphPin,D->GetInputPin());
    TestTrue(TEXT("Native widget and helper agree"),NativePin->IsConnected());
    TestTrue(TEXT("Resolved pin uses schema color"),NBLPresentation::PinColor(*D->GetInputPin())==Graph->GetSchema()->GetPinTypeColor(D->GetInputPin()->PinType));
    D->GetOutputPin()->MakeLinkTo(Source->GetInputPin());
    TestTrue(TEXT("Wired local output filled"),NBLPresentation::IsPinFilled(*D->GetOutputPin()));
    D->GetOutputPin()->BreakAllPinLinks();
    TestFalse(TEXT("Unwired local output hollow again"),NBLPresentation::IsPinFilled(*D->GetOutputPin()));
    U->GetOutputPin()->MakeLinkTo(Source->GetInputPin());
    TestTrue(TEXT("Wired Usage output filled"),NBLPresentation::IsPinFilled(*U->GetOutputPin()));
    TestFalse(TEXT("Usage wiring still leaves Declaration output hollow"),NBLPresentation::IsPinFilled(*D->GetOutputPin()));
    auto* E=NewObject<UK2Node_NBLExecDeclaration>(Graph); E->AllocateDefaultPins();
    TestTrue(TEXT("Execution geometry category preserved"),E->GetInputPin()->PinType.PinCategory==UEdGraphSchema_K2::PC_Exec);
    TestFalse(TEXT("Unwired execution input hollow"),NBLPresentation::IsPinFilled(*E->GetInputPin()));
    return true;
}
#endif
