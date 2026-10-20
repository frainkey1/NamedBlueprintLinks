#include "NBLRoutePlanner.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace NBL;
static int Checks = 0;
static void Check(bool Value, const char* Message)
{
    ++Checks;
    if (!Value) { std::cerr << "FAIL: " << Message << '\n'; std::exit(1); }
}
static bool Has(const Plan& P, int Source, int Destination)
{
    return std::find(P.Edges.begin(), P.Edges.end(), Edge{Source, Destination}) != P.Edges.end();
}
int main()
{
    Snapshot G;
    G.Nodes = {{"d", "", Kind::DataDeclaration, 10, 11},
               {"u", "d", Kind::DataUsage, 0, 21},
               {"v", "d", Kind::DataUsage, 0, 31}};
    G.Edges = {{1,10}, {11,2}, {21,3}, {31,4}, {5,6}};
    auto P = BuildPlan(G);
    Check(P.Ok(), "valid data pair must resolve");
    Check(P.Edges.size() == 4 && Has(P,1,2) && Has(P,1,3) && Has(P,1,4) && Has(P,5,6),
          "local output plus multiple Usages route to original source and preserve ordinary edges");
    Check(G.Edges.size() == 5 && G.Nodes.size() == 3, "planning does not mutate saved graph");
    auto Reordered = G;
    std::reverse(Reordered.Nodes.begin(), Reordered.Nodes.end());
    Check(BuildPlan(Reordered).Edges == P.Edges, "routing independent of node insertion order");

    Snapshot Exec{{{"d","",Kind::ExecDeclaration,10,0}, {"u","d",Kind::ExecUsage,0,21}},
                  {{1,10},{21,2}}};
    P = BuildPlan(Exec);
    Check(P.Ok() && P.Edges.size()==1 && Has(P,1,2), "execution is one ordinary connection");
    Exec.Edges.push_back({3,10});
    P = BuildPlan(Exec);
    Check(P.Ok() && Has(P,1,2) && Has(P,3,2), "multiple permitted incoming exec paths retained");
    Exec.Nodes.push_back({"v","d",Kind::ExecUsage,0,31});
    Check(!BuildPlan(Exec).Ok(), "second execution Usage rejected");

    auto Missing = G; Missing.Nodes[1].Target = "absent";
    Check(!BuildPlan(Missing).Ok(), "missing declaration cannot produce a default");
    auto Duplicate = G; Duplicate.Nodes[1].Id = "d";
    Check(!BuildPlan(Duplicate).Ok(), "duplicate endpoint identity rejected");
    auto Mismatch = G; Mismatch.Nodes[1].Type = Kind::ExecUsage;
    Check(!BuildPlan(Mismatch).Ok(), "data/exec mismatch rejected");
    auto NoInput = G; NoInput.Edges.erase(NoInput.Edges.begin());
    Check(!BuildPlan(NoInput).Ok(), "consumed declaration with no source rejected");
    auto DoubleInput = G; DoubleInput.Edges.push_back({7,10});
    Check(!BuildPlan(DoubleInput).Ok(), "multiple data sources rejected");

    Snapshot Chain{{{"a","",Kind::DataDeclaration,10,11}, {"b","",Kind::DataDeclaration,20,22},
                    {"u","a",Kind::DataUsage,0,31}, {"v","b",Kind::DataUsage,0,41}},
                   {{1,10},{31,20},{41,2}}};
    P = BuildPlan(Chain);
    Check(P.Ok() && P.Edges.size()==1 && Has(P,1,2), "chain of named links collapses to source");
    Chain.Edges[0] = {41,10};
    Check(!BuildPlan(Chain).Ok(), "routing-only cycle rejected without recursive overflow");
    Snapshot Ordinary{{}, {{1,2},{2,1}}};
    P = BuildPlan(Ordinary);
    Check(P.Ok() && P.Edges == Ordinary.Edges, "ordinary graph loop is left to engine schema");
    auto Repeat = BuildPlan(G);
    Check(Repeat.Edges == BuildPlan(G).Edges, "repeated compilation planning stable");
    std::cout << "PASS: " << Checks << " routing checks\n";
}
