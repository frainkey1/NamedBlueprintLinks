// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "K2Node_NBLBase.h"
#include "K2Node_NBLExecDeclaration.generated.h"
UCLASS()
class NAMEDBLUEPRINTLINKSNODES_API UK2Node_NBLExecDeclaration : public UK2Node_NBLBase
{
    GENERATED_BODY()
public:
    virtual bool IsDeclaration() const override { return true; }
    virtual bool IsExecutionLink() const override { return true; }
};
