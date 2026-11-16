# Bug Tracking Methodology

**Document status:** Active working methodology  
**Applies to:** Project engineering, code reviews, defect corrections, and improvements  
**Location:** `docs/bugs/bug-tracking-methodology.md`

## 1. Purpose and guiding principles

This document defines how we identify, describe, discuss, fix, review, and close defects and engineering improvements in the project. The tracker is part of the repository and is the durable record of decisions, not merely a to-do list.

- **Review before change.** A finding is recorded before implementation, and its proposed remedy is not automatically approved.
- **Preserve history.** Historical snapshots and reference branches remain unchanged; fixes and improvements belong on an explicitly designated working branch. Never rewrite a preserved baseline as part of issue resolution.
- **One issue, one identifiable concern.** Related issues may be linked, but distinct causes or acceptance criteria should not be hidden in a single ticket.
- **Keep records consistent.** Every category file has the same header, summary, and ticket structure.
- **Record decisions and rework.** Notes form a chronological, append-only engineering history, including discarded proposals, review feedback, and reopened issues.
- **Distinguish evidence from hypotheses.** A suspected defect is permissible, but must be labelled as unverified until confirmed by analysis or tests.
- **Track implementation commits.** Each issue retains a commit list with branch, short hash, review status, and concise change description.
- **No silent closure.** A resolved issue must say what changed, how it was verified, and who approved the conclusion.

## 2. Files and issue ownership

Each file under `docs/bugs/` covers one logical subsystem or engineering area, not necessarily one source directory.

Example category files (illustrative; this table is not a work plan or list of files to create):

| Example file | Example scope | ID prefix |
| --- | --- | --- |
| `platform.md` | Platform-dependent behavior and device integration | `PRJ-PLT` |
| `core.md` | Shared algorithms, interfaces, and data structures | `PRJ-CORE` |
| `interface.md` | Interface layout, input, and visual behavior | `PRJ-UI` |

Add, rename, or reorganize category files according to the project's needs, maintaining unique ID prefixes and preserving existing issue IDs. Place each issue where its **primary corrective responsibility** lies; cross-reference other affected files or tickets in Details or Notes. Do not duplicate the same issue in two files.

## 3. Issue identification

IDs have the form `<PROJECT>-<AREA>-NNNN`, for example `PRJ-CORE-0001` and `PRJ-PLT-0001`. Choose a short, stable project prefix before registering the first issue; `PRJ` is an illustrative placeholder, not a prescribed prefix.

- Numbers start at `0001` and increase within each category file.
- An ID is permanent. Never reuse, renumber, or recycle it, even if the issue is rejected, moved, or closed.
- The ID remains stable if the title, description, or owning category changes; leave a cross-reference in the old file if an issue must move.
- Commit messages, review notes, tests, and related tickets should reference the ID.

## 4. Status model

| Status | Meaning |
| --- | --- |
| `OPEN` | Recorded for assessment. The cause or proposed fix may still be unverified. |
| `IN_PROGRESS` | The agreed investigation or implementation is actively being carried out. |
| `REVIEW` | A proposed implementation or conclusion is ready for examination and testing evidence is recorded. |
| `RESOLVED` | The outcome has been reviewed and accepted, with the resolution recorded; integration or final housekeeping may remain. |
| `CLOSED` | The accepted outcome is integrated into the designated working branch, or a documented no-change disposition has been completed. |
| `DEFERRED` | Deliberately postponed, with a reason and a condition for reconsideration. |

Normal flow: `OPEN → IN_PROGRESS → REVIEW → RESOLVED → CLOSED`.

Investigation may return from `IN_PROGRESS` or `REVIEW` to `OPEN` or `IN_PROGRESS` when the hypothesis or proposed change needs revision. A `DEFERRED`, `RESOLVED`, or `CLOSED` issue may be reopened by returning it to `OPEN`, with an explicit dated note explaining why. No special “reopened” status is needed: the note preserves the history.

Issues may be closed without code changes only when the disposition (for example, duplicate, not reproducible, or intentionally retained behavior) is documented and approved. Record the rationale in Resolution Description; do not delete the ticket.

## 5. Required structure of every category file

Every category file uses exactly this top-level order:

