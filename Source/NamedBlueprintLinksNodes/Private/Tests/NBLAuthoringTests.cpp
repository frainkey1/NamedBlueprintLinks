// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestFixture.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLExecDeclaration.h"
#include "NBLLinkResolver.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLAuthoringPairs,"NamedBlueprintLinks.Authoring.CreateRenameAndNavigate",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLAuthoringPairs::RunTest(const FString&)
{
    NBLTests::FFixture F;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    auto* Other=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph);
    TestTrue(TEXT("New Declarations have unique default names"),D->LinkName!=Other->LinkName);
    auto* U=D->CreateUsage(); auto* V=D->CreateUsage();
    if (!TestNotNull(TEXT("First data Usage created"),U) || !TestNotNull(TEXT("Second data Usage created"),V)) return false;
    TestTrue(TEXT("Usage resolves by identity"),NBLLinkResolver::FindDeclaration(*U)==D);
    const FGuid Identity=D->DeclarationId;
    FText Error;
    TestTrue(TEXT("Declaration can be renamed"),D->RenameLink(TEXT("MoonOrbitPosition"),Error));
    TestEqual(TEXT("Usage follows declaration name"),U->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),FString(TEXT("MoonOrbitPosition")));
    TestTrue(TEXT("Rename preserves identity"),D->DeclarationId==Identity && U->TargetDeclarationId==Identity);
    TestTrue(TEXT("Usage navigates to Declaration"),U->GetJumpTargetForDoubleClick()==D && U->CanJumpToDefinition());
    TestFalse(TEXT("Duplicate data name rejected"),Other->RenameLink(TEXT("MoonOrbitPosition"),Error));
    TestFalse(TEXT("Empty name rejected"),D->RenameLink(NAME_None,Error));
    auto* ED=NBLTests::Add<UK2Node_NBLExecDeclaration>(F.Graph);
    TestTrue(TEXT("Exec category may use same name"),ED->RenameLink(TEXT("MoonOrbitPosition"),Error));
    TestNotNull(TEXT("One exec Usage created"),ED->CreateUsage());
    TestFalse(TEXT("Second exec Usage action disabled"),ED->CanCreateUsage());
    TestNull(TEXT("Second exec Usage cannot be created"),ED->CreateUsage());
    F.Graph->RemoveNode(D);
    TestNull(TEXT("Deleted Declaration leaves unresolved Usage"),NBLLinkResolver::FindDeclaration(*U));
    TestFalse(TEXT("Unresolved Usage cannot navigate"),U->CanJumpToDefinition());
    return true;
}
#endif
