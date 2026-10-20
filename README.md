# Named Blueprint Links

By **Frainkey** · UDU: Universal Declaration/Usage

Named Blueprint Links replaces long Blueprint wires with named Declaration and Usage endpoints. Use descriptive names, place Usages near their consumers, and double-click a Usage to return to its Declaration.

**Status:** prerelease preparation for a free Fab listing. The plugin is not published on Fab yet. This repository hosts documentation and issue reporting; it currently does not distribute plugin source or binaries.

## Documentation

- [Usage guide, installation and limitations](docs/UsageGuide.md)
- [Report a bug or request an improvement](https://github.com/frainkey1/NamedBlueprintLinks/issues)

## Features

- One universal Declaration/Usage pair for supported data and execution connections.
- Multiple data Usages and a local Declaration data output.
- One linked Usage in execution mode.
- Declaration rename synchronization and double-click navigation.
- Compact type-colored nodes, clipboard handling, and undo/redo support.

## Tested scope

Windows, Unreal Engine **5.8.3**. Tested cases include Actor/Level Blueprint graphs and functions, widget function routing, Animation Blueprint Event Graph routing, and represented Development/Shipping packaged cases. Other platforms, engine versions and specialized contexts are unverified. Macros and AnimGraph pose graphs are excluded. NBL pins do not offer Split Struct Pin; use native split pins or Break/Make nodes around links.

Universal support is a goal, not a claim that every Blueprint type and context has been verified.

## Reporting problems

Use the bug-report template in Issues. Include your plugin/engine versions, graph context, steps to reproduce, expected/actual behavior and error text. A small screenshot or minimal reproduction is helpful. GitHub issues are public; remove private project details and credentials from attachments.
