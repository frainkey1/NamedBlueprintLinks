// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once

#include "EdGraphUtilities.h"
#include "BlueprintConnectionDrawingPolicy.h"

class FNBLPosePulseConnectionFactory final : public FGraphPanelPinConnectionFactory
{
public:
    virtual FConnectionDrawingPolicy* CreateConnectionPolicy(
        const UEdGraphSchema* Schema,
        int32 InBackLayerID,
        int32 InFrontLayerID,
        float InZoomFactor,
        const FSlateRect& InClippingRect,
        FSlateWindowElementList& InDrawElements,
        UEdGraph* InGraphObj) const override;
};
