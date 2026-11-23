#pragma once

#include "AnimBlueprintExtension.h"
#include "NBLAnimBlueprintExtension.generated.h"

class UAnimGraphNode_Base;
class IAnimBlueprintCompilationContext;
class IAnimBlueprintGeneratedClassCompiledData;

UCLASS()
class UNBLAnimBlueprintExtension : public UAnimBlueprintExtension
{
    GENERATED_BODY()

protected:
    virtual void HandlePreProcessAnimationNodes(
        TArrayView<UAnimGraphNode_Base*> InAnimNodes,
        IAnimBlueprintCompilationContext& InCompilationContext,
        IAnimBlueprintGeneratedClassCompiledData& OutCompiledData) override;
};
