# Bugs — Runtime

Scope: Live application of supported settings, invalidation and UI state consistency (excluding runtime display-resolution switching).  
ID prefix: `RWM-RUN`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `RWM-RUN-0001` | `OPEN` | Maintain correct layout and repaint when applying themes at runtime |

## Issues

### RWM-RUN-0001

**ID:** `RWM-RUN-0001`  
**Status:** `OPEN`  
**Title:** Maintain correct layout and repaint when applying themes at runtime

**Description**  
The current runtime theme switch changes colours safely, but a future theme that changes fonts or layout metrics will require coordinated geometry, input hit regions and repainting.

**Details**  
**Confirmed:** `wm_set_theme` copies a colour-only theme and marks windows dirty; `ui_render` passes it to menu painting. `rm_init` computes menu bounds and rows once using fixed text metrics, so adding mutable style metrics without a layout update could desynchronize rendering and interaction.

**Acceptance criteria for review:** On supported runtime theme changes, drawing and hit testing agree on current layout; menu bounds and content clipping remain valid; unsuccessful style changes leave the previous valid state; repaint/invalidation covers windows and menus. Preserve existing colour theme behaviour.

**Dependencies:** `RWM-CFG-0002`, `RWM-FNT-0002`. **Out of scope:** runtime logical resolution changes; `RWM-CFG-0001` selects size at compile time.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Decide which theme fields are runtime-mutable, then make application and layout recalculation atomic or explicitly reject unsupported live changes. Test menu-open, focus and pointer-capture states as applicable.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
