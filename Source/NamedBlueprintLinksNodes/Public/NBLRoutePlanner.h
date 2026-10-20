// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace NBL
{
enum class Kind { DataDeclaration, DataUsage, ExecDeclaration, ExecUsage };
struct Edge
{
    int Source = 0;
    int Destination = 0;
    bool operator==(const Edge& Other) const { return Source == Other.Source && Destination == Other.Destination; }
};
struct Endpoint
{
    std::string Id;
    std::string Target;
    Kind Type;
    int Input = 0;
    int Output = 0;
};
struct Snapshot { std::vector<Endpoint> Nodes; std::vector<Edge> Edges; };
struct Plan
{
    std::vector<Edge> Edges;
    std::string Error;
    std::string ErrorNode;
    bool Ok() const { return Error.empty(); }
};
inline Plan BuildPlan(const Snapshot& Graph)
{
    Plan Result;
    std::unordered_map<std::string, const Endpoint*> ById;
    std::unordered_map<int, const Endpoint*> Outputs;
    std::unordered_set<int> OwnedPins;
    std::unordered_map<int, std::vector<int>> Incoming;
    std::unordered_map<std::string, int> ExecUses;
    auto Fail = [&](const Endpoint& Node, const char* Message)
    {
        Result.Error = Message; Result.ErrorNode = Node.Id;
    };
    for (const auto& Node : Graph.Nodes)
    {
        if (Node.Id.empty() || !ById.emplace(Node.Id, &Node).second)
        { Fail(Node, "Missing or duplicate endpoint identity"); return Result; }
        const bool Declaration = Node.Type == Kind::DataDeclaration || Node.Type == Kind::ExecDeclaration;
        if ((Declaration && Node.Input <= 0) ||
            (Node.Type != Kind::ExecDeclaration && Node.Output <= 0) ||
            (!Declaration && Node.Input != 0) || (Node.Type == Kind::ExecDeclaration && Node.Output != 0))
        { Fail(Node, "Invalid endpoint pins"); return Result; }
        for (int Pin : {Node.Input, Node.Output})
        {
            if (Pin > 0 && !OwnedPins.insert(Pin).second)
            { Fail(Node, "Duplicate routing pin identity"); return Result; }
        }
        if (Node.Output > 0) Outputs.emplace(Node.Output, &Node);
    }
    for (const auto& Node : Graph.Nodes)
    {
        if (Node.Type == Kind::DataDeclaration || Node.Type == Kind::ExecDeclaration) continue;
        const auto Found = ById.find(Node.Target);
        if (Found == ById.end())
        { Fail(Node, "Missing declaration in this graph"); return Result; }
        const Kind Expected = Node.Type == Kind::DataUsage ? Kind::DataDeclaration : Kind::ExecDeclaration;
        if (Found->second->Type != Expected)
        { Fail(Node, "Usage and declaration categories differ"); return Result; }
        if (Node.Type == Kind::ExecUsage && ++ExecUses[Node.Target] > 1)
        { Fail(Node, "Execution declarations permit only one Usage"); return Result; }
    }
    for (const auto& Wire : Graph.Edges) Incoming[Wire.Destination].push_back(Wire.Source);

    // Resolve named-only paths without traversing ordinary nodes. Ordinary loops
    // remain the compiler/schema's concern; no variable or evaluation cache exists.
    std::unordered_set<int> Visiting;
    std::function<bool(int, std::vector<int>&)> Resolve;
    Resolve = [&](int Pin, std::vector<int>& Sources)
    {
        const auto Found = Outputs.find(Pin);
        if (Found == Outputs.end())
        {
            if (OwnedPins.count(Pin))
            { Result.Error = "Routing input used as a source"; return false; }
            Sources.push_back(Pin); return true;
        }
        const Endpoint& Node = *Found->second;
        if (Visiting.size() >= 512 || !Visiting.insert(Pin).second)
        { Fail(Node, "Routing cycle or chain exceeds prototype limit of 512"); return false; }
        const Endpoint* Declaration = &Node;
        if (Node.Type == Kind::DataUsage || Node.Type == Kind::ExecUsage)
            Declaration = ById.find(Node.Target)->second;
        const auto& Inputs = Incoming[Declaration->Input];
        if (Inputs.empty() || (Declaration->Type == Kind::DataDeclaration && Inputs.size() != 1))
        { Fail(*Declaration, "Consumed declaration needs a valid connected source"); return false; }
        for (int Source : Inputs) if (!Resolve(Source, Sources)) return false;
        Visiting.erase(Pin);
        return true;
    };
    for (const auto& Wire : Graph.Edges)
    {
        if (OwnedPins.count(Wire.Destination)) continue;
        std::vector<int> Sources;
        if (!Resolve(Wire.Source, Sources)) { Result.Edges.clear(); return Result; }
        for (int Source : Sources)
        {
            Edge Replacement{Source, Wire.Destination};
            if (std::find(Result.Edges.begin(), Result.Edges.end(), Replacement) == Result.Edges.end())
                Result.Edges.push_back(Replacement);
        }
    }
    return Result;
}
}
