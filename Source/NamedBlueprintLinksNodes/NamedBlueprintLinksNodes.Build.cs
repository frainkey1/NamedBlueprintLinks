// Copyright (c) 2026 Frainkey. Licensed under the MIT License.

using UnrealBuildTool;

public class NamedBlueprintLinksNodes : ModuleRules
{
    public NamedBlueprintLinksNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "BlueprintGraph" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "KismetCompiler", "Kismet", "ToolMenus", "Slate", "SlateCore" });
    }
}
