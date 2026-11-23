# UE 5.0–5.9 build and test matrix

This stable release targets Win64. On 2026-11-23, every available engine branch was rebuilt from its matching tested plugin copy in an isolated host project. Each build used that engine installation’s own Build.bat. Unreal automation tests ran from the same staged plugin, and each downloadable ZIP was checked for its versioned folder, plugin descriptor, matching editor DLL, and both release screenshots.

## Results

| Engine target | User-tested installed copy | Fresh 1.0.1 engine build | 1.0.1 automation | ZIP contents |
| --- | --- | --- | --- | --- |
| UE 5.0 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.1 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.2 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.3 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.4 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.5 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.6 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.7 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.8 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |
| UE 5.9 Preview 2 | Pass — AnimGraph and duplication | Pass | 13/13 pass | Pass |

The installed-copy results are the user’s manual checks of the direct per-engine copies, including working AnimGraphs and duplication. Fresh builds and automated tests were then run against the 1.0.1 staged packages. These automated checks confirm compilation and the plugin test suite; they are not a substitute for a human visual check of every packaged editor UI.

## Package names

The public release attaches one Win64 ZIP per engine:

- `NBLv.1.0.1UE5.0.zip` through `NBLv.1.0.1UE5.8.zip`
- `NBLv.1.0.1UE5.9P2.zip` for Unreal Engine 5.9 Preview 2

Each archive contains the matching top-level folder, plugin source, matching engine binaries, documentation, and both screenshots. Never copy binaries from one engine package into another. The release package for each version is the official installable copy; keep its folder name when extracting.
