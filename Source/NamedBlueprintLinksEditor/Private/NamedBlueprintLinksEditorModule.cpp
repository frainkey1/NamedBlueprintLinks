// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"
#include "K2Node_NBLDataDeclaration.h"
#include "K2Node_NBLDataUsage.h"
#include "K2Node_NBLExecDeclaration.h"
#include "K2Node_NBLExecUsage.h"
#include "SNBLGraphNode.h"
#include "NBLStalePosePulse.h"
#include "NBLAnimBlueprintExtension.h"
#include "AnimBlueprintExtension.h"
#include "Animation/AnimBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
class FNBLNodeFactory : public FGraphPanelNodeFactory
{
public:
    virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
    {
        if (Node && (Node->IsA<UK2Node_NBLDataDeclaration>() || Node->IsA<UK2Node_NBLDataUsage>() ||
            Node->IsA<UK2Node_NBLExecDeclaration>() || Node->IsA<UK2Node_NBLExecUsage>()))
        {
            if (Node->GetGraph() && Node->GetGraph()->GetSchema()->GetGraphType(Node->GetGraph()) == GT_Animation)
                if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForGraph(Node->GetGraph())))
                    if (!UAnimBlueprintExtension::FindExtension<UNBLAnimBlueprintExtension>(AnimBlueprint))
                        UAnimBlueprintExtension::RequestExtension<UNBLAnimBlueprintExtension>(AnimBlueprint);
            return SNew(SNBLGraphNode,CastChecked<UK2Node_NBLBase>(Node));
        }
        return nullptr;
    }
};
class FNamedBlueprintLinksEditorModule : public IModuleInterface
{
    TSharedPtr<FNBLNodeFactory> Factory;
    TSharedPtr<FNBLPosePulseConnectionFactory> PulseConnectionFactory;
public:
    virtual void StartupModule() override
    {
        Factory=MakeShared<FNBLNodeFactory>();
        FEdGraphUtilities::RegisterVisualNodeFactory(Factory);
        PulseConnectionFactory=MakeShared<FNBLPosePulseConnectionFactory>();
        FEdGraphUtilities::RegisterVisualPinConnectionFactory(PulseConnectionFactory);
    }
    virtual void ShutdownModule() override
    {
        if (PulseConnectionFactory.IsValid()) FEdGraphUtilities::UnregisterVisualPinConnectionFactory(PulseConnectionFactory);
        PulseConnectionFactory.Reset();
        if (Factory.IsValid()) FEdGraphUtilities::UnregisterVisualNodeFactory(Factory);
        Factory.Reset();
    }
};
IMPLEMENT_MODULE(FNamedBlueprintLinksEditorModule,NamedBlueprintLinksEditor)