1. **Header:** title, scope, ID prefix, and a brief pointer to this methodology. Include a short statement that the summary is the current index and the issue records below are authoritative.
2. **Summary:** a Markdown table with exactly three columns — `ID`, `Status`, and `Title`. Each issue occupies one row, with a short, single-line title. Sort by issue ID. The table is updated whenever status or title changes.
3. **Issues:** complete tickets, in ascending ID order. Do not create an issue entry without a corresponding summary row, or vice versa.

Do not use the summary to hold technical explanations or discussions. Those belong in the corresponding ticket.

### Category file skeleton (illustrative placeholders only; not real tickets)

```markdown
# Bugs — <Area>

Scope: <logical scope>  
ID prefix: `PRJ-XXX`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `PRJ-XXX-0001` | `OPEN` | <Short single-line title> |

## Issues

### PRJ-XXX-0001

**ID:** `PRJ-XXX-0001`  
**Status:** `OPEN`  
**Title:** <Short descriptive title>

**Description**  
<What is wrong, unclear, unsafe, obsolete, or worth improving.>

**Details**  
<Files/functions, current behavior, expected behavior, evidence, reproduction,
impact, assumptions, and dependencies. Mark unverified assertions explicitly.>

**Notes**
- **YYYY-MM-DD — Discovery:** <Source of finding and initial observations.>

**Proposed Change**  
<Recommended direction, constraints, and alternatives; or 'To be determined'.
This is a proposal until reviewed and agreed.>

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
```

The required ticket fields must always appear, in this order: **ID, Status, Title, Description, Details, Notes, Proposed Change, Commits, Resolution Description**. Additional technical subheadings may be used *within* Details or Notes, but should not replace these fields. An empty Commits table means no commits have yet been associated with the ticket.

## 6. Writing effective issues

**Title:** name the failure or improvement, not a vague activity such as “review buffer”. Keep it short enough for a one-line summary.

**Description:** state the observable problem, risk, inconsistency, or improvement opportunity. Say why it matters; do not assume a proposed fix is the cause.

**Details:** give relative source paths, relevant function names, version/branch inspected, assumptions, expected versus actual behavior, triggering conditions, relevant execution contexts (such as event loops, interrupt handlers, or concurrent workers) where applicable, and tests or code evidence. Distinguish *confirmed*, *suspected*, and *not yet tested*. Optional descriptors such as defect / safety / cleanup / improvement can live here if useful; they are not mandatory extra fields.

**Proposed Change:** describe a possible remedy and its consequences, not an irreversible instruction. Include open questions and alternatives where appropriate. If no remedy is yet justified, write `To be determined after review`.

**Commits:** maintain the per-issue implementation history in the four-column table prescribed in Section 8. Keep superseded commits visible rather than deleting them.

**Resolution Description:** initially `Pending`. Once accepted, capture the final disposition, files changed, relevant commits/PRs, verification results and limitations, and the reason for any deviation from the original proposal. Do not claim tests were run unless they were.

## 7. Notes: the chronological engineering log

Notes are **append-only** and contain dated entries. Add a note whenever an issue is materially investigated, its description/proposal is corrected, code is changed, a test is run, a review finds a problem, rework occurs, its status changes, or a decision is made.

Suggested entry syntax:

```markdown
- **YYYY-MM-DD — <Activity>:** <Observation, decision, evidence, or change>
```

Useful activity labels include `Discovery`, `Investigation`, `Decision`, `Implementation`, `Test`, `Code Review`, `Rework`, `Status Change`, and `Reopened`.

Keep current Description, Details, and Proposed Change accurate; when editing their substance, append a note explaining what changed and why. Do not silently erase an earlier interpretation or unsuccessful approach. Link to commits and tests where applicable. Record reviewer approval explicitly; do not infer it from silence.

## 8. Commit tracking and code review

Each ticket contains its own **Commits** section, immediately after Proposed Change and before Resolution Description. Record commits that implement, test, revert, or rework that ticket. Documentation-only commits unrelated to a ticket do not need an entry.

Use a table with exactly four columns:

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `example-branch` | `a1b2c3d` | `UNREVIEWED` | Illustrative implementation change |

