// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "KismetNodes/SGraphNodeK2Var.h"
class UK2Node_NBLBase;
class SNBLGraphNode : public SGraphNodeK2Var
{
public:
    SLATE_BEGIN_ARGS(SNBLGraphNode) {} SLATE_END_ARGS()
    void Construct(const FArguments&,UK2Node_NBLBase* Node);
    virtual void UpdateGraphNode() override;
    virtual void AddPin(const TSharedRef<SGraphPin>& Pin) override;
private:
    FSlateColor GetLinkTint() const;
};
