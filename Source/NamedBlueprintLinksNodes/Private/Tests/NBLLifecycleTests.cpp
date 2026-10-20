// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTestFixture.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLExecDeclaration.h"
#include "NBLLinkResolver.h"
#include "EdGraphUtilities.h"
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "ScopedTransaction.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString CopyLinks(const TArray<UK2Node_NBLBase*>& Nodes)
{
    TSet<UObject*> Selection;
    for (auto* Node : Nodes) { Node->PrepareForCopying(); Selection.Add(Node); }
    FString Text; FEdGraphUtilities::ExportNodesToText(Selection,Text); return Text;
}
TArray<UK2Node_NBLBase*> PasteLinks(UEdGraph* Graph,const FString& Text)
{
    TSet<UEdGraphNode*> Imported; FEdGraphUtilities::ImportNodesFromText(Graph,Text,Imported);
    TArray<UK2Node_NBLBase*> Result;
    for (auto* Node : Imported) { Node->CreateNewGuid(); if (auto* Link=Cast<UK2Node_NBLBase>(Node)) Result.Add(Link); }
    return Result;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLCopyPairs,"NamedBlueprintLinks.Lifecycle.CopyPasteIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLCopyPairs::RunTest(const FString&)
{
    NBLTests::FFixture F; NBLTests::FFixture Other;
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph); auto* U=D->CreateUsage(); auto* V=D->CreateUsage();
    TestTrue(TEXT("Copy enabled"),D->CanDuplicateNode() && U->CanDuplicateNode());
    const FGuid Original=D->DeclarationId; const FString Text=CopyLinks({D,U,V});
    for (int32 Repeat=0; Repeat<2; ++Repeat)
    {
        auto Pasted=PasteLinks(F.Graph,Text);
        if (!TestEqual(TEXT("Complete pair set imported"),Pasted.Num(),3)) return false;
        UK2Node_NBLBase* NewD=nullptr;
        for (auto* N : Pasted) if (N->IsDeclaration()) NewD=N;
        if (!TestNotNull(TEXT("Copied Declaration exists"),NewD)) return false;
        TestTrue(TEXT("Copied Declaration has fresh identity and name"),NewD->DeclarationId!=Original && NewD->LinkName!=D->LinkName);
        for (auto* N : Pasted) if (!N->IsDeclaration())
            TestTrue(TEXT("Copied Usage resolves to copied Declaration"),NBLLinkResolver::FindDeclaration(*N)==NewD);
    }
    auto Alone=PasteLinks(F.Graph,CopyLinks({U}));
    if (!TestEqual(TEXT("Usage alone imported"),Alone.Num(),1)) return false;
    TestTrue(TEXT("Data Usage alone retains original in original graph"),NBLLinkResolver::FindDeclaration(*Alone[0])==D);
    auto Foreign=PasteLinks(Other.Graph,CopyLinks({U}));
    if (!TestEqual(TEXT("Foreign Usage imported"),Foreign.Num(),1)) return false;
    TestNull(TEXT("Usage alone in other graph is unresolved"),NBLLinkResolver::FindDeclaration(*Foreign[0]));
    auto Cross=PasteLinks(Other.Graph,Text);
    if (!TestEqual(TEXT("Complete set pastes across graphs"),Cross.Num(),3)) return false;
    UK2Node_NBLBase* CrossD=nullptr;
    for (auto* N : Cross) if (N->IsDeclaration()) CrossD=N;
    for (auto* N : Cross) if (!N->IsDeclaration()) TestTrue(TEXT("Cross-graph pair resolves locally"),NBLLinkResolver::FindDeclaration(*N)==CrossD);
    auto* ED=NBLTests::Add<UK2Node_NBLExecDeclaration>(F.Graph); auto* EU=ED->CreateUsage();
    auto ExecAlone=PasteLinks(F.Graph,CopyLinks({EU}));
    if (!TestEqual(TEXT("Exec Usage alone imported"),ExecAlone.Num(),1)) return false;
    TestNull(TEXT("Exec Usage copy cannot create second linked Usage"),NBLLinkResolver::FindDeclaration(*ExecAlone[0]));
    auto ExecPair=PasteLinks(F.Graph,CopyLinks({ED,EU}));
    if (!TestEqual(TEXT("Exec pair imported"),ExecPair.Num(),2)) return false;
    UK2Node_NBLBase* NewED=nullptr;
    for (auto* N : ExecPair) if (N->IsDeclaration()) NewED=N;
    for (auto* N : ExecPair) if (!N->IsDeclaration()) TestTrue(TEXT("Exec pair remaps"),NBLLinkResolver::FindDeclaration(*N)==NewED);
    TestTrue(TEXT("Original pair untouched"),D->DeclarationId==Original && NBLLinkResolver::FindDeclaration(*U)==D);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNBLUndoPairs,"NamedBlueprintLinks.Lifecycle.UndoRedo",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNBLUndoPairs::RunTest(const FString&)
{
    if (!TestNotNull(TEXT("Editor transaction system exists"),GEditor)) return false;
    NBLTests::FFixture F;
    F.Graph->SetFlags(RF_Transactional);
    auto* D=NBLTests::Add<UK2Node_NBLDataDeclaration>(F.Graph); D->SetFlags(RF_Transactional);
    const FName OriginalName=D->LinkName; const FGuid Identity=D->DeclarationId;
    auto* U=D->CreateUsage();
    GEditor->UndoTransaction();
    TestFalse(TEXT("Undo removes created Usage"),F.Graph->Nodes.Contains(U));
    GEditor->RedoTransaction();
    TestTrue(TEXT("Redo restores linked Usage"),F.Graph->Nodes.Contains(U) && NBLLinkResolver::FindDeclaration(*U)==D);
    FText Error; D->RenameLink(TEXT("UndoTest"),Error);
    GEditor->UndoTransaction();
    TestTrue(TEXT("Undo rename restores both names"),D->LinkName==OriginalName && U->LinkName==OriginalName);
    GEditor->RedoTransaction();
    TestTrue(TEXT("Redo rename preserves identity"),D->LinkName==TEXT("UndoTest") && U->LinkName==D->LinkName && D->DeclarationId==Identity);
    const int32 Count=F.Graph->Nodes.Num();
    {
        FScopedTransaction Transaction(FText::FromString(TEXT("Test paste"))); F.Graph->Modify();
        PasteLinks(F.Graph,CopyLinks({D,U}));
    }
    TestEqual(TEXT("Paste added pair"),F.Graph->Nodes.Num(),Count+2);
    GEditor->UndoTransaction(); TestEqual(TEXT("Undo paste removes pair"),F.Graph->Nodes.Num(),Count);
    GEditor->RedoTransaction(); TestEqual(TEXT("Redo paste restores pair"),F.Graph->Nodes.Num(),Count+2);
    for (UEdGraphNode* N : F.Graph->Nodes)
        if (auto* Link=Cast<UK2Node_NBLBase>(N)) if (!Link->IsDeclaration())
            TestNotNull(TEXT("Redone Usage resolves"),NBLLinkResolver::FindDeclaration(*Link));
    return true;
}
#endif
