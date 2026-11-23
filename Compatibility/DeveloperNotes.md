# Developer notes

## Modules and dependencies

The plugin has two modules:

- `NamedBlueprintLinksNodes` contains the Blueprint node classes, identity/lifecycle handling, validation, and compiler rewrite. Its private dependencies include UnrealEd, KismetCompiler, Kismet, ToolMenus, Slate, SlateCore, and AnimGraph. The descriptor marks it `UncookedOnly`.
- `NamedBlueprintLinksEditor` contains graph presentation and editor factories. Its public module dependencies include Core, CoreUObject, Engine, Slate, SlateCore, GraphEditor, and NamedBlueprintLinksNodes. Its private dependencies include UnrealEd, BlueprintGraph, Kismet, and AnimGraph.

AnimGraph is used by the node module to recognize pose pins and maintain the hidden pose bridge, and by the editor module for the Anim Blueprint compile extension and pose pulse drawing. Event Graph execution pulse rendering remains Unreal’s native Kismet connection policy.

## Version-specific build notes

The plugin descriptor does not pin the plugin to one engine version. The tested per-engine source copies have three `K2Node_NBLBase.cpp` variants: UE 5.0–5.4 share one form, UE 5.5–5.7 and UE 5.9 Preview 2 share another, and UE 5.8 has a distinct pass-through implementation. The Anim Blueprint extension and resolver sources are shared across those copies. Preserve each engine’s own package source and binaries; do not replace one target’s binaries with another’s.

UE 5.6 through UE 5.9 Preview 2 builds set C++20 in both plugin modules. UE 5.6, UE 5.8, and UE 5.9 Preview 2 were built with MSVC 14.50; UE 5.7 used its accepted default compiler. The UE 5.8 and 5.9 Preview 2 temporary host projects disable the unrelated Fab and FabLauncher engine plugins during validation. Those host-only settings are not part of the plugin packages.

The Desktop copies provide the working behavior baseline. Each official 1.0.1 archive contains that engine’s source variant and freshly compiled binaries. See [BuildMatrix.md](BuildMatrix.md) for the build and automation results.

UE 5.9 support is specifically for Preview 2. It does not imply support for later 5.9 previews or the final 5.9 release.

## Copy/paste naming and identity

A copied Declaration receives a fresh Declaration ID; copied Usages that were part of the same copied set are remapped to the new ID. Data Usage-only copies may resolve to their original Declaration within the source graph. Execution Usage-only copies remain unresolved, preserving the single-Usage rule.

The serialized `bUsesDefaultLinkName` flag distinguishes generated fallback labels from user labels. It is set on new Declaration nodes and cleared on rename. Only generated fallback names are uniquified when pasted; custom names are preserved exactly. Existing serialized nodes without this flag default to custom-name handling for backward compatibility.

## Pulse drawing

The editor module registers the Anim Blueprint extension when it constructs a named node in an Animation Graph. The extension rewrites pose-link routes in the compiler’s consolidated graph. A separate pose connection factory is scoped to Animation Graph schemas. It draws a white animated overlay on named pose connections when the Blueprint status is not `BS_UpToDate`; this reflects Blueprint compile status and does not diagnose whether an individual link is broken or stale. Pin geometry is required to draw the overlay. The base Kismet connection policy still uses animation debug data for live evaluation. Event Graph execution connections continue through Unreal’s normal renderer and retain the native red/orange pulse.

When changing engine APIs, compile the version-specific source with each target engine and add version guards only for a compiler-confirmed difference.
