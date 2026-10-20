// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "NBLLinkResolver.h"
#include "NBLTypePropagation.h"
#include "K2Node_NBLDataDeclaration.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"

void UK2Node_NBLBase::AllocateDefaultPins()
{
    const FName Category = (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PC_Exec : UEdGraphSchema_K2::PC_Wildcard;
    if (IsDeclaration()) CreatePin(EGPD_Input, Category,
        (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Execute : FName(TEXT("In")));
    if (IsUniversalLink() || !IsExecutionLink() || !IsDeclaration()) CreatePin(EGPD_Output, Category,
        (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Then : FName(TEXT("Out")));
    for (auto* Pin : Pins) Pin->bDefaultValueIsIgnored = true;
}
void UK2Node_NBLBase::PostPlacedNewNode()
{
    Super::PostPlacedNewNode();
    if (IsDeclaration())
    {
        DeclarationId = FGuid::NewGuid();
        const FString Prefix=IsUniversalLink() ? TEXT("Link") : TEXT("ExecLink");
        int32 Suffix=0;
        bool Taken;
        do
        {
            LinkName=FName(*(Suffix==0 ? Prefix : FString::Printf(TEXT("%s_%d"),*Prefix,Suffix)));
            Taken=false;
            if (GetGraph()) for (UEdGraphNode* GraphNode : GetGraph()->Nodes)
                if (auto* Other=Cast<UK2Node_NBLBase>(GraphNode))
                    if (Other!=this && Other->IsDeclaration() && Other->IsUniversalLink()==IsUniversalLink() &&
                        Other->LinkName==LinkName) { Taken=true; break; }
            ++Suffix;
        } while (Taken);
    }
}
UEdGraphPin* UK2Node_NBLBase::GetInputPin() const
{
    return FindPin((!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Execute : FName(TEXT("In")), EGPD_Input);
}
UEdGraphPin* UK2Node_NBLBase::GetOutputPin() const
{
    return FindPin((!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Then : FName(TEXT("Out")), EGPD_Output);
}
void UK2Node_NBLBase::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);
    RefreshDataType();
}
void UK2Node_NBLBase::RefreshDataType()
{
    if (!IsUniversalLink() || !GetGraph()) return;
    UK2Node_NBLBase* D = IsDeclaration() ? this : NBLLinkResolver::FindDeclaration(*this);
    if (auto* Declaration=Cast<UK2Node_NBLDataDeclaration>(D)) NBLTypePropagation::Refresh(*Declaration);
}
void UK2Node_NBLBase::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    if (!bCompilerResolved && SourceGraph) NBLLinkResolver::ExpandGraph(*SourceGraph, CompilerContext);
}
FText UK2Node_NBLBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (IsDeclaration() && (TitleType==ENodeTitleType::MenuTitle || TitleType==ENodeTitleType::ListView))
        return FText::FromString(IsUniversalLink() ? TEXT("Named Link Declaration") : TEXT("Named Execution Declaration (Legacy)"));
    if (!IsDeclaration())
        if (auto* D = NBLLinkResolver::FindDeclaration(*this)) return FText::FromName(D->LinkName);
    return FText::FromName(LinkName);
}
FText UK2Node_NBLBase::GetTooltipText() const
{
    const UEdGraphPin* Pin=GetOutputPin() ? GetOutputPin() : GetInputPin();
    const FString Type=Pin ? UEdGraphSchema_K2::TypeToText(Pin->PinType).ToString() : TEXT("Unresolved");
    const FString Hint=IsDeclaration() ? TEXT("Right-click to create a linked Usage. F2 renames this pair.") :
        (NBLLinkResolver::FindDeclaration(*this) ? TEXT("Double-click to find the Declaration.") : TEXT("Missing Declaration in this graph."));
    return FText::FromString(FString::Printf(TEXT("%s\n%s • %s\n%s"),*GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
        IsDeclaration() ? TEXT("Declaration") : TEXT("Usage"),*Type,*Hint));
}
FText UK2Node_NBLBase::GetMenuCategory() const { return FText::FromString(TEXT("Named Blueprint Links|Prototype")); }
void UK2Node_NBLBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& Registrar) const
{
    if (!IsDeclaration() || !IsUniversalLink()) return; // Legacy classes still load; new links use the universal pair.
    UClass* ActionKey = GetClass();
    if (Registrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(ActionKey);
        Registrar.AddBlueprintAction(ActionKey, Spawner);
    }
}
bool UK2Node_NBLBase::IsCompatibleWithGraph(const UEdGraph* Graph) const
{
    if (!Graph || !Graph->GetSchema()->IsA<UEdGraphSchema_K2>()) return false;
    const auto Type = Graph->GetSchema()->GetGraphType(Graph);
    // Functions are enabled only to run the transient compiler fixture. Macro,
    // animation and specialized graph support has not been validated.
    return (Type == GT_Ubergraph || Type == GT_Function) && Super::IsCompatibleWithGraph(Graph);
}
