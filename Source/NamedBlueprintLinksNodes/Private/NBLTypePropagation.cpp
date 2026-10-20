// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLTypePropagation.h"
#include "K2Node_NBLDataDeclaration.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"

void NBLTypePropagation::Refresh(UK2Node_NBLDataDeclaration& Declaration)
{
    UEdGraph* Graph=Declaration.GetGraph();
    if (!Graph) return;
    // A named pair, plus any physically connected named pairs, is one type
    // constraint component. Only external connections constrain its type;
    // old pin colors/types on the routing nodes are not constraints.
    TArray<UK2Node_NBLBase*> Component{&Declaration};
    for (int32 Index=0; Index<Component.Num(); ++Index)
    {
        auto* Current=Component[Index];
        for (UEdGraphNode* GraphNode : Graph->Nodes)
        {
            auto* Other=Cast<UK2Node_NBLBase>(GraphNode);
            if (!Other || !Other->IsUniversalLink()) continue;
            if ((Current->IsDeclaration() && !Other->IsDeclaration() &&
                 Other->TargetDeclarationId==Current->DeclarationId) ||
                (!Current->IsDeclaration() && Other->IsDeclaration() &&
                 Current->TargetDeclarationId==Other->DeclarationId)) Component.AddUnique(Other);
        }
        for (auto* Pin : Current->Pins)
            for (auto* Linked : Pin->LinkedTo)
                if (auto* Other=Cast<UK2Node_NBLBase>(Linked->GetOwningNode()))
                    if (Other->IsUniversalLink() && Other->GetGraph()==Graph) Component.AddUnique(Other);
    }

    TArray<UEdGraphPin*> Constraints;
    for (auto* Node : Component)
        for (auto* Pin : Node->Pins)
            for (auto* Other : Pin->LinkedTo)
            {
                auto* OtherNode=Cast<UK2Node_NBLBase>(Other->GetOwningNode());
                if (OtherNode && Component.Contains(OtherNode)) continue;
                if (Other->PinType.PinCategory!=UEdGraphSchema_K2::PC_Wildcard)
                    Constraints.AddUnique(Other);
            }

    FEdGraphPinType Type;
    Type.PinCategory=UEdGraphSchema_K2::PC_Wildcard;
    if (!Constraints.IsEmpty())
    {
        // The actual source defines exact class/container/reference qualifiers.
        // Before a source is connected, infer from a downstream consumer.
        UEdGraphPin* Preferred=Constraints[0];
        for (auto* Pin : Constraints)
            if (Pin->Direction==EGPD_Output) { Preferred=Pin; break; }
        Type=Preferred->PinType;
        const auto* Schema=GetDefault<UEdGraphSchema_K2>();
        // Match K2 connection validation: Self is relative to this Blueprint,
        // not a concrete class stored in the pin's subtype object.
        const UBlueprint* Blueprint=FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
        const UClass* CallingContext=Blueprint
            ? (Blueprint->GeneratedClass ? Blueprint->GeneratedClass : Blueprint->ParentClass) : nullptr;
        for (auto* Pin : Constraints)
        {
            const bool Compatible=Pin->Direction==EGPD_Output
                ? Schema->ArePinTypesCompatible(Pin->PinType,Type,CallingContext)
                : Schema->ArePinTypesCompatible(Type,Pin->PinType,CallingContext);
            // Do not silently recolor an already contradictory component.
            // Connection schema/compiler validation diagnoses invalid links.
            if (!Compatible) return;
        }
    }
    bool AnyChanged=false;
    for (auto* Node : Component)
    {
        const bool Execution=Type.PinCategory==UEdGraphSchema_K2::PC_Exec;
        bool Changed=Node->bUniversalExecution!=Execution;
        for (auto* Pin : Node->Pins) Changed |= !(Pin->PinType==Type);
        if (Node->IsDeclaration() && Node->GetOutputPin())
            Changed |= Node->GetOutputPin()->bHidden!=Execution;
        if (!Changed) continue;
        AnyChanged=true;
        Node->Modify(false);
        Node->bUniversalExecution=Execution;
        for (auto* Pin : Node->Pins) Pin->PinType=Type;
        if (Node->IsDeclaration() && Node->GetOutputPin()) Node->GetOutputPin()->bHidden=Execution;
    }
    // Native pin widgets cache their geometry when constructed. Notify once,
    // after the entire component is consistent, to rebuild both endpoints.
    if (AnyChanged) Graph->NotifyGraphChanged();
}
