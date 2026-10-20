// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLLinkResolver.h"
#include "NBLRoutePlanner.h"
#include "K2Node_NBLBase.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Misc/ScopeExit.h"

UK2Node_NBLBase* NBLLinkResolver::FindDeclaration(const UK2Node_NBLBase& Usage)
{
    if (!Usage.GetGraph() || !Usage.TargetDeclarationId.IsValid()) return nullptr;
    UK2Node_NBLBase* Result = nullptr;
    for (UEdGraphNode* Node : Usage.GetGraph()->Nodes)
        if (auto* Candidate = Cast<UK2Node_NBLBase>(Node))
            if (Candidate->IsDeclaration() && Candidate->DeclarationId == Usage.TargetDeclarationId)
            {
                if (Result) return nullptr; // Never guess between duplicate identities.
                Result = Candidate;
            }
    return Result;
}

void NBLLinkResolver::ExpandGraph(UEdGraph& Graph, FKismetCompilerContext& Context)
{
    // Expansion must operate on a compiler intermediate graph, never an asset graph.
    if (UBlueprint* BP = FBlueprintEditorUtils::FindBlueprintForGraph(&Graph))
        if (BP->UbergraphPages.Contains(&Graph) || BP->FunctionGraphs.Contains(&Graph) || BP->MacroGraphs.Contains(&Graph))
        {
            Context.MessageLog.Error(TEXT("Named Blueprint Links refused to rewrite an asset graph; compiler hook needs review."));
            return;
        }
    // Every endpoint invokes this graph-wide pass. On failure, retire ALL
    // endpoints on the compiler copy so later visits do not repeat the error
    // or leave custom nodes for the K2 backend to diagnose as unexpected.
    // The asset-graph guard above must precede this cleanup scope.
    TArray<UK2Node_NBLBase*> Nodes;
    for (UEdGraphNode* GraphNode : Graph.Nodes)
        if (auto* Node = Cast<UK2Node_NBLBase>(GraphNode))
            if (!Node->bCompilerResolved) Nodes.Add(Node);
    if (Nodes.IsEmpty()) return;
    bool bExpansionSucceeded = false;
    ON_SCOPE_EXIT
    {
        if (!bExpansionSucceeded)
            for (auto* Node : Nodes)
            {
                Node->BreakAllNodeLinks();
                Node->bCompilerResolved = true;
            }
    };
    // Older saved universal nodes may carry exec pin types without the new
    // mode property. Normalize only this compiler copy, before classification.
    for (UEdGraphNode* GraphNode : Graph.Nodes)
        if (auto* Node=Cast<UK2Node_NBLBase>(GraphNode))
            if (Node->IsUniversalLink()) Node->RefreshDataType();
    NBL::Snapshot Snapshot;
    TMap<UEdGraphPin*, int32> Ids;
    TArray<UEdGraphPin*> Pins;
    Pins.Add(nullptr); // zero is reserved for an absent pin
    auto PinId = [&](UEdGraphPin* Pin) -> int32
    {
        if (!Pin) return 0;
        if (const int32* Existing = Ids.Find(Pin)) return *Existing;
        const int32 Id = Pins.Add(Pin); Ids.Add(Pin,Id); return Id;
    };
    for (UEdGraphNode* GraphNode : Graph.Nodes)
    {
        for (auto* Pin : GraphNode->Pins) PinId(Pin);
        if (auto* Node = Cast<UK2Node_NBLBase>(GraphNode))
        {
            if (Node->bCompilerResolved) continue;
            if (Node->IsUniversalLink() && Node->IsDeclaration() && Node->IsExecutionLink() &&
                Node->GetOutputPin() && !Node->GetOutputPin()->LinkedTo.IsEmpty())
            {
                Context.MessageLog.Error(TEXT("Execution link @@ cannot use a local passthrough output. Use its single linked Usage."),Node);
                return;
            }
            if (!(Node->IsDeclaration() ? Node->DeclarationId : Node->NodeGuid).IsValid())
            { Context.MessageLog.Error(TEXT("Named Blueprint Links has an invalid endpoint identity at @@"), Node); return; }
            const NBL::Kind Kind = Node->IsExecutionLink()
                ? (Node->IsDeclaration() ? NBL::Kind::ExecDeclaration : NBL::Kind::ExecUsage)
                : (Node->IsDeclaration() ? NBL::Kind::DataDeclaration : NBL::Kind::DataUsage);
            const FGuid Id = Node->IsDeclaration() ? Node->DeclarationId : Node->NodeGuid;
            Snapshot.Nodes.push_back({TCHAR_TO_UTF8(*Id.ToString()), TCHAR_TO_UTF8(*Node->TargetDeclarationId.ToString()),
                Kind, PinId(Node->GetInputPin()), (Node->IsDeclaration() && Node->IsExecutionLink()) ? 0 : PinId(Node->GetOutputPin())});
        }
    }
    if (Nodes.IsEmpty()) return;
    // Snapshot the complete graph before any expansion discards an endpoint.
    for (UEdGraphNode* GraphNode : Graph.Nodes)
        for (auto* Pin : GraphNode->Pins)
            if (Pin->Direction == EGPD_Output)
                for (auto* Other : Pin->LinkedTo)
                {
                    if (Other->GetOwningNode()->GetGraph() != &Graph)
                    { Context.MessageLog.Error(TEXT("Named Blueprint Links rejects cross-graph wire at @@"), GraphNode); return; }
                    Snapshot.Edges.push_back({PinId(Pin),PinId(Other)});
                }
    const NBL::Plan Plan = NBL::BuildPlan(Snapshot);
    if (!Plan.Ok())
    {
        UK2Node_NBLBase* Offender = Nodes[0];
        for (auto* Node : Nodes)
        {
            const FGuid Id = Node->IsDeclaration() ? Node->DeclarationId : Node->NodeGuid;
            if (Plan.ErrorNode == TCHAR_TO_UTF8(*Id.ToString())) { Offender = Node; break; }
        }
        const FString Message = FString::Printf(TEXT("%s for link '%s' at @@"),
            UTF8_TO_TCHAR(Plan.Error.c_str()), *Offender->LinkName.ToString());
        Context.MessageLog.Error(*Message, Offender); return;
    }
    // Validate replacement types before touching the intermediate graph.
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    for (const auto& Wire : Plan.Edges)
    {
        if (std::find(Snapshot.Edges.begin(),Snapshot.Edges.end(),Wire) != Snapshot.Edges.end()) continue;
        UEdGraphPin* Source = Pins[Wire.Source]; UEdGraphPin* Target = Pins[Wire.Destination];
        if (Source->Direction != EGPD_Output || Target->Direction != EGPD_Input ||
            !Schema->ArePinTypesCompatible(Source->PinType,Target->PinType,Context.NewClass))
        { Context.MessageLog.Error(TEXT("Named Blueprint Links cannot connect the source and consumer types at @@"), Target->GetOwningNode()); return; }
    }
    for (auto* Node : Nodes) Node->BreakAllNodeLinks();
    for (const auto& Wire : Plan.Edges)
        if (!Pins[Wire.Source]->LinkedTo.Contains(Pins[Wire.Destination]))
            Pins[Wire.Source]->MakeLinkTo(Pins[Wire.Destination]);
    for (auto* Node : Nodes) Node->bCompilerResolved = true;
    bExpansionSucceeded = true;
}
