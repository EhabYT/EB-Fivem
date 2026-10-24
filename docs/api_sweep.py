#!/usr/bin/env python3
"""Static compile-risk sweep over the root sources.

The sandbox that produced this branch has no MSVC, so executable compile
checks are scarce.  This is one of them: it greps every root-level .cpp/.h
(vendored trees under imgui/ and kiero/ excluded) for ImGui API names that the
vendored 1.92.9 no longer provides - removed outright, or present only as a
commented-out obsolete redirect in imgui.h - and exits nonzero on any hit.

Run it after vendoring a new imgui tag or touching widget code:

    python docs/api_sweep.py

Comment text is stripped before matching, so explanatory comments (like the
ListBoxHeader/ListBoxFooter note in luamenus.cpp) do not count as hits.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# name -> why it must not appear in root sources
# Verified against the vendored imgui.h: every entry below is either absent or
# present only as a commented-out obsolete redirect.  PushTabStop/PopTabStop are
# deliberately NOT listed - they survived as live inline wrappers (imgui.h ~4140).
TOKENS = {
    "SetNextFrameWantTextInput": "never existed in any ImGui version - a phantom API",
    "ListBoxHeader": "removed in 1.89 (BeginListBox/EndListBox replace it)",
    "ListBoxFooter": "removed in 1.89 (BeginListBox/EndListBox replace it)",
    "GetOverlayDrawList": "removed long ago (GetForegroundDrawList replaces it)",
    "ImDrawCornerFlags": "replaced by ImDrawFlags in 1.82 and removed since",
    "CalcListClipping": "removed with the 1.9x clipper cleanup",
}


def main():
    hits = []
    for name in sorted(os.listdir(ROOT)):
        if not name.endswith((".cpp", ".h")):
            continue
        path = os.path.join(ROOT, name)
        with open(path, encoding="utf-8", errors="replace") as fh:
            for lineno, line in enumerate(fh, 1):
                code = line.split("//", 1)[0]          # strip comment text
                for token, why in TOKENS.items():
                    if token in code:
                        hits.append("%s:%d: %s - %s" % (name, lineno, token, why))
    if hits:
        for h in hits:
            print("API SWEEP HIT:", h)
        sys.exit(1)
    print("api sweep clean: %d root files, no removed ImGui API in use"
          % len([n for n in os.listdir(ROOT) if n.endswith(('.cpp', '.h'))]))


if __name__ == "__main__":
    main()
