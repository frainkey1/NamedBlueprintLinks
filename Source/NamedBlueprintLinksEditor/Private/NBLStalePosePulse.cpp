// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "NBLStalePosePulse.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_Base.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_NBLBase.h"
#include "SGraphPin.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"

namespace
{
class FNBLStalePosePulsePolicy final : public FKismetConnectionDrawingPolicy
{
public:
    FNBLStalePosePulsePolicy(
        int32 InBackLayerID,
        int32 InFrontLayerID,
        float InZoomFactor,
        const FSlateRect& InClippingRect,
        FSlateWindowElementList& InDrawElements,
        UEdGraph* InGraphObj)
        : FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj)
        , PulseLayer(InFrontLayerID)
    {
    }

    virtual bool TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const override
    {
        return InputPin && OutputPin && UAnimationGraphSchema::IsPosePin(OutputPin->PinType);
    }

    virtual void BuildExecutionRoadmap() override
    {
        PredecessorPins.Reset();

        UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForGraph(GraphObj));
        UAnimBlueprintGeneratedClass* AnimClass = AnimBlueprint
            ? Cast<UAnimBlueprintGeneratedClass>(AnimBlueprint->GeneratedClass)
            : nullptr;
        if (!AnimClass || !AnimBlueprint->GetObjectBeingDebugged())
        {
            return;
        }

        FAnimBlueprintDebugData& DebugData = AnimClass->GetAnimBlueprintDebugData();
        const int32 NumAnimNodes = AnimClass->GetAnimNodeProperties().Num();

        for (const FAnimBlueprintDebugData::FNodeVisit& Visit : DebugData.UpdatedNodesThisFrame)
        {
            if (Visit.SourceID < 0 || Visit.SourceID >= NumAnimNodes ||
                Visit.TargetID < 0 || Visit.TargetID >= NumAnimNodes)
            {
                continue;
            }

            const int32 SourceIndex = NumAnimNodes - 1 - Visit.SourceID;
            const int32 TargetIndex = NumAnimNodes - 1 - Visit.TargetID;
            const UAnimGraphNode_Base* SourceNode = Cast<UAnimGraphNode_Base>(
                DebugData.NodePropertyIndexToNodeMap.FindRef(SourceIndex));
            const UAnimGraphNode_Base* TargetNode = Cast<UAnimGraphNode_Base>(
                DebugData.NodePropertyIndexToNodeMap.FindRef(TargetIndex));
            if (!SourceNode || !TargetNode)
            {
                continue;
            }

            UEdGraphPin* PoseOutput = nullptr;
            for (UEdGraphPin* Pin : TargetNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && UAnimationGraphSchema::IsPosePin(Pin->PinType))
                {
                    PoseOutput = Pin;
                    break;
                }
            }

            if (PoseOutput)
            {
                FTimePair& Timing = PredecessorPins.FindOrAdd(const_cast<UAnimGraphNode_Base*>(SourceNode))
                    .FindOrAdd(PoseOutput);
                Timing.PredExecTime = 0.0;
                Timing.ThisExecTime = FMath::Clamp(Visit.Weight, 0.0f, 1.0f);
            }
        }
    }

    virtual void DetermineStyleOfExecWire(
        float& Thickness,
        FLinearColor& WireColor,
        bool& bDrawBubbles,
        const FTimePair& Times) override
    {
        const float BlendWeight = static_cast<float>(Times.ThisExecTime);
        Thickness = FMath::Lerp(SustainWireThickness, AttackWireThickness, BlendWeight);
        WireColor *= BlendWeight * 0.5f + 0.5f;
        bDrawBubbles = true;
    }

    virtual void Draw(
        TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
        FArrangedChildren& ArrangedNodes) override
    {
        FKismetConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);

        if (!IsBlueprintStale())
        {
            return;
        }

        const double Time = FPlatformTime::Seconds();
        for (UEdGraphNode* Node : GraphObj->Nodes)
        {
            if (!Node || !Node->IsA<UK2Node_NBLBase>())
            {
                continue;
            }

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin || !UAnimationGraphSchema::IsPosePin(Pin->PinType))
                {
                    continue;
                }

                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (!LinkedPin || !LinkedPin->GetOwningNode() ||
                        !LinkedPin->GetOwningNode()->IsA<UAnimGraphNode_Base>())
                    {
                        continue;
                    }

                    UEdGraphPin* OutputPin = Pin->Direction == EGPD_Output ? Pin : LinkedPin;
                    UEdGraphPin* InputPin = Pin->Direction == EGPD_Input ? Pin : LinkedPin;
                    const FGeometry* StartGeometry = FindPinGeometry(InPinGeometries, OutputPin);
                    const FGeometry* EndGeometry = FindPinGeometry(InPinGeometries, InputPin);
                    if (!StartGeometry || !EndGeometry)
                    {
                        continue;
                    }

                    const FVector2f Start = StartGeometry->LocalToAbsolute(
                        FVector2f(StartGeometry->GetLocalSize().X, StartGeometry->GetLocalSize().Y * 0.5f));
                    const FVector2f End = EndGeometry->LocalToAbsolute(
                        FVector2f(0.0f, EndGeometry->GetLocalSize().Y * 0.5f));
                    DrawPulse(Start, End, Time);
                }
            }
        }
    }

