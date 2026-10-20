// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

using UnrealBuildTool;
public class NamedBlueprintLinksEditor : ModuleRules
{
    public NamedBlueprintLinksEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Slate", "SlateCore", "GraphEditor", "NamedBlueprintLinksNodes" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "BlueprintGraph", "Kismet" });
    }
}
