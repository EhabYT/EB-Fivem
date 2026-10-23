#!/usr/bin/env python3
"""Regenerate the README "Include graph" mermaid block from the real includes.

The README claims the graph is "generated from the actual #include lines"; this
script is what makes that true and keeps it true.  It scans every root-level
.cpp/.h of the repository (vendored trees under imgui/ and kiero/ are not part
of the graph, matching the previous hand-made block), collects quoted includes
that name a root header, and rewrites the fenced mermaid block in the
"### Include graph" section plus the node/edge counts in the paragraph above
it.  Run it after adding or removing a root module or an #include:

    python docs/gen_include_graph.py

Layout mirrors the old block: one subgraph per module (a .cpp plus the header
it owns), standalone rounded boxes for headers nobody owns a .cpp for,
standalone rectangles for .cpp files without a sibling header.  Nodes are
numbered n0, n1, ... in alphabetical module order so diffs stay readable.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
README = os.path.join(ROOT, "README.md")

INCLUDE_RE = re.compile(r'^\s*#include\s+"([^"/\\]+\.h)"\s*$')


def root_files(ext):
    names = []
    for name in sorted(os.listdir(ROOT)):
        path = os.path.join(ROOT, name)
        if os.path.isfile(path) and name.endswith(ext):
            names.append(name)
    return names


def includes_of(fname):
    out = []
    with open(os.path.join(ROOT, fname), encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = INCLUDE_RE.match(line)
            if m:
                out.append(m.group(1))
    return out


def main():
    cpps = root_files(".cpp")
    hdrs = set(root_files(".h"))

    modules = []      # (module name, cpp or None, header or None)
    owned = set()
    for cpp in cpps:
        hdr = cpp[:-4] + ".h"
        if hdr in hdrs:
            modules.append((cpp[:-4], cpp, hdr))
            owned.add(hdr)
        else:
            modules.append((cpp[:-4], cpp, None))
    modules.sort(key=lambda m: m[0])
    standalone_hdrs = sorted(h for h in hdrs if h not in owned)

    node_id = {}
    lines = []
    counter = 0
    for name, cpp, hdr in modules:
        if cpp and hdr:
            lines.append('    subgraph sg_%s["%s"]' % (name, name))
            node_id[cpp] = "n%d" % counter
            lines.append('        %s["%s"]' % (node_id[cpp], cpp))
            counter += 1
            node_id[hdr] = "n%d" % counter
            lines.append('        %s(["%s"])' % (node_id[hdr], hdr))
            counter += 1
            lines.append("    end")
        else:
            node_id[cpp] = "n%d" % counter
            lines.append('    subgraph sg_%s["%s"]' % (name, name))
            lines.append('        %s["%s"]' % (node_id[cpp], cpp))
            counter += 1
            lines.append("    end")
    for hdr in standalone_hdrs:
        node_id[hdr] = "n%d" % counter
        lines.append('    %s(["%s"])' % (node_id[hdr], hdr))
        counter += 1

    edges = []
    srcs = sorted(cpps + sorted(hdrs), key=lambda f: node_id.get(f, f))
    for src in srcs:
        for tgt in includes_of(src):
            if src in node_id and tgt in node_id and src != tgt:
                pair = (node_id[src], node_id[tgt])
                if pair not in edges:
                    edges.append(pair)
    lines.append("")
    for a, b in sorted(edges, key=lambda e: (int(e[0][1:]), int(e[1][1:]))):
        lines.append("    %s --> %s" % (a, b))

    block = "flowchart LR\n" + "\n".join(lines) + "\n"

    with open(README, encoding="utf-8", newline="") as fh:
        readme = fh.read()

    start = readme.index("### Include graph")
    fence_open = readme.index("```mermaid", start)
    fence_close = readme.index("```", fence_open + 10)
    readme = readme[:fence_open + 10] + "\n" + block + readme[fence_close:]

    # tolerate the script's own output: the counts sentence ends with the
    # "(regenerate with ...)" parenthesis it writes itself
    count_re = re.compile(r"\*\*Generated from the actual `#include` lines\*\* — \d+ nodes, "
                          r"\d+ edges(?: \([^)]*\))?\.")
    new_counts = ("**Generated from the actual `#include` lines** — %d nodes, %d edges "
                  "(regenerate with `python docs/gen_include_graph.py`)."
                  % (counter, len(edges)))
    readme, n = count_re.subn(new_counts, readme, count=1)
    if n != 1:
        sys.exit("counts paragraph not found")

    with open(README, "w", encoding="utf-8", newline="") as fh:
        fh.write(readme)
    print("include graph: %d nodes, %d edges" % (counter, len(edges)))


if __name__ == "__main__":
    main()
