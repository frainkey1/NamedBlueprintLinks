// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "NBLLinkResolver.h"
#include "NBLTypePropagation.h"
#include "K2Node_NBLDataDeclaration.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "AnimationGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"

void UK2Node_NBLBase::AllocateDefaultPins()
{
    const FName Category = (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PC_Exec : UEdGraphSchema_K2::PC_Wildcard;
    const bool bHasInput = IsDeclaration();
    const bool bHasOutput = IsUniversalLink() || !IsExecutionLink() || !IsDeclaration();
    const FName InputName = (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Execute : FName(TEXT("In"));
    const FName OutputName = (!IsUniversalLink() && IsExecutionLink()) ? UEdGraphSchema_K2::PN_Then : FName(TEXT("Out"));
    // K2Node_Knot has non-virtual accessors that index Pins[0] and Pins[1].
    // Reserve hidden companion pins for NBL's one-pin endpoints so engine reroute paths stay safe.
    if (bHasInput) CreatePin(EGPD_Input, Category, InputName);
    else
    {
        UEdGraphPin* HiddenInput = CreatePin(EGPD_Input, Category, TEXT("NBL_InternalInput"));
        HiddenInput->bHidden = true;
        HiddenInput->bNotConnectable = true;
    }
    const bool bNeedsPoseBridgeOutput = IsUniversalLink() && IsDeclaration() && !IsExecutionLink() &&
        GetGraph() && GetGraph()->GetSchema()->IsA<UAnimationGraphSchema>();
    if (bNeedsPoseBridgeOutput || !bHasOutput)
    {
        UEdGraphPin* HiddenOutput = CreatePin(EGPD_Output, Category, TEXT("NBL_InternalOutput"));
        HiddenOutput->bHidden = true;
        HiddenOutput->bNotConnectable = true;
    }
    if (bHasOutput) CreatePin(EGPD_Output, Category, OutputName);
    for (auto* Pin : Pins) Pin->bDefaultValueIsIgnored = true;
}
void UK2Node_NBLBase::PostPlacedNewNode()
{
    Super::PostPlacedNewNode();
    if (IsDeclaration())
    {
        DeclarationId = FGuid::NewGuid();
        bUsesDefaultLinkName = true;
        const FString Prefix=IsUniversalLink() ? TEXT("Link") : TEXT("ExecLink");
        int32 Suffix=0;
        bool Taken;
        do
        {
            LinkName=FName(*(Suffix==0 ? Prefix : FString::Printf(TEXT("%s%d"),*Prefix,Suffix)));
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
UEdGraphPin* UK2Node_NBLBase::GetPassThroughPin(const UEdGraphPin* FromPin) const
{
    if (FromPin == GetInputPin()) return GetOutputPin();
    if (FromPin == GetOutputPin()) return GetInputPin();
    return nullptr;
}
void UK2Node_NBLBase::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    // UK2Node_Knot assumes a conventional input/output pair.
    UK2Node::NotifyPinConnectionListChanged(Pin);
    RefreshDataType();
}
void UK2Node_NBLBase::RefreshDataType()
{
    if (!IsUniversalLink() || !GetGraph()) return;
    UK2Node_NBLBase* D = IsDeclaration() ? this : NBLLinkResolver::FindDeclaration(*this);
    if (auto* Declaration=Cast<UK2Node_NBLDataDeclaration>(D)) NBLTypePropagation::Refresh(*Declaration);
    RefreshPoseLinkBridge();
}
void UK2Node_NBLBase::RefreshPoseLinkBridge()
{
    UEdGraph* Graph = GetGraph();
    if (!Graph || !IsUniversalLink()) return;
    UK2Node_NBLBase* Declaration = IsDeclaration() ? this : NBLLinkResolver::FindDeclaration(*this);
    if (!Declaration)
    {
        if (UEdGraphPin* BridgeInput = FindPin(TEXT("NBL_InternalInput"), EGPD_Input))
        {
            const TArray<UEdGraphPin*> ExistingLinks = BridgeInput->LinkedTo;
            for (UEdGraphPin* LinkedPin : ExistingLinks)
                BridgeInput->BreakLinkTo(LinkedPin, false);
        }
        return;
    }
    UEdGraphPin* BridgeOutput = Declaration->FindPin(TEXT("NBL_InternalOutput"), EGPD_Output);
    if (!BridgeOutput) return;
    const UEdGraphPin* PosePin = Declaration->GetInputPin();
    const bool bPoseLink = Graph->GetSchema()->IsA<UAnimationGraphSchema>() && PosePin &&
        UAnimationGraphSchema::IsPosePin(PosePin->PinType);
    TArray<UK2Node_NBLBase*> Usages;
    for (UEdGraphNode* GraphNode : Graph->Nodes)
        if (UK2Node_NBLBase* Usage = Cast<UK2Node_NBLBase>(GraphNode))
            if (!Usage->IsDeclaration() && Usage->IsUniversalLink() && Usage->TargetDeclarationId == Declaration->DeclarationId)
                Usages.Add(Usage);
    const bool bCanBridge = bPoseLink && !Declaration->IsExecutionLink() && Usages.Num() == 1 && !Usages[0]->IsExecutionLink();
    bool bGraphChanged = false;
    for (UK2Node_NBLBase* Usage : Usages)
    {
        UEdGraphPin* BridgeInput = Usage->FindPin(TEXT("NBL_InternalInput"), EGPD_Input);
        if (!BridgeInput) continue;
        if (bCanBridge && !BridgeOutput->LinkedTo.Contains(BridgeInput))
        {
            BridgeOutput->MakeLinkTo(BridgeInput, false);
            bGraphChanged = true;
        }
        else if (!bCanBridge && BridgeOutput->LinkedTo.Contains(BridgeInput))
        {
            BridgeOutput->BreakLinkTo(BridgeInput, false);
            bGraphChanged = true;
        }
    }
    if (bGraphChanged) Graph->NotifyGraphChanged();
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
    const bool bSupportedGraph = Type == GT_Ubergraph || Type == GT_Function ||
        (Type == GT_Animation && IsUniversalLink() && !IsExecutionLink());
    return bSupportedGraph && Super::IsCompatibleWithGraph(Graph);
}
