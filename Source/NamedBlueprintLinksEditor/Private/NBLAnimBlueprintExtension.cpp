#include "NBLAnimBlueprintExtension.h"
#include "NBLLinkResolver.h"
#include "IAnimBlueprintCompilationContext.h"
#include "Engine/Blueprint.h"
#include "Animation/AnimBlueprint.h"
#include "EdGraph/EdGraph.h"

void UNBLAnimBlueprintExtension::HandlePreProcessAnimationNodes(
    TArrayView<UAnimGraphNode_Base*> InAnimNodes,
    IAnimBlueprintCompilationContext& InCompilationContext,
    IAnimBlueprintGeneratedClassCompiledData& OutCompiledData)
{
    UEdGraph* ConsolidatedGraph = InCompilationContext.GetConsolidatedEventGraph();
    const UBlueprint* Blueprint = InCompilationContext.GetBlueprint();
    if (!ConsolidatedGraph || !Blueprint) return;
    FCompilerResultsLog& MessageLog = InCompilationContext.GetMessageLog();
    UClass* CallingContext = const_cast<UClass*>(Blueprint->GeneratedClass.Get());
    NBLLinkResolver::ExpandAnimationGraph(*ConsolidatedGraph, MessageLog, CallingContext);
}
