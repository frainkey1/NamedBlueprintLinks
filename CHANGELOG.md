# Changelog

## 1.0.1 — Stable

The project is out of beta.

- Added a pale white line and animated pulse on named pose connections while the owning Animation Blueprint is not up to date (compile-stale). This reflects whole-Blueprint compile status; an individual stale or unresolved link is a separate state.
- Preserved Unreal’s native red/orange pulse display for Event Graph execution connections.
- Improved full-pair duplication: copied links receive fresh identity and linked Usages remap to the new Declaration.
- Preserved user-entered names during duplication, including numeric and underscore suffixes. Generated fallback names use compact `Link`, `Link1`, `Link2` naming.
- Added per-engine Win64 release packages for Unreal Engine 5.0–5.8 and Unreal Engine 5.9 Preview 2.
- Set the Unreal plugin browser Support button to the PayPal support page. GitHub Issues remains the bug-report and feature-request channel.

Engine-by-engine build, automation, and runtime evidence is recorded in [Compatibility/BuildMatrix.md](Compatibility/BuildMatrix.md). Unreal Engine 5.9 is limited to Preview 2.
