# Tested scope

## Stable 1.0.1 release validation — 2026-11-23

All ten engine targets (UE 5.0–5.8 and UE 5.9 Preview 2) built from their matching per-engine plugin copy with the corresponding installed engine. Each run compiled the plugin through an isolated host project. All 13 `NamedBlueprintLinks` Unreal automation tests passed on every target (130 test executions total). Each ZIP was opened and checked for its matching top-level folder, `.uplugin` descriptor, `UnrealEditor-NamedBlueprintLinksEditor.dll`, and both supplied screenshots.

The user had already installed and manually checked the organized per-engine plugin copies, reporting working AnimGraphs, duplication, and related behavior. This manual confirmation refers to those working copies; the fresh 1.0.1 checks separately cover the staged release source, compilation, automation, and ZIP contents.

The C++ routing/presentation baseline was built with Unreal Engine 5.8.3 on Windows. The latest beta installation passed all 13 Unreal automation tests and the combined Standalone check after reopening the project. The portable production routing planner passed 16 assertions on Linux; this is not a Linux Unreal compatibility claim.

Manual tests covered authoring, native rename, type/color/shape changes, copy/paste, whole Blueprint and function duplication, deletion/undo/redo, save/restart, and Usage navigation. Specific function values, array/set/map mutation, whole vectors/native split producer pins, chained data and execution links, delay/loop branching, widget functions and animation Event Graph routing were exercised.

A combined Windows Shipping harness verified function result42, original array length4/index3=40, original set membership40, map Found/value UpdatedForty, positive routed animation delta, more than one recorded animation update, and exact recorded loop order02. Independent editor negative controls for animation thresholds and loop order produced FAIL; restored expected values produced PASS. Shipping and Development packages were launched successfully for the represented cases.

Beta.2 passed a fresh Windows UE5.8.3 BuildPlugin build and all 13 Unreal automation tests (report 6b54a792b5704dc18de876cf9520a2c2). The installed Plugins panel showed beta.2, Frainkey and the approved icon. The combined runtime harness reached PASS. Context-insensitive Probe/Record action searches showed no NBL test fixtures.

Beta.3 passed the Windows BuildPlugin/13-test run (report f6bb0f98ea1a458dbe228e69fe70bc8b); screenshots2153-2155 confirm installed beta.3 and combined runtime PASS. The 1.0.1 release adds executable pose-link presentation and copy/paste lifecycle behavior; see the release validation and compatibility matrix above. The plugin does not claim certification for every Blueprint type or context.
