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
| `RWM-ICO-0001` | `IN_PROGRESS` | Define portable icon resource descriptors and identifiers |
| `RWM-ICO-0002` | `OPEN` | Implement clipped icon rendering on the software canvas |
| `RWM-ICO-0003` | `IN_PROGRESS` | Establish a reproducible icon import and build-time conversion workflow |
| `RWM-ICO-0004` | `OPEN` | Integrate optional icons into selected UI components and demo |
| `RWM-ICO-0005` | `OPEN` | Demonstrate three desktop icons with selection and click activation |

## Issues

### RWM-ICO-0001

**ID:** `RWM-ICO-0001`  
**Status:** `IN_PROGRESS`  
**Title:** Define portable icon resource descriptors and identifiers

**Description**  
The repository has exploratory icon extraction tools, but no public icon descriptor or semantic naming convention suitable for embedded reuse.

**Details**  
**Baseline evidence:** `icons/README.md` and `icons/newaita/README.md` document evaluation artwork; there was no toolkit icon API before this working branch. The shared header `include/retro_icon.h` is now a **preliminary implementation**, but validation, renderer integration and portability review are outstanding.

**Required resource contract (working proposal):**
- `icon_t` stores `uint16_t width, height`, `uint8_t pixel_type`, `uint32_t colour_key` and `const void *icon_pixels`. The pointer is `void*` rather than `char*` so generated typed C arrays can be referenced without reinterpretation; data is immutable.
- `wm_icon_pixel_type`: `WM_ICON_PIXEL_1BPP = 1`, `WM_ICON_PIXEL_RGB565 = 2`, `WM_ICON_PIXEL_RGB888 = 3`. This is deliberately separate from the **compile-time framebuffer macros** `WM_PIXEL_FORMAT_RGB565 = 565` and `WM_PIXEL_FORMAT_RGB888 = 888`, whose numeric values exceed `uint8_t`.
- 1bpp is row-packed MSB-first, byte-aligned per scanline; a **zero bit is transparent**, a one bit requires the caller's chosen foreground colour. `colour_key` is irrelevant for 1bpp.
- RGB565 stores native `uint16_t` pixels; a 16-bit value equal to `colour_key` (currently `0xF81F`) means transparent, preserving opaque white (`0xFFFF`). RGB888 stores native `uint32_t` pixels as `0x00RRGGBB`; `colour_key` is compared over its valid 24-bit colour. `WM_ICON_COLOUR_KEY_NONE = UINT32_MAX` disables keying for opaque colour icons.
- `tools/convert_icons_rgb565.py` emits `icons/converted/c/<icon>.h/.c` containing one `uint16_t` array and one `icon_t` object **after** any manual pixel-mask cleanup. Example: `icon_drive_pixels[]` and `icon_drive`. C array literals are host-endian word **values**, whereas raw .rgb565 byte order is separately configurable. The generator currently emits **RGB565 only**; 1bpp/RGB888 are supported by the descriptor contract, not yet generated.
- The icon source format is independent from the renderer's compile-time framebuffer format; conversions and clipping are handled by `RWM-ICO-0002`. The renderer must not assume icon data always matches `wm_pixel` or reference SDL.

**Acceptance criteria for review:** Verify C11 and embedded compatibility, exact memory layout, 1bpp row pitch and foreground contract, RGB565/RGB888 conversion, unaligned/invalid inputs and sentinel/colour-key handling; compile generated assets under RGB565 and RGB888 builds; retain constant/static resource ownership; document whether semantic IDs or application-owned icon pointers are preferred. The descriptor and generator are not proof of runtime support.

**Related:** `RWM-ICO-0002` renderer, `RWM-ICO-0003` resource generation, `RWM-ICO-0004` general UI support, and `RWM-ICO-0005` demo.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.
- **2026-11-16 — Review:** Retro WM has `WM_PIXEL_FORMAT_RGB565 = 565` and `WM_PIXEL_FORMAT_RGB888 = 888` for compile-time framebuffer selection, not a resource-format enum. Since the proposed `pixel_type` is a byte, dedicated icon values 1/2/3 were selected. Use `const void *` for typed static data, not `const char *`.
- **2026-11-16 — Implementation:** Introduced initial `include/retro_icon.h` with the proposed enum, `icon_t` descriptor and no-key sentinel. The generated source uses this contract, but drawing and validation APIs are not implemented.
- **2026-11-16 — Status Change:** OPEN → IN_PROGRESS due to the preliminary resource header. Renderer integration, review and acceptance remain pending.


