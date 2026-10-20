// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLPresentation.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
namespace NBLPresentation
{
bool IsPinFilled(const UEdGraphPin& Pin) { return Pin.LinkedTo.Num()>0; }
FLinearColor PinColor(const UEdGraphPin& Pin)
{
    if (Pin.PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard) return FLinearColor(0.35f,0.35f,0.35f);
    const auto* Schema=Pin.GetSchema();
    return Schema ? Schema->GetPinTypeColor(Pin.PinType) : FLinearColor::White;
}
}