private:
    bool IsBlueprintStale() const
    {
        const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(GraphObj);
        return Blueprint && Blueprint->Status != BS_UpToDate;
    }

    static const FGeometry* FindPinGeometry(
        const TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries,
        const UEdGraphPin* Pin)
    {
        for (const TPair<TSharedRef<SWidget>, FArrangedWidget>& Entry : PinGeometries)
        {
            const TSharedRef<SGraphPin> GraphPin = StaticCastSharedRef<SGraphPin>(Entry.Key);
            if (GraphPin->GetPinObj() == Pin)
            {
                return &Entry.Value.Geometry;
            }
        }
        return nullptr;
    }

    void DrawPulse(const FVector2f& Start, const FVector2f& End, double Time) const
    {
        const FVector2f Tangent = ComputeSplineTangent(Start, End);
        const FVector2f Control1 = Start + Tangent;
        const FVector2f Control2 = End - Tangent;

        const auto PointAt = [&](float T)
        {
            const float OneMinusT = 1.0f - T;
            return OneMinusT * OneMinusT * OneMinusT * Start
                + 3.0f * OneMinusT * OneMinusT * T * Control1
                + 3.0f * OneMinusT * T * T * Control2
                + T * T * T * End;
        };

        for (int32 Dot = 0; Dot < 4; ++Dot)
        {
            const float Phase = FMath::Fmod(static_cast<float>(Time * 0.55) + Dot * 0.25f, 1.0f);
            const float T0 = FMath::Max(0.0f, Phase - 0.018f);
            const float T1 = FMath::Min(1.0f, Phase + 0.018f);
            if (T1 <= T0)
            {
                continue;
            }

            const FVector2f P0 = PointAt(T0);
            const FVector2f P1 = PointAt(T1);
            const FVector2f SegmentTangent = (P1 - P0) * 0.5f;
            const float Pulse = FMath::Sin(Phase * UE_TWO_PI);
            const float Alpha = 0.35f + 0.65f * Pulse * Pulse;
            FSlateDrawElement::MakeDrawSpaceSpline(
                DrawElementsList,
                PulseLayer,
                P0,
                SegmentTangent,
                P1,
                SegmentTangent,
                2.25f,
                ESlateDrawEffect::None,
                FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
        }
    }

    int32 PulseLayer;
};
}

FConnectionDrawingPolicy* FNBLPosePulseConnectionFactory::CreateConnectionPolicy(
    const UEdGraphSchema* Schema,
    int32 InBackLayerID,
    int32 InFrontLayerID,
    float InZoomFactor,
    const FSlateRect& InClippingRect,
    FSlateWindowElementList& InDrawElements,
    UEdGraph* InGraphObj) const
{
    if (!Schema || !Schema->IsA<UAnimationGraphSchema>() || !InGraphObj)
    {
        return nullptr;
    }

    const bool bHasNamedPoseConnection = InGraphObj->Nodes.ContainsByPredicate([](const UEdGraphNode* Node)
    {
        if (!Node || !Node->IsA<UK2Node_NBLBase>())
        {
            return false;
        }

        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin && UAnimationGraphSchema::IsPosePin(Pin->PinType))
            {
                for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (LinkedPin && LinkedPin->GetOwningNode() && LinkedPin->GetOwningNode()->IsA<UAnimGraphNode_Base>())
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    });

    if (!bHasNamedPoseConnection)
    {
        return nullptr;
    }

    return new FNBLStalePosePulsePolicy(
        InBackLayerID,
        InFrontLayerID,
        InZoomFactor,
        InClippingRect,
        InDrawElements,
        InGraphObj);
}
