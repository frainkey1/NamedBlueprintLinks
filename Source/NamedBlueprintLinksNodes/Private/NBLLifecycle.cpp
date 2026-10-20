// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "NBLLinkResolver.h"
#include "EdGraph/EdGraph.h"

void UK2Node_NBLBase::PrepareForCopying()
{
    Super::PrepareForCopying();
    CopySourceGraph=GetGraph() ? GetGraph()->GetPathName() : FString();
}
void UK2Node_NBLBase::PostEditImport()
{
    Super::PostEditImport();
    bPendingPaste=true;
    bCompilerResolved=false;
}
void UK2Node_NBLBase::PostPasteNode()
{
    Super::PostPasteNode();
    if (!bPendingPaste || !GetGraph()) return;
    // ImportNodesFromText constructs every object (PostEditImport) before
    // PostProcessPastedNodes invokes the first PostPasteNode. Process exactly
    // this pending batch once; never touch pre-existing graph endpoints.
    TArray<UK2Node_NBLBase*> Batch;
    for (UEdGraphNode* N : GetGraph()->Nodes)
        if (auto* Link=Cast<UK2Node_NBLBase>(N)) if (Link->bPendingPaste) Batch.Add(Link);
    TMap<FGuid,UK2Node_NBLBase*> Declarations;
    TSet<FGuid> Ambiguous;
    for (auto* Link : Batch) if (Link->IsDeclaration())
    {
        const FGuid Old=Link->DeclarationId;
        if (Declarations.Contains(Old)) Ambiguous.Add(Old);
        else Declarations.Add(Old,Link);
        Link->Modify(); Link->DeclarationId=FGuid::NewGuid();
        const FString Base=Link->LinkName.ToString(); int32 Suffix=0;
        bool Taken;
        do
        {
            Link->LinkName=FName(*(Suffix==0 ? Base : FString::Printf(TEXT("%s_%d"),*Base,Suffix)));
            Taken=false;
            for (UEdGraphNode* N : GetGraph()->Nodes)
                if (auto* Other=Cast<UK2Node_NBLBase>(N))
                    if (Other!=Link && Other->IsDeclaration() && Other->IsUniversalLink()==Link->IsUniversalLink() && Other->LinkName==Link->LinkName)
                    { Taken=true; break; }
            ++Suffix;
        } while (Taken);
    }
    TSet<FGuid> UsedExecDeclarations;
    for (auto* Link : Batch) if (!Link->IsDeclaration())
    {
        Link->Modify();
        const FGuid Old=Link->TargetDeclarationId;
        auto* const* Found=Declarations.Find(Old);
        if (Found && !Ambiguous.Contains(Old) && (*Found)->IsExecutionLink()==Link->IsExecutionLink() &&
            (!Link->IsExecutionLink() || !UsedExecDeclarations.Contains((*Found)->DeclarationId)))
        {
            Link->TargetDeclarationId=(*Found)->DeclarationId; Link->LinkName=(*Found)->LinkName;
            if (Link->IsExecutionLink()) UsedExecDeclarations.Add((*Found)->DeclarationId);
        }
        else
        {
            // Data-only copy may retain its original graph-local Declaration.
            // Exec-only copies remain unresolved to enforce the single Usage rule.
            if (Found || Link->IsExecutionLink() || Link->CopySourceGraph!=GetGraph()->GetPathName())
                Link->TargetDeclarationId.Invalidate();
            if (auto* Original=NBLLinkResolver::FindDeclaration(*Link)) Link->LinkName=Original->LinkName;
            else Link->TargetDeclarationId.Invalidate();
        }
    }
    for (auto* Link : Batch) { Link->bPendingPaste=false; Link->bCompilerResolved=false; }
    for (auto* Link : Batch) Link->RefreshDataType();
}
void UK2Node_NBLBase::PostEditUndo()
{
    Super::PostEditUndo();
    bPendingPaste=false;
    bCompilerResolved=false;
    RefreshDataType();
}
