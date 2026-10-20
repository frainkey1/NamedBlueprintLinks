// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#include "SNBLGraphNode.h"
#include "NBLPresentation.h"
#include "K2Node_NBLBase.h"
#include "SGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"
#include "GraphEditorSettings.h"
#include "Styling/AppStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SErrorText.h"

namespace
{
// Two interlocking outlines; no external image/font dependency.
class SNBLLinkIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SNBLLinkIcon) : _Tint(FLinearColor::White) {}
        SLATE_ATTRIBUTE(FSlateColor,Tint)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Tint=Args._Tint; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(18,14); }
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geometry,const FSlateRect&,
        FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool) const override
    {
        static const FSlateRoundedBoxBrush Outline(FLinearColor::Transparent,2.0f,FLinearColor::White,1.0f);
        FSlateDrawElement::MakeBox(Elements,Layer,Geometry.ToPaintGeometry(FVector2f(11,8),FSlateLayoutTransform(FVector2f(0,5))),
            &Outline,ESlateDrawEffect::None,Style.GetColorAndOpacityTint()*Tint.Get().GetSpecifiedColor());
        FSlateDrawElement::MakeBox(Elements,Layer+1,Geometry.ToPaintGeometry(FVector2f(11,8),FSlateLayoutTransform(FVector2f(6,1))),
            &Outline,ESlateDrawEffect::None,Style.GetColorAndOpacityTint()*Tint.Get().GetSpecifiedColor());
        return Layer+1;
    }
private:
    TAttribute<FSlateColor> Tint;
};
}
void SNBLGraphNode::Construct(const FArguments&,UK2Node_NBLBase* Node)
{
    GraphNode=Node; SetCursor(EMouseCursor::CardinalCross); UpdateGraphNode();
}
FSlateColor SNBLGraphNode::GetLinkTint() const
{
    const auto* Node=CastChecked<UK2Node_NBLBase>(GraphNode);
    const auto* Pin=Node->GetInputPin() ? Node->GetInputPin() : Node->GetOutputPin();
    FLinearColor Color=Pin ? NBLPresentation::PinColor(*Pin) : FLinearColor::Gray;
    Color.A=1.0f; return Color;
}
void SNBLGraphNode::AddPin(const TSharedRef<SGraphPin>& Pin)
{
    Pin->SetShowLabel(false);
    // Keep native geometry/container glyphs; only dim unresolved wildcard pins.
    if (Pin->GetPinObj()->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard)
        Pin->SetPinColorModifier(FLinearColor(0.35f,0.35f,0.35f));
    SGraphNode::AddPin(Pin);
}
void SNBLGraphNode::UpdateGraphNode()
{
    InputPins.Empty(); OutputPins.Empty(); LeftNodeBox.Reset(); RightNodeBox.Reset();
    SetupErrorReporting();
    SAssignNew(InlineEditableText,SInlineEditableTextBlock)
        .Style(FAppStyle::Get(),"Graph.Node.NodeTitleInlineEditableText")
        .Text_Lambda([this]() { return GraphNode->GetNodeTitle(ENodeTitleType::FullTitle); })
        .OnVerifyTextChanged_Lambda([this](const FText& Text,FText& Error) { return OnVerifyNameTextChanged(Text,Error); })
        .OnTextCommitted_Lambda([this](const FText& Text,ETextCommit::Type Commit) { OnNameTextCommited(Text,Commit); })
        .IsReadOnly_Lambda([this]() { return IsNameReadOnly(); })
        .IsSelected_Lambda([this]() { return IsSelectedExclusively(); });
    GetOrAddSlot(ENodeZone::Center).HAlign(HAlign_Fill).VAlign(VAlign_Center)
    [
        SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()
        [
            SNew(SOverlay)
            +SOverlay::Slot()
            [ SNew(SImage).Image(FAppStyle::GetBrush("Graph.VarNode.Body")) ]
            +SOverlay::Slot().VAlign(VAlign_Top)
            [ SNew(SImage).Image(FAppStyle::GetBrush("Graph.VarNode.ColorSpill")).ColorAndOpacity(this,&SNBLGraphNode::GetLinkTint) ]
            +SOverlay::Slot()
            [ SNew(SImage).Image(FAppStyle::GetBrush("Graph.VarNode.Gloss")) ]
            +SOverlay::Slot().VAlign(VAlign_Top).Padding(12,2,12,0)
            [
                SNew(SBox).HeightOverride(2)
                [ SNew(SImage).Image(FAppStyle::GetBrush("WhiteBrush")).ColorAndOpacity(this,&SNBLGraphNode::GetLinkTint) ]
            ]
            +SOverlay::Slot().Padding(4,3)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SAssignNew(LeftNodeBox,SVerticalBox) ]
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4,0)
                [ SNew(SNBLLinkIcon).Tint(this,&SNBLGraphNode::GetLinkTint) ]
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2,4,6,4)
                [
                    SNew(SBox).MinDesiredWidth(48)
                    [ InlineEditableText.ToSharedRef() ]
                ]
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SAssignNew(RightNodeBox,SVerticalBox) ]
            ]
        ]
        +SVerticalBox::Slot().AutoHeight().Padding(5,1)
        [ ErrorReporting->AsWidget() ]
    ];
    TSharedPtr<SCommentBubble> Bubble;
    SAssignNew(Bubble,SCommentBubble).GraphNode(GraphNode)
        .Text(this,&SGraphNode::GetNodeComment).OnTextCommitted(this,&SGraphNode::OnCommentTextCommitted)
        .ColorAndOpacity(GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor)
        .AllowPinning(true).EnableTitleBarBubble(true).EnableBubbleCtrls(true)
        .GraphLOD(this,&SGraphNode::GetCurrentLOD).IsGraphNodeHovered(this,&SGraphNode::IsHovered);
    GetOrAddSlot(ENodeZone::TopCenter)
        .SlotOffset2f(TAttribute<FVector2f>(Bubble.Get(),&SCommentBubble::GetOffset2f))
        .SlotSize2f(TAttribute<FVector2f>(Bubble.Get(),&SCommentBubble::GetSize2f))
        .AllowScaling(TAttribute<bool>(Bubble.Get(),&SCommentBubble::IsScalingAllowed)).VAlign(VAlign_Top)
        [ Bubble.ToSharedRef() ];
    CreatePinWidgets();
}
