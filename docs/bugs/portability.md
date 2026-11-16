# Bugs — Portability

Scope: Renderer/platform contracts, framebuffer layout and future emulator/embedded host integration.  
ID prefix: `RWM-PLT`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `RWM-PLT-0001` | `OPEN` | Document and validate the native framebuffer contract |
| `RWM-PLT-0002` | `OPEN` | Specify a minimal emulator and embedded host interface |

## Issues

### RWM-PLT-0001

**ID:** `RWM-PLT-0001`  
**Status:** `OPEN`  
**Title:** Document and validate the native framebuffer contract

**Description**  
Correct use on other CPUs and display controllers depends on assumptions about pixel storage, alignment, stride, endianness and presentation that are not all enforced by the core interface.

**Details**  
**Confirmed:** `include/retro_pixel.h` provides compile-time RGB565 (`uint16_t`) or RGB888-as-32-bit (`uint32_t`) pixels, while `wm_framebuffer` has `stride_pixels` and a CPU pointer. `platform/sdl/sdl_host.c` maps to SDL RGB565/ARGB8888. The README documents native byte order and defers cache/pitch concerns to a platform adapter.

**Acceptance criteria for review:** State assumptions and responsibilities for native storage and byte order, stride/pitch, buffer lifetime, clipping, alignment and presentation; test stride padding and both pixel formats without SDL; identify any required conversion boundary for incompatible controllers. Do not claim hardware validation that has not occurred.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Preserve the existing direct-framebuffer reference renderer. Add contract checks, documentation and targeted portability tests; introduce conversion/copy operations only at the appropriate platform boundary when justified.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-PLT-0002

**ID:** `RWM-PLT-0002`  
**Status:** `OPEN`  
**Title:** Specify a minimal emulator and embedded host interface

**Description**  
The SDL adapter is the only implemented host. Future RISC-V emulator and embedded targets need a precise boundary for display storage, normalized input, pointer capture/cursor support and presenting frames.

**Details**  
**Confirmed:** `platform/sdl/sdl_host.*` owns SDL events, framebuffer allocation and presentation. `ui_context` and the example app are SDL-independent. No emulator or board host exists yet.

**Acceptance criteria for review:** Document the platform-facing responsibilities and a minimal integration example or contract without introducing dependencies into the core; distinguish mandatory operations from SDL-only conveniences; retain SDL-free builds/tests.

**Related:** `RWM-PLT-0001`. Platform-specific implementation is future work and not assumed in this ticket.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Use the existing SDL adapter as a reference for a small platform contract. Reuse `wm_event`/`wm_framebuffer` where sufficient rather than imposing a new general driver framework prematurely.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
