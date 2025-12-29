# Archived Documentation

**Archived:** November 25, 2025
**Reason:** Obsolete, superseded, or duplicate documentation from earlier project phases

---

## Important Notice

⚠️ **These documents DO NOT reflect the current implementation state.**

**Current Implementation:**
- **Architecture:** Plan E (per-eye vertex buffer rendering with layer-based depth separation)
- **Build MD5:** `4afa913c0c994705ed6cc4f07795c3ca`
- **Status:** P1-P6 bugs fixed, ready for hardware testing
- **Date:** November 25, 2025

**For Current Documentation:** See `/DOCUMENTATION_INDEX.md` or `/POST_FIX_VERIFICATION_NOV25.md`

---

## Archive Structure

### `obsolete-plans/`
Status documents and planning docs from earlier phases that have been superseded:
- STATUS.md - Early status (Oct 20, claimed "15% complete")
- FINAL_STATUS.md - Pre-Plan E status (Nov 11, wrong build MD5)
- BUILD_SUMMARY_NOV14.md - Logging build session
- PHASE1-3 documents - Obsolete phase tracking system
- Early analysis documents (RENDERING_ARCHITECTURE_SUMMARY, etc.)

### `wrong-approaches/`
Implementation documents describing incorrect approaches that were later replaced:
- IMPLEMENTATION_COMPLETE.md - Vertex offset without per-eye buffers (wrong)
- IMPLEMENTATION_LOG.md - 52KB of Session 1-3 incorrect approaches
- STEREO_IMPLEMENTATION_FINAL.md - Flat stereo texture approach (not Plan E)

### `historical-sessions/`
Hardware testing and debugging session notes from Nov 13-14:
- HARDWARE_TESTING_SESSION_NOV13.md - Pre-Plan E testing
- HARDWARE_TESTING_SESSION_NOV14.md - Display corruption debugging
- AI_VIBECODE_SESSION_NOV14.md - Session notes

### `duplicates/`
Documents that duplicate information from more recent/authoritative sources:
- NEXT_STEPS.md / NEXT_STEPS_UPDATED.md - Superseded by NEXT_SESSION_GUIDE.md
- SESSION_SUMMARY.md - Superseded by individual session docs
- SANITY_CHECK.md - Superseded by SANITY_CHECK_NOV12.md
- README_OLD.md - Superseded by README.md

### `indexes/`
Index documents that reference now-archived files:
- STEREO_DOCUMENTATION_INDEX.md
- RENDERING_ANALYSIS_INDEX.md

---

## Why These Were Archived

**Problem:** Project iterated through multiple implementation approaches:
1. **Sessions 1-3 (Oct 20-Nov 11):** Various incorrect approaches (depth buffer modification, post-processing, etc.)
2. **Session 4 (Nov 11-12):** First working prototype (vertex offset approach)
3. **Plan E (Nov 14-15):** Current architecture (per-eye vertex buffers, dual rendering)
4. **Bug Fixes (Nov 25):** P1-P6 fixes for slider semantics, dead code, VRAM optimization

Each iteration created documentation that became obsolete when the next approach was discovered. These documents were archived to prevent confusion about the current implementation state.

---

## Historical Value

These documents are kept for:
- **Lessons learned:** Understanding what approaches DIDN'T work and why
- **Build history:** Tracking MD5 hashes and progression through builds
- **Debugging reference:** If similar issues occur, these sessions show how they were solved
- **Project timeline:** Complete record of development process

---

## What Replaced These Documents

| Archived Document | Current Replacement |
|-------------------|---------------------|
| STATUS.md, FINAL_STATUS.md | POST_FIX_VERIFICATION_NOV25.md |
| IMPLEMENTATION_COMPLETE.md, STEREO_IMPLEMENTATION_FINAL.md | docs/PLAN_E_STEREO_COMPLETE.md |
| NEXT_STEPS.md, NEXT_STEPS_UPDATED.md | NEXT_SESSION_GUIDE.md |
| SANITY_CHECK.md | SANITY_CHECK_NOV12.md |
| PHASE1-3 documents | (Phase system abandoned) |

---

## If You're Looking For...

- **Current implementation status** → `/POST_FIX_VERIFICATION_NOV25.md`
- **Build history and verification** → `/SANITY_CHECK_NOV12.md`
- **Hardware testing procedures** → `/NEXT_SESSION_GUIDE.md`
- **Plan E architecture details** → `/docs/PLAN_E_STEREO_COMPLETE.md`
- **SNES9x rendering analysis** → `/docs/LAYER_RENDERING_RESEARCH.md`
- **Complete doc listing** → `/DOCUMENTATION_INDEX.md`

---

**Last Updated:** November 25, 2025
