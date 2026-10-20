// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "K2Node_NBLDataUsage.h"
#include "K2Node_NBLExecUsage.h"
#include "NBLLinkResolver.h"
#include "EdGraph/EdGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Framework/Commands/UIAction.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Logging/TokenizedMessage.h"

bool UK2Node_NBLBase::CanCreateUsage() const
{
    if (!IsDeclaration() || !GetGraph() || !GetGraph()->bEditable) return false;
    if (!IsExecutionLink()) return true;
    for (UEdGraphNode* GraphNode : GetGraph()->Nodes)
        if (auto* Node=Cast<UK2Node_NBLBase>(GraphNode))
            if (!Node->IsDeclaration() && Node->TargetDeclarationId==DeclarationId) return false;
    return true;
}
UK2Node_NBLBase* UK2Node_NBLBase::CreateUsage()
{
    if (!CanCreateUsage()) return nullptr;
    const FScopedTransaction Transaction(NSLOCTEXT("NamedBlueprintLinks","CreateUsage","Create Named Link Usage"));
    UEdGraph* Graph=GetGraph(); Graph->Modify();
    UClass* UsageClass=(!IsUniversalLink() && IsExecutionLink()) ? UK2Node_NBLExecUsage::StaticClass() : UK2Node_NBLDataUsage::StaticClass();
    auto* Usage=NewObject<UK2Node_NBLBase>(Graph,UsageClass,NAME_None,RF_Transactional);
    Usage->TargetDeclarationId=DeclarationId; Usage->LinkName=LinkName;
    Usage->NodePosX=NodePosX+320; Usage->NodePosY=NodePosY+80;
    Graph->AddNode(Usage,true,false); Usage->CreateNewGuid();
    Usage->PostPlacedNewNode(); Usage->AllocateDefaultPins(); Usage->RefreshDataType();
    if (UBlueprint* BP=FBlueprintEditorUtils::FindBlueprintForGraph(Graph)) FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
    Graph->NotifyGraphChanged();
    return Usage;
}
bool UK2Node_NBLBase::RenameLink(FName NewName, FText& OutError)
{
    OutError=FText::GetEmpty();
    if (!IsDeclaration() || !GetGraph()) { OutError=FText::FromString(TEXT("Rename the Declaration.")); return false; }
    if (NewName.IsNone() || NewName.ToString().TrimStartAndEnd().IsEmpty())
    { OutError=FText::FromString(TEXT("A link name cannot be empty.")); return false; }
    for (UEdGraphNode* GraphNode : GetGraph()->Nodes)
        if (auto* Other=Cast<UK2Node_NBLBase>(GraphNode))
            if (Other!=this && Other->IsDeclaration() && Other->IsUniversalLink()==IsUniversalLink() && Other->LinkName==NewName)
            { OutError=FText::FromString(TEXT("That link name is already used in this graph and category.")); return false; }
    if (NewName==LinkName) return true;
    const FScopedTransaction Transaction(NSLOCTEXT("NamedBlueprintLinks","RenameLink","Rename Named Link"));
    Modify(); LinkName=NewName; ClearCompilerMessage();
    for (UEdGraphNode* GraphNode : GetGraph()->Nodes)
        if (auto* Usage=Cast<UK2Node_NBLBase>(GraphNode))
            if (!Usage->IsDeclaration() && Usage->TargetDeclarationId==DeclarationId)
            { Usage->Modify(); Usage->LinkName=NewName; }
    if (UBlueprint* BP=FBlueprintEditorUtils::FindBlueprintForGraph(GetGraph())) FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
    GetGraph()->NotifyGraphChanged();
    return true;
}
void UK2Node_NBLBase::OnRenameNode(const FString& NewName)
{
    FText Error;
    if (!RenameLink(FName(*NewName.TrimStartAndEnd()),Error))
    { bHasCompilerMessage=true; ErrorType=EMessageSeverity::Error; ErrorMsg=Error.ToString(); GetGraph()->NotifyGraphChanged(); }
}
void UK2Node_NBLBase::PostReconstructNode()
{
    Super::PostReconstructNode(); RefreshDataType();
}
bool UK2Node_NBLBase::CanJumpToDefinition() const
{ return !IsDeclaration() && NBLLinkResolver::FindDeclaration(*this)!=nullptr; }
UObject* UK2Node_NBLBase::GetJumpTargetForDoubleClick() const
{ return IsDeclaration() ? nullptr : NBLLinkResolver::FindDeclaration(*this); }
void UK2Node_NBLBase::JumpToDefinition() const
{
    if (auto* Target=GetJumpTargetForDoubleClick()) FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Target);
}
void UK2Node_NBLBase::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu,Context);
    if (!Menu) return;
    auto* Mutable=const_cast<UK2Node_NBLBase*>(this);
    auto& Section=Menu->AddSection(TEXT("NBL"),FText::FromString(TEXT("Named Blueprint Links")),
        FToolMenuInsert(TEXT("EdGraphSchemaNodeActions"),EToolMenuInsertType::After));
    if (IsDeclaration())
        Section.AddMenuEntry(TEXT("NBLCreateUsage"),FText::FromString(TEXT("Create Linked Usage")),
            FText::FromString(TEXT("Create a Usage linked to this Declaration.")),FSlateIcon(),
            FUIAction(FExecuteAction::CreateWeakLambda(Mutable,[Mutable]() { Mutable->CreateUsage(); }),
                FCanExecuteAction::CreateWeakLambda(Mutable,[Mutable]() { return Mutable->CanCreateUsage(); })));
    else
        Section.AddMenuEntry(TEXT("NBLGoToDeclaration"),FText::FromString(TEXT("Go to Declaration")),
            FText::FromString(TEXT("Find this Usage's Declaration.")),FSlateIcon(),
            FUIAction(FExecuteAction::CreateWeakLambda(Mutable,[Mutable]() { Mutable->JumpToDefinition(); }),
                FCanExecuteAction::CreateWeakLambda(Mutable,[Mutable]() { return Mutable->CanJumpToDefinition(); })));
}
