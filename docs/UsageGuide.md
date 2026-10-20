# Named Blueprint Links

By **Frainkey** · Beta 0.1.0 · Windows · Unreal Engine 5.8 (tested on 5.8.3)

Replace long Blueprint wires with named Declaration and Usage endpoints. A universal pair handles supported data types and execution connections. Names identify signals; double-clicking a Usage returns you to its Declaration.

## Installation

### Through Fab / Epic Games Launcher

When this product becomes available through Fab, install the plugin for the matching Unreal Engine version using the offered Epic installation flow. Open the project, find **Named Blueprint Links** under **Edit → Plugins**, enable it and restart if prompted.

### From a source package

If you receive a source-only package, it contains no precompiled binaries. A matching Unreal C++ build toolchain is required. Close the editor and copy the entire `NamedBlueprintLinks` folder into your project's `Plugins` folder. The descriptor must be at `YourProject/Plugins/NamedBlueprintLinks/NamedBlueprintLinks.uplugin`. Open/build the project with the matching Unreal Engine toolchain, then enable the plugin under Edit → Plugins. If Unreal cannot rebuild it automatically, build the project's Editor target through your IDE or Unreal's plugin build tools.

Keep a backup before adopting the plugin in an existing project.

## First named connection

1. Open a supported Blueprint Event Graph or function.
2. Search the graph action menu for the Named Blueprint Links Declaration.
3. Connect the source to the Declaration's input; its type/color follows the connected signal.
4. Rename the Declaration, for example `MoonPosition` or `CanInteract`.
5. Right-click the Declaration and choose **Create Linked Usage**.
6. Move the Usage near its consumer and connect its output.
7. Compile and test the Blueprint. Double-click the Usage to navigate to the Declaration.

Data permits multiple Usages and a local Declaration output. Execution permits one linked Usage and has no usable local Declaration output. Gray indicates an unresolved wildcard; white arrows indicate execution. Pin fill describes physical wires attached to that pin, not the invisible association.

## Rename, copy, delete and recover

Rename the Declaration; linked Usage labels update together. Associations use identity, not just matching text. A Usage cannot be renamed independently.

Copy a complete Declaration/Usage pair to remap the pair together. A data Usage copied alone can retain its Declaration in the same graph; pasting it into another graph leaves it unresolved. Execution Usage-only copies are deliberately unresolved. Do not use these nodes as cross-graph or cross-Blueprint variables.

Deleting a required Declaration or leaving an invalid association produces a compile error. Undo can restore the association. Compile after editing and verify the affected behavior.

## Compatibility and limits

- Tested on Windows Unreal Engine 5.8.3; other engine versions and platforms are unverified.
- Observed cases include Actor and Level Blueprint graphs/functions, widget function routing, and Animation Blueprint Event Graph routing. This does not imply support for every specialized Blueprint context.
- Macros and AnimGraph pose graphs are excluded.
- NBL pins do not offer **Split Struct Pin**. Use native producer/consumer split pins or Break/Make nodes around a whole typed link.
- Tested values include Boolean, numeric, vector, rotator, and actor/Self references. Specific integer array/set and integer-to-string map mutation cases were verified. Arbitrary reference qualifiers, container element types, delegates, networking and other untested types are not blanket-supported.
- Universal type support is a goal, not an exhaustive compatibility claim.

## Packaging and removal

Keep the plugin enabled while assets contain NBL nodes, including during cooking. Editor/uncooked modules perform the compile-time rewrite; packaged behavior has been tested in Windows Development and Shipping builds for the represented cases.

Before disabling or removing the plugin, replace its connections with ordinary wires, remove its nodes, compile and save the affected assets. Disabling it while those assets still need its node classes is not a supported uninstall method.

## Support

Report bugs and request improvements through [GitHub Issues](https://github.com/frainkey1/NamedBlueprintLinks/issues). Include your Unreal Engine version, plugin version, reproduction steps, expected and actual behavior, and relevant error text.

## Troubleshooting

For missing links, inspect the Declaration and Usage in the same graph and follow the compiler's named error. Restore the source, undo the deletion, or recreate the intended pair. Matching labels alone do not repair identity.

When reporting a problem, include engine version, platform, exact error text, reproduction steps, and a small graph screenshot. Avoid sharing private project files unless requested through the publisher's support channel.

Functions under **NBL Tests** are internal uncooked fixtures, not game features. Do not place them in project assets.
