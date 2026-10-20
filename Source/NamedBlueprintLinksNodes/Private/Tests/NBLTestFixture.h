// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "NBLTestLibrary.h"

namespace NBLTests
{
template<class T> T* Add(UEdGraph* Graph)
{
    T* Node = NewObject<T>(Graph);
    Graph->AddNode(Node,false,false); Node->CreateNewGuid();
    Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
    return Node;
}
inline UK2Node_CallFunction* Call(UEdGraph* Graph, FName Name)
{
    auto* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(UNBLTestLibrary::StaticClass()->FindFunctionByName(Name));
    Graph->AddNode(Node,false,false); Node->CreateNewGuid(); Node->AllocateDefaultPins();
    return Node;
}
struct FFixture
{
    UBlueprint* BP = nullptr;
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Return = nullptr;
    FFixture()
    {
        const FName Name = MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("NBLTypes"));
        BP=FKismetEditorUtilities::CreateBlueprint(UObject::StaticClass(),GetTransientPackage(),Name,
            BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
        Graph=FBlueprintEditorUtils::CreateNewGraph(BP,TEXT("Evaluate"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph<UClass>(BP,Graph,true,nullptr);
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (auto* E=Cast<UK2Node_FunctionEntry>(Node)) Entry=E;
            if (auto* R=Cast<UK2Node_FunctionResult>(Node)) Return=R;
        }
        if (!Return) Return=Add<UK2Node_FunctionResult>(Graph);
    }
};
}
