// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
class UK2Node_NBLBase;
class UEdGraph;
class FKismetCompilerContext;
class FCompilerResultsLog;
namespace NBLLinkResolver
{
UK2Node_NBLBase* FindDeclaration(const UK2Node_NBLBase& Usage);
void ExpandGraph(UEdGraph& Graph, FKismetCompilerContext& CompilerContext);
NAMEDBLUEPRINTLINKSNODES_API void ExpandAnimationGraph(UEdGraph& Graph, FCompilerResultsLog& MessageLog, UClass* CallingContext);
}
