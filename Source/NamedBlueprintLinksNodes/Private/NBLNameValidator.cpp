// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "K2Node_NBLBase.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "EdGraph/EdGraph.h"
namespace
{
class FNBLNameValidator : public INameValidatorInterface
{
    TWeakObjectPtr<const UK2Node_NBLBase> Owner;
public:
    explicit FNBLNameValidator(const UK2Node_NBLBase* Node) : Owner(Node) {}
    virtual EValidatorResult IsValid(const FString& Name,bool = false) override
    {
        const FString Trimmed=Name.TrimStartAndEnd();
        if (Trimmed.IsEmpty() || Trimmed.Equals(TEXT("None"),ESearchCase::IgnoreCase)) return EValidatorResult::EmptyName;
        if (Trimmed.Len()>=NAME_SIZE) return EValidatorResult::TooLong;
        return IsValid(FName(*Trimmed));
    }
    virtual EValidatorResult IsValid(const FName& Name,bool = false) override
    {
        if (Name.IsNone()) return EValidatorResult::EmptyName;
        const auto* Node=Owner.Get();
        if (!Node || !Node->GetGraph()) return EValidatorResult::AlreadyInUse;
        if (Name==Node->LinkName) return EValidatorResult::ExistingName;
        for (UEdGraphNode* GraphNode : Node->GetGraph()->Nodes)
            if (const auto* Other=Cast<UK2Node_NBLBase>(GraphNode))
                if (Other!=Node && Other->IsDeclaration() && Other->IsUniversalLink()==Node->IsUniversalLink() && Other->LinkName==Name)
                    return EValidatorResult::AlreadyInUse;
        return EValidatorResult::Ok;
    }
};
}
TSharedPtr<INameValidatorInterface> UK2Node_NBLBase::MakeNameValidator() const
{ return MakeShared<FNBLNameValidator>(this); }