**Proposed Change**  
Use `include/retro_icon.h` and the C resources generated by `RWM-ICO-0003` as the basis for the eventual icon API. Review the enum and descriptor details above, add explicit validation/format documentation, and coordinate pixel conversion and transparency behaviour with `RWM-ICO-0002`. Do not make the renderer depend on PNG/Pillow or on a single icon colour depth.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `icon-devel` | [`849724f`](https://github.com/keroblabs/retro-wm/commit/849724faf8296eee6b62a5bce4ce702c7931521d) | `UNREVIEWED` | Add preliminary `icon_t` resource type and icon pixel-format enum |

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
- **2026-11-16 — Decision:** Produce a native C array plus a descriptor for **each** converted icon, using the shared `icon_t` definition in `include/retro_icon.h`. Generate from the final RGB565 pixels after manual `--cleanup`; preserve the existing raw .rgb565 exports. The C arrays contain typed `uint16_t` words, independent of raw byte endianness.
- **2026-11-16 — Implementation:** `tools/convert_icons_rgb565.py` now writes reproducible `icons/converted/c/<name>.c/.h` with one static `uint16_t` pixel array and a public `const icon_t` object for each image. `--no-c` can suppress C generation. The manifest names the source/header and C symbol; symbol collisions and invalid dimensions are checked.
- **2026-11-16 — Test:** Added checks for generated descriptors, RGB565 word values with both raw byte orders, opaque no-key mode, invalid inputs, and optional native-C compilation. Test suite requires execution on the checkout and code review before acceptance.


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
| `icon-devel` | [`33906ce`](https://github.com/keroblabs/retro-wm/commit/33906cef9b0e7a98bb5847dd2486710f549f5f4f) | `UNREVIEWED` | Generate C source/header pairs and `icon_t` descriptors |
| `icon-devel` | [`d30c23d`](https://github.com/keroblabs/retro-wm/commit/d30c23dcbee60491140164f31f6b2b725ecf4c98) | `UNREVIEWED` | Test C array word values, byte-order independence and fields |
| `icon-devel` | [`d94484d`](https://github.com/keroblabs/retro-wm/commit/d94484df69cd5c5a4b712fd33f6378934ff90de9) | `UNREVIEWED` | Add optional native compile smoke test for generated C |
| `icon-devel` | [`0b5e098`](https://github.com/keroblabs/retro-wm/commit/0b5e0980e6955053a1a1fe2e5879bd797ecddb6a) | `UNREVIEWED` | Document generated C assets and icon-format contract |

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
- **2026-11-16 — Scope Clarification:** `RWM-ICO-0005` owns the concrete three-icon desktop demo, its captions and two-click selection/activation. Keep this ticket for general optional icon integration into reusable UI components, not duplicate the demo acceptance criteria.


**Proposed Change**  
Add optional icon references to chosen UI descriptors, with metrics/layout review where needed. Prioritize a minimal demonstration before expanding the widget set.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-ICO-0005

**ID:** `RWM-ICO-0005`  
**Status:** `OPEN`  
**Title:** Demonstrate three desktop icons with selection and click activation

**Description**  
The SDL demonstration should show three right-aligned desktop icons, label them, support selection by one click on the icon image, and activate the selected icon on a second completed click by opening a window confirming the event. The layout must be configurable without hardcoding screen resolution.

**Details**  
**Assets:** Use the generated C resources and `icon_t` descriptors from `RWM-ICO-0001` / `RWM-ICO-0003` for `icon_trash`, `icon_drive`, and `icon_floppy_disk`. Suggested displayed captions: **Trash**, **Drive**, **Floppy Disk**. Do not load PNG files at runtime. Icon dimensions are currently 64×64.

**Required desktop arrangement:** One column at the right, ordered **bottom-to-top Trash, Drive, Floppy Disk**. Use named, build-time/demo configuration values rather than scattered literals:

- `ICON_SIZE = 64`, `RIGHT_MARGIN = 64`, `BOTTOM_MARGIN = 64`.
- `VERTICAL_IMAGE_GAP = 48`, giving top-to-top **pitch = 64 + 48 = 112**.
- `CAPTION_GAP = 14`; font **C8x14** gives `CAPTION_HEIGHT = 14`. Icon + gap + caption occupies **64 + 14 + 14 = 92** pixels. This leaves 20 pixels between caption bottom and the next icon above/below at a 112-pixel pitch.
- **Coordinate correction:** The originally proposed `X = screen_width - 64`, `Y_trash = screen_height - 64` would put the rightmost/bottommost image edge at the framebuffer boundary and the lower caption offscreen. To achieve the stated **64-pixel right and bottom margins around the full content**, use:

```text
x = screen_width - RIGHT_MARGIN - ICON_SIZE
trash_y = screen_height - BOTTOM_MARGIN
          - (ICON_SIZE + CAPTION_GAP + CAPTION_HEIGHT)
drive_y = trash_y - (ICON_SIZE + VERTICAL_IMAGE_GAP)
floppy_y = drive_y - (ICON_SIZE + VERTICAL_IMAGE_GAP)
caption_y = icon_y + ICON_SIZE + CAPTION_GAP
```

- At 960×640 the image origins are **Trash (832,484)**, **Drive (832,372)**, **Floppy Disk (832,260)**. At 640×480 they are **Trash (512,324)**, **Drive (512,212)**, **Floppy Disk (512,100)**. Logical size selection remains **compile-time** (`RWM-CFG-0001`); there is no runtime-resolution switch requirement.
- Caption text should be **centred beneath each icon**, not forcibly clipped to a 64-pixel column: “Floppy Disk” is 11 × 8 = **88 pixels** in C8x14. Ensure captions and their selection rectangles remain inside the desktop and do not overlap the next icon.

**Rendering order:** The icons/captions belong to the **desktop layer**, above its background but **below windows and menus**, and must never be painted over an overlapping window. The current demonstration creates initial overlapping windows, so initial placement or window geometry may need adjustment to keep all three icons visible. Selection changes must invalidate and repaint the relevant desktop area without corrupting window content.

**Click/selection behaviour:**
- Hit testing considers **only the 64×64 icon image rectangles**. Captions are not clickable.
- A completed primary-button click (press and release on the same exposed icon, without an intervening drag/cancellation) selects it. Show selection by **inverting the caption foreground/background**; preserve C8x14 legibility in colour and monochrome themes.
- A second completed click on that same selected icon invokes its demo `on_click` action. For now, opening a normal Retro WM window titled or labelled for the corresponding icon is sufficient evidence that dispatch worked.
- Clicking a different icon selects that icon instead; clicking empty desktop space clears selection. Window/menu interactions take priority over obscured desktop icons. Do not use a double-click timing threshold; two successive single clicks are intentional.
- Keep selection and activation state deterministic, including pointer capture, dragging, window overlap, and cancellation. Define repaint/invalidation for selection transitions and opened windows.

**Acceptance criteria for review:** Demo builds with generated icon assets and normal C11 compilation; exactly three icons render in the intended order at both 960×640 and 640×480 (when build-configured), with 64-pixel right/bottom margins and 112-pixel image pitch. Text is centred at 14-pixel gap and selected text is inverted. Tests cover hit rectangles, first/second click, changing selection, clearing selection, actions, occlusion and repaint. Existing window/menu interactions and SDL-free core builds must remain functional.

**Dependencies:** `RWM-ICO-0001` descriptor, `RWM-ICO-0002` canvas icon renderer, `RWM-ICO-0003` C asset generation. This is the **concrete desktop example**, separate from `RWM-ICO-0004` general reusable UI icon support.

**Notes**
- **2026-11-16 — Discovery:** Registered from the proposed three-icon desktop layout and selection/activation interaction. Coordinates were checked against caption height and stated right/bottom margins. This is a planning ticket; implementation has not started.
- **2026-11-16 — Review:** The user's explicit original top-left coordinates would clip the bottom caption and give no 64-pixel image right margin. The proposed corrected layout anchors the **complete icon-plus-caption group** to a 64-pixel bottom margin while preserving the requested 112-pixel image pitch. Keep these margins configurable for later tuning.

**Proposed Change**  
Provide a portable desktop icon layer or paint callback integrated into the existing window rendering order. Implement an application-owned list of three icons, per-icon 64×64 image hit regions, caption inversion selection state, and two-successive-click activation. Use named spacing configuration, C8x14 font metrics, and a simple message window to demonstrate the on-click event. Do not introduce runtime image decoding or timed double-click handling.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
