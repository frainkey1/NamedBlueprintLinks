// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestFixture.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLExecDeclaration.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLRenameValidator,"NamedBlueprintLinks.Authoring.EditorRenameValidator",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLRenameValidator::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    auto* E=NBLTests::Add<UK2Node_NBLExecDeclaration>(F.Graph);
    auto* Other=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    FText Error; Other->RenameLink(TEXT("TakenName"),Error);
    for (UK2Node_NBLBase* Node : TArray<UK2Node_NBLBase*>{D,E})
    {
        // Guard before invoking the same factory used by the Blueprint editor:
        // old code returns null here; the factory check would crash the editor.
        auto Validator=Node->MakeNameValidator();
        if (!TestTrue(TEXT("Renamable endpoint supplies validator"),Validator.IsValid())) continue;
        auto EditorValidator=FNameValidatorFactory::MakeValidator(Node);
        TestTrue(TEXT("Editor factory accepts endpoint"),EditorValidator.IsValid());
        TestTrue(TEXT("New name accepted"),EditorValidator->IsValid(FString(TEXT("CopiedExec")))==EValidatorResult::Ok);
        TestTrue(TEXT("Empty name rejected"),EditorValidator->IsValid(FString(TEXT("   ")))==EValidatorResult::EmptyName);
        TestTrue(TEXT("Original name recognized"),EditorValidator->IsValid(Node->LinkName)==EValidatorResult::ExistingName);
        TestTrue(TEXT("Category scoped duplicates"),EditorValidator->IsValid(FName(TEXT("TakenName")))==
            (Node->IsExecutionLink() ? EValidatorResult::Ok : EValidatorResult::AlreadyInUse));
        auto* Usage=Node->CreateUsage();
        Node->OnRenameNode(TEXT("RenamedLink"));
        TestTrue(TEXT("Editor commit updates linked Usage"),Usage && Usage->LinkName==Node->LinkName && Node->LinkName==TEXT("RenamedLink"));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLUniversalExec,"NamedBlueprintLinks.Types.UniversalExecutionRules",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLUniversalExec::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph); auto* U=D->CreateUsage();
    const auto* Schema=GetDefault<UEdGraphSchema_K2>();
    auto* Source=F.Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then);
    auto* Consumer=F.Return->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
    TestTrue(TEXT("Universal input accepts execution"),Schema->TryCreateConnection(Source,D->GetInputPin()));
    TestTrue(TEXT("Declaration and Usage resolve to execution"),D->IsExecutionLink() && U->IsExecutionLink());
    TestTrue(TEXT("Local output hidden in execution mode"),D->GetOutputPin()->bHidden);
    TestFalse(TEXT("Additional Usage creation disabled"),D->CanCreateUsage());
    TestNull(TEXT("Additional Usage cannot be created"),D->CreateUsage());
    TestFalse(TEXT("Local execution passthrough rejected"),Schema->TryCreateConnection(D->GetOutputPin(),Consumer));
    TestTrue(TEXT("Execution Usage connects"),Schema->TryCreateConnection(U->GetOutputPin(),Consumer));
    Source->BreakAllPinLinks(); Consumer->BreakAllPinLinks(); D->RefreshDataType();
    TestFalse(TEXT("Disconnect resets execution mode"),D->IsExecutionLink() || U->IsExecutionLink());
    TestFalse(TEXT("Local output returns when unresolved"),D->GetOutputPin()->bHidden);
    TestTrue(TEXT("Destination-first execution inference"),Schema->TryCreateConnection(U->GetOutputPin(),Consumer));
    TestTrue(TEXT("Destination-first propagates mode"),D->IsExecutionLink() && U->IsExecutionLink());
    Consumer->BreakAllPinLinks(); D->RefreshDataType();
    auto* Extra=D->CreateUsage();
    if (!TestNotNull(TEXT("Wildcard permits extra data Usage"),Extra)) return false;
    TestFalse(TEXT("Existing multiple Usages block execution source"),Schema->TryCreateConnection(Source,D->GetInputPin()));
    TestFalse(TEXT("Existing multiple Usages block execution consumer"),Schema->TryCreateConnection(U->GetOutputPin(),Consumer));
    TestFalse(TEXT("Rejected conversion stays unresolved"),D->IsExecutionLink());
    return true;
}
#endif
