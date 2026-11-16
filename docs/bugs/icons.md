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
| `RWM-ICO-0003` | `IN_PROGRESS` | Establish a reproducible icon import and build-time conversion workflow |
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
**Status:** `IN_PROGRESS`  
**Title:** Establish a reproducible icon import and build-time conversion workflow

**Description**  
Exploratory extracted images are not yet production-ready assets. Reusing them without validated source rights, masks and a build-time encoding path could create visual or redistribution problems.

**Details**  
**Confirmed:** `tools/extract_icons.py` and `tools/extract_newaita_icons.py` are documented as extraction utilities; `icons/README.md` notes unknown licensing for the montage, and `icons/newaita/README.md` notes a non-commercial ShareAlike licence and opaque image backgrounds. Those files do not establish permission to redistribute arbitrary extracted artwork.

**Acceptance criteria for review:** Given user-supplied/licensed source images, produce deterministic resources and a manifest with dimensions, names, attribution and conversion options; validate resource outputs for `RWM-ICO-0001`; define a policy for assets that may be committed or must remain local. Do not commit third-party images before rights are confirmed.

**Dependencies:** `RWM-ICO-0001` for the target format; **external dependency:** user prepares/selects icon artwork.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.
- **2026-11-16 — Investigation:** Inspected seven selected 88×88 RGB PNGs on the icon-devel branch. Their content bounds and surrounding opaque white areas differ; measured candidate bounds are recorded in icons/images/README.md. No source PNGs were modified.
- **2026-11-16 — Decision:** Use a preliminary 64×64 RGB565 output canvas with aspect-ratio-preserving scaling, 3-pixel destination padding, and editable crop proposals based on near-white thresholding and source padding. Default output is opaque little-endian RGB565; transparency requires separate review under RWM-ICO-0001/0002.
- **2026-11-16 — Implementation:** Added tools/convert_icons_rgb565.py, icons/images/crop-proposals.json, icons/images/README.md, tests/test_convert_icons_rgb565.py and ignored generated output. The converter produces headerless binary files, quantized previews, a manifest, and a crop contact sheet.
- **2026-11-16 — Test:** Four synthetic Pillow-based unit tests passed locally: RGB565 packing/endianness, crop bounds/layout, invalid crop validation and RGBA-matte batch output. Actual source PNGs were inspected and content bounds measured separately; running the Python converter against all seven repo PNGs and manual review of generated previews remain outstanding.
- **2026-11-16 — Status Change:** OPEN → IN_PROGRESS. This initial conversion utility is not accepted as the complete icon asset pipeline. Review and further asset/licence decisions are pending.
- **2026-11-16 — Decision:** Trial four-neighbour flood-fill from the source image border, selecting only near-white connected background pixels. Reserve RGB565 word `0xF81F` as a proposed transparency key rather than treating white (`0xFFFF`) as transparent. Retain enclosed white details; fail conversion if an opaque source colour quantizes to the reserved key.
- **2026-11-16 — Implementation:** Extended the converter with configurable exterior flood tolerance, binary alpha cutoff, 16-bit colour key, legacy opaque mode, transparent PNG previews and checkerboard contact sheet. The raw RGB565 still has no alpha; runtime skip-key support remains a separate `RWM-ICO-0001/0002` design decision.
- **2026-11-16 — Test:** Added synthetic unit cases for enclosed-white preservation, near-white flood thresholds, RGB565 endianness/key preservation, and opaque-key collisions. Execution against the updated script and visual review of all seven PNG outputs remain **not yet verified**; no code-review approval is claimed.
- **2026-11-16 — Decision:** Keep automatic flood tolerance conservative to protect pale icon details. Introduce an editable post-conversion pixel map, with `#` for opaque RGB565 pixels and `.` for colour-key transparency. A manual cleanup pass must only change `#` to `.`, preserving every other pixel word. This supersedes a proposed automatic disconnected-component removal as the immediate cleanup workflow.
- **2026-11-16 — Implementation:** Default colour-key conversion now creates non-destructively named `icons/masks/*.txt` grids and companion SHA-256 metadata. `--cleanup` applies user edits only after checking dimensions, key, endianness, baseline hash and no transparent-to-opaque changes. `--reset-maps` is an explicit, destructive regeneration operation. Source PNGs and other RGB565 pixels are unchanged.
- **2026-11-16 — Test:** Added regression tests for manual removal of a selected pixel, editable-map persistence, PNG preview transparency, corrupted/stale map rejection, and explicit regeneration. Tests have been committed but **not executed in this session**; target PNG review remains pending.
- **2026-11-16 — Decision:** Following user review, reverse the prior removal-only restriction: manual pixel-map edits now support both `#` → `.` and `.` → `#`. To restore a mistakenly transparent pixel, reconstruct its original quantized RGB565 colour from the same input PNG/crop/scale rather than inferring it from the colour key. Retain existing baseline-hash verification and non-destructive map handling.
- **2026-11-16 — Implementation:** `--cleanup` now separately renders an opaque RGB565 colour reference for restoration. It writes the key for removed pixels and the source-derived RGB565 colour for restored pixels, without changing unedited words. A restored pixel colliding with the reserved key is rejected. The manifest distinguishes `cleanup_removed_pixels` and `cleanup_restored_pixels`.
- **2026-11-16 — Test:** Added regression coverage for both simultaneous edit directions, non-white source-colour restoration, big-endian RGB565, reserved-key collisions, and invalid reference size. Automated tests remain unexecuted in this session; code review and seven-icon visual acceptance remain pending.


