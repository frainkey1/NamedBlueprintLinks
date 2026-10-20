// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "NBLLinkResolver.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
bool UK2Node_NBLBase::IsConnectionDisallowed(const UEdGraphPin* MyPin,const UEdGraphPin* OtherPin,FString& OutReason) const
{
    if (!OtherPin || !IsUniversalLink()) return Super::IsConnectionDisallowed(MyPin,OtherPin,OutReason);
    if (OtherPin->PinType.PinCategory==UEdGraphSchema_K2::PC_Exec)
    {
        const auto* D=IsDeclaration() ? this : NBLLinkResolver::FindDeclaration(*this);
        if (!D) { OutReason=TEXT("Execution needs a Declaration in this graph."); return true; }
        if (IsDeclaration() && MyPin==GetOutputPin())
        { OutReason=TEXT("Use a linked Usage for execution; local passthrough is available for data only."); return true; }
        int32 Usages=0;
        for (UEdGraphNode* N : D->GetGraph()->Nodes)
            if (const auto* U=Cast<UK2Node_NBLBase>(N))
                if (!U->IsDeclaration() && U->TargetDeclarationId==D->DeclarationId) ++Usages;
        if (Usages>1 || (D->GetOutputPin() && !D->GetOutputPin()->LinkedTo.IsEmpty()))
        { OutReason=TEXT("Execution requires at most one Usage and no connected local passthrough. Remove the extra paths first."); return true; }
    }
    return Super::IsConnectionDisallowed(MyPin,OtherPin,OutReason);
}
