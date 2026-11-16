# Bugs — Icons

Scope: Portable icon resources, conversion, software drawing and optional UI use.  
ID prefix: `RWM-ICO`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `RWM-ICO-0001` | `OPEN` | Define portable icon resource descriptors and identifiers |
| `RWM-ICO-0002` | `OPEN` | Implement clipped icon rendering on the software canvas |
| `RWM-ICO-0003` | `OPEN` | Establish a reproducible icon import and build-time conversion workflow |
| `RWM-ICO-0004` | `OPEN` | Integrate optional icons into selected UI components and demo |

## Issues

### RWM-ICO-0001

**ID:** `RWM-ICO-0001`  
**Status:** `OPEN`  
**Title:** Define portable icon resource descriptors and identifiers

**Description**  
The repository has exploratory icon extraction tools, but no public icon descriptor or semantic naming convention suitable for embedded reuse.

**Details**  
**Confirmed:** `icons/README.md` and `icons/newaita/README.md` document evaluation artwork and manifests, not a toolkit icon API. Source artwork selection, licensing and the required final icon collection are not yet agreed.

**Acceptance criteria for review:** Define an icon's dimensions, pixel/mask representation, ownership/lifetime, stable identifier or reference mechanism, and rules for optional assets. Support a resource representation usable without SDL or runtime image decoders. No external artwork is required to finalize the interface.

**Related:** `RWM-ICO-0002`, `RWM-ICO-0003`, `RWM-ICO-0004`.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Compare small C-array, indexed palette and mask/alpha formats with embedded memory costs and RGB565/RGB888 behaviour. Decide a minimal, documented descriptor independent of particular icon artwork.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-ICO-0002

**ID:** `RWM-ICO-0002`  
**Status:** `OPEN`  
**Title:** Implement clipped icon rendering on the software canvas

**Description**  
There is no general canvas API for drawing icons, which applications and future chrome elements will need.

**Details**  
**Confirmed:** the public `wm_canvas_*` API in `include/retro_wm.h` provides fill/frame/text only. The renderer already handles clipping and native RGB565/RGB888 colour packing.

**Acceptance criteria for review:** Draw test icons from the portable resource contract, correctly clip to canvas and framebuffer bounds, support target pixel formats and the selected transparency mechanism, and preserve neighbouring pixels/stride padding. Include SDL-free tests for transparent and opaque pixels, partial clipping and edge coordinates.

**Dependency:** `RWM-ICO-0001`. Synthetic test artwork is sufficient; user-supplied icons are not yet required.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Introduce a canvas icon/blit operation and focused tests, reusing the existing framebuffer machinery. Select transparency semantics only after reviewing the resource format.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-ICO-0003

**ID:** `RWM-ICO-0003`  
**Status:** `OPEN`  
**Title:** Establish a reproducible icon import and build-time conversion workflow

**Description**  
Exploratory extracted images are not yet production-ready assets. Reusing them without validated source rights, masks and a build-time encoding path could create visual or redistribution problems.

**Details**  
**Confirmed:** `tools/extract_icons.py` and `tools/extract_newaita_icons.py` are documented as extraction utilities; `icons/README.md` notes unknown licensing for the montage, and `icons/newaita/README.md` notes a non-commercial ShareAlike licence and opaque image backgrounds. Those files do not establish permission to redistribute arbitrary extracted artwork.

**Acceptance criteria for review:** Given user-supplied/licensed source images, produce deterministic resources and a manifest with dimensions, names, attribution and conversion options; validate resource outputs for `RWM-ICO-0001`; define a policy for assets that may be committed or must remain local. Do not commit third-party images before rights are confirmed.

**Dependencies:** `RWM-ICO-0001` for the target format; **external dependency:** user prepares/selects icon artwork.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Extend the current extraction tools or add a separate converter for final icon resources. Keep image preparation distinct from runtime rendering and preserve provenance of source artwork.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-ICO-0004

**ID:** `RWM-ICO-0004`  
**Status:** `OPEN`  
**Title:** Integrate optional icons into selected UI components and demo

**Description**  
Icons are not yet displayed by the reusable window/menu UI or demonstration. Placement and visual roles must be agreed so icon support does not become hardcoded to a particular asset pack.

**Details**  
**Confirmed:** `src/retro_wm.c` draws title, close and resize controls procedurally; `src/retro_menu.c` draws text, checks and separators without icon resources; `examples/demo_app.c` draws text and shapes.

**Acceptance criteria for review:** Agree which placements need icons (for example menu items, window title, or client content); make absent icons harmless; preserve existing text-only appearance; keep hit testing/layout consistent with the selected icon size; demo uses the chosen prepared assets only when available.

**Dependencies:** `RWM-ICO-0001`, `RWM-ICO-0002`; production visual integration also depends on `RWM-ICO-0003` and user-provided artwork.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Add optional icon references to chosen UI descriptors, with metrics/layout review where needed. Prioritize a minimal demonstration before expanding the widget set.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
