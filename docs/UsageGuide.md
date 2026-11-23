# Named Blueprint Links 1.0.1

Named Blueprint Links replaces long Blueprint connections with named Declaration and Usage nodes. The association uses a unique Declaration ID; matching text by itself does not connect nodes.

## Install a release package

1. Download the ZIP for your exact engine from [GitHub Releases](https://github.com/frainkey1/NamedBlueprintLinks/releases).
2. Extract it into the project’s `Plugins` folder and keep the archive’s top-level folder name.
3. Close and reopen the project, then enable **Named Blueprint Links** under **Edit → Plugins**.
4. Let Unreal compile the Win64 plugin for the matching engine if it prompts.

Release ZIPs use `NBLv.1.0.1UE5.0` through `NBLv.1.0.1UE5.8`; UE 5.9 Preview 2 uses `NBLv.1.0.1UE5.9P2`. Do not use the 5.9 Preview 2 package with another 5.9 preview or final engine build. Results and scope are in [Compatibility/BuildMatrix.md](../Compatibility/BuildMatrix.md).

## Create and connect a link

1. In a supported graph, open the Blueprint action menu and add **Named Link Declaration**.
2. Connect a producer to the Declaration input. Data links follow the source type; execution links carry flow.
3. Rename the Declaration to identify the signal.
4. Right-click it and select **Create Linked Usage**.
5. Place the Usage near its consumer and connect its output.
6. Compile and test the Blueprint. Double-click the Usage to jump to the Declaration.

A data Declaration may have multiple Usages and can also connect to a consumer locally. An execution Declaration permits one linked Usage and has no usable local output. A named pose link carries a pose through an Animation Blueprint AnimGraph.

## Rename and duplicate

Rename the Declaration to update its linked Usage labels. Names are descriptive labels; the unique Declaration identity controls the association.

Copy the entire Declaration/Usage pair to duplicate a link. The copied Declaration gets a fresh identity and copied Usages are remapped to it. Custom names remain unchanged, even when they end in a number or underscore. Generated fallback names use `Link`, `Link1`, `Link2`, and onward.

A data Usage copied by itself can keep resolving to its original Declaration only in the same graph. In another graph it remains unresolved. An execution Usage copied by itself stays unresolved, preserving the one-Usage rule.

## Pulse display

- **Event Graph:** execution connections use Unreal’s native red/orange execution pulse.
- **AnimGraph:** named pose connections use a white pose-link pulse when stale animation data is being evaluated.

The custom AnimGraph connection drawing policy is scoped to pose connections. It does not replace the Event Graph’s native execution pulse behavior.

## Known limits

- The release packages target Win64. See the compatibility matrix for engine-specific evidence.
- UE 5.9 support is limited to Preview 2.
- Specialized Blueprint contexts and every possible type are not certified.
- NBL pins cannot be split with **Split Struct Pin**. Split native producer/consumer pins or use Break/Make nodes around a whole typed link.
- Do not use Named Blueprint Links as cross-graph or cross-Blueprint variables.
- Keep the plugin enabled while project assets contain its nodes, including during cooking.

To remove it safely, replace the named connections with ordinary wires, delete the plugin nodes, compile, and save affected assets before disabling the plugin.

## Help and support

For node behavior or installation issues, file a report in [GitHub Issues](https://github.com/frainkey1/NamedBlueprintLinks/issues) with your engine version, plugin version, and reproduction steps. Unreal’s plugin browser **Support** button opens the [PayPal support page](https://www.paypal.com/donate/?hosted_button_id=RWT3F8XL7TTPE).
