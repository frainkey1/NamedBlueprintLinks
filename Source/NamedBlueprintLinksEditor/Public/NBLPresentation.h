// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
class UEdGraphPin;
namespace NBLPresentation
{
    NAMEDBLUEPRINTLINKSEDITOR_API bool IsPinFilled(const UEdGraphPin& Pin);
    NAMEDBLUEPRINTLINKSEDITOR_API FLinearColor PinColor(const UEdGraphPin& Pin);
}