The example row above is a format illustration, **not** a real tracked commit. In actual tickets, leave the table with no data rows until a relevant commit exists.

- **Branch:** the branch on which the commit was created or submitted for review. Preserve the historical branch entry if a change is later integrated elsewhere.
- **Commit (short hash):** an unambiguous abbreviated Git SHA (normally 7–12 hexadecimal characters). Prefer linking the short hash to the full GitHub commit URL. The linked full SHA identifies the exact reviewed revision.
- **Review Status:** the disposition of that specific commit, independent of the ticket's overall status.
- **Description:** one short line explaining the change made by the commit, rather than a proposed or future action.

Use these per-commit review statuses:

| Review Status | Meaning |
| --- | --- |
| `UNREVIEWED` | Commit recorded; code review has not started. |
| `IN_REVIEW` | Commit is undergoing review. |
| `CHANGES_REQUESTED` | Review identified changes required before acceptance. |
| `APPROVED` | Commit has been reviewed and accepted. |
| `SUPERSEDED` | Commit was replaced, amended, or made obsolete by a later change; retain its record. |

Add a row when the commit becomes available. Update its review status when review progresses, and log the review decision, date, reviewer feedback, test results, or required rework in **Notes**. If amending or rebasing changes the commit hash, retain the old row marked `SUPERSEDED` and add a row for the new hash. Never transfer an `APPROVED` status automatically to a different commit SHA; review the replacement as a new revision.

A single commit may be listed in more than one ticket when it genuinely addresses multiple issues; reference all relevant IDs in the commit message where practical. Do not mistake an `APPROVED` commit for an automatically `RESOLVED` issue: the complete behavior and resolution still require verification and acceptance.

## 9. Review and resolution workflow

1. **Discover:** inspect the relevant code or baseline and register an `OPEN` ticket with evidence or a clearly marked hypothesis. Update the summary row.
2. **Triage together:** decide whether it is a real defect, a safety or portability risk, historical residue, or an improvement worth making. Clarify expected behavior and scope; log the decision.
3. **Agree on a proposed change:** examine alternatives and dependencies, particularly public interfaces, ownership, memory use, event flow, performance constraints, and affected platforms. If the change is not agreed, keep the issue open or defer it.
4. **Implement:** work only on the designated working branch; make focused changes and reference ticket IDs in commits. Record each relevant commit in the issue's Commits table as `UNREVIEWED`. Do not modify preserved historical baselines.
5. **Verify:** add or run relevant native tests, build checks, static review, and target-platform, timing, concurrency, or interrupt-specific checks as appropriate. Record exactly what passed, failed, or could not be tested. A build alone is not proof of runtime correctness.
6. **Review:** set the issue to `REVIEW`, review relevant commits, update their review statuses, and examine implementation, API behavior, edge cases, ownership, execution-context requirements, resource constraints, error paths, and evidence. Log review findings and rework.
7. **Accept and close:** after explicit approval, set `RESOLVED` with a complete resolution; set `CLOSED` once integrated into the designated branch or the approved no-change disposition is complete. Update the summary at each transition.

If a test later exposes a regression, reopen the original issue when it concerns the same root cause; otherwise open a new issue and cross-reference it.

## 10. Consistency and scope controls

- Any change to status or title must be reflected in both the summary row and the ticket fields in the same edit.
- Each issue has one authoritative record; links between issues express dependency, duplication, or shared investigation.
- Do not batch unrelated defects into one ticket just because they occur in the same file.
- Keep defect corrections separate from discretionary API redesign when their acceptance criteria differ.
- Keep the Commits table consistent with the code review history. Never delete a superseded commit entry or claim that a different SHA has inherited its approval.
- Do not mark findings as fixed merely because a patch exists; verify the behavior and record review.
- Repository changes are separate from discussion: a proposed ticket or fix is not automatically approved because it was drafted.

## 11. Maintaining this methodology

This document describes **how to work**, not **what work to perform**. Bug discoveries, specific subsystem reviews, implementation priorities, and delivery plans belong in issue files or other project planning records, not in this methodology.

Changes to the tracking format or lifecycle should be reviewed like other engineering decisions, committed to the repository, and applied consistently to existing issue files. Preserve the issue IDs, historical notes, and commit review trail when evolving the format.
