// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include "K2Node.h"
#include "K2Node_NBLBase.generated.h"

UCLASS(Abstract)
class NAMEDBLUEPRINTLINKSNODES_API UK2Node_NBLBase : public UK2Node
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, Category="Named Link")
    FName LinkName = TEXT("NamedLink");
    UPROPERTY(VisibleAnywhere, Category="Named Link")
    FGuid DeclarationId;
    UPROPERTY(EditAnywhere, Category="Named Link")
    FGuid TargetDeclarationId;
    UPROPERTY(Transient)
    bool bCompilerResolved = false;

    UPROPERTY()
    bool bUniversalExecution = false;
    virtual bool IsUniversalLink() const { return false; }
    virtual bool IsDeclaration() const PURE_VIRTUAL(UK2Node_NBLBase::IsDeclaration, return false;);
    virtual bool IsExecutionLink() const PURE_VIRTUAL(UK2Node_NBLBase::IsExecutionLink, return false;);
    UEdGraphPin* GetInputPin() const;
    UEdGraphPin* GetOutputPin() const;
    void RefreshDataType();
    UK2Node_NBLBase* CreateUsage();
    bool CanCreateUsage() const;
    bool RenameLink(FName NewName, FText& OutError);
    virtual void AllocateDefaultPins() override;
    virtual void PostPlacedNewNode() override;
    virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
    virtual void PostReconstructNode() override;
    virtual bool GetCanRenameNode() const override { return IsDeclaration(); }
    virtual TSharedPtr<INameValidatorInterface> MakeNameValidator() const override;
    virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin,const UEdGraphPin* OtherPin,FString& OutReason) const override;
    virtual void OnRenameNode(const FString& NewName) override;
    virtual bool CanJumpToDefinition() const override;
    virtual UObject* GetJumpTargetForDoubleClick() const override;
    virtual void JumpToDefinition() const override;
    virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
    // UE's extra pure-node pruning pass does not honor IsNodeRootSet alone.
    // Until expansion, classify endpoints as impure compiler roots so their
    // upstream pure dependencies survive. No runtime statements are emitted.
    virtual bool IsNodePure() const override { return bCompilerResolved && !IsExecutionLink(); }
    virtual bool IsNodeRootSet() const override { return !bCompilerResolved; }
    virtual bool IsNodeSafeToIgnore() const override { return bCompilerResolved; }
    virtual bool CanSplitPin(const UEdGraphPin* Pin) const override { return false; }
    virtual bool CanDuplicateNode() const override { return true; }
    virtual void PrepareForCopying() override;
    virtual void PostEditImport() override;
    virtual void PostPasteNode() override;
    virtual void PostEditUndo() override;
private:
    // Serialized only for clipboard provenance; graph-local lookup never uses it.
    UPROPERTY()
    FString CopySourceGraph;
    UPROPERTY(Transient)
    bool bPendingPaste = false;
public:
    virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual FText GetMenuCategory() const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
};