**Proposed Change**  
Extend the current extraction tools or add a separate converter for final icon resources. Keep image preparation distinct from runtime rendering and preserve provenance of source artwork.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `icon-devel` | [`1ea7402`](https://github.com/keroblabs/retro-wm/commit/1ea740220737149db3df917687cfd0b17e70a65e) | `UNREVIEWED` | Add crop analysis and RGB565 conversion script |
| `icon-devel` | [`77a012a`](https://github.com/keroblabs/retro-wm/commit/77a012aa4d61a172621e85ef52a0eef2258c207a) | `UNREVIEWED` | Add synthetic conversion tests |
| `icon-devel` | [`a570789`](https://github.com/keroblabs/retro-wm/commit/a57078939185e55167685e84abebc48716b611a4) | `UNREVIEWED` | Record editable proposed crop rectangles |
| `icon-devel` | [`ca67790`](https://github.com/keroblabs/retro-wm/commit/ca67790db49405d86016c38dfbee96ad4fccd763) | `UNREVIEWED` | Document analysis, processing and review workflow |
| `icon-devel` | [`8df32fd`](https://github.com/keroblabs/retro-wm/commit/8df32fd96699a80ebccf45200574707b8a7fd20a) | `UNREVIEWED` | Ignore generated RGB565 assets |
| `icon-devel` | [`e75dd11`](https://github.com/keroblabs/retro-wm/commit/e75dd117ab09fd91dd67f6ac266fb3b8ece7fffe) | `UNREVIEWED` | Add flood-based RGB565 colour-key transparency |
| `icon-devel` | [`baf9e48`](https://github.com/keroblabs/retro-wm/commit/baf9e489ae7589a27af11db3921af1c09cc2684d) | `UNREVIEWED` | Extend tests for flood boundary, white interiors and key collisions |
| `icon-devel` | [`edc2f6b`](https://github.com/keroblabs/retro-wm/commit/edc2f6b8527d5a4220e6b74db3d90c115bfe9d15) | `UNREVIEWED` | Simplify opaque packing compatibility test |
| `icon-devel` | [`94128ce`](https://github.com/keroblabs/retro-wm/commit/94128ced6c0b801f72eb1cfc416493f1c735e2c4) | `UNREVIEWED` | Document colour-key format, flood controls and limitations |
| `icon-devel` | [`c057ed5`](https://github.com/keroblabs/retro-wm/commit/c057ed56609eb98d6b2a60a69c47573689022806) | `UNREVIEWED` | Retain RGB source lifetime during flood pixel access |
| `icon-devel` | [`6d729bb`](https://github.com/keroblabs/retro-wm/commit/6d729bb9c607597bd01dccd9ce1a635e8e455af8) | `UNREVIEWED` | Export editable per-icon pixel maps and implement manual cleanup |
| `icon-devel` | [`06fe5cd`](https://github.com/keroblabs/retro-wm/commit/06fe5cd064bf0b01bda171be9a3f4f7a40d61b8b) | `UNREVIEWED` | Add tests for cleanup persistence and stale-mask protection |
| `icon-devel` | [`155bce9`](https://github.com/keroblabs/retro-wm/commit/155bce9e71414889a47a94546e24ec0b5647fcc8) | `UNREVIEWED` | Document editable pixel map and manual cleanup steps |
| `icon-devel` | [`9d96da0`](https://github.com/keroblabs/retro-wm/commit/9d96da02194daa6eadda2a7dbdd5a62c4208612e) | `UNREVIEWED` | Restore `.` → `#` pixels from original PNG colour data |
| `icon-devel` | [`c65c601`](https://github.com/keroblabs/retro-wm/commit/c65c601061eb39db3fb674bda832ac9c621ed26d) | `UNREVIEWED` | Test bidirectional edits, restoration and collision checks |
| `icon-devel` | [`1062b3c`](https://github.com/keroblabs/retro-wm/commit/1062b3c86e1decabc8e60fb3399a8be03b1e9ede) | `UNREVIEWED` | Document restoring previously transparent pixels |

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
