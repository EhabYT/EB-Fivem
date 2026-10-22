# `ci/` — the build workflow, staged but not yet enabled

`build.yml` in this folder is a **complete, validated GitHub Actions workflow**. It is
parked here rather than in `.github/workflows/` for one reason only: the token this PR was
produced with is not allowed to create files under `.github/workflows/`, and GitHub refuses
the push outright.

```
! [remote rejected] arena/57915bfa-eb-fivem (refusing to allow a GitHub App to create or
                     update workflow `.github/workflows/build.yml` without `workflows` permission)
```

The file itself is fine. It parses as YAML, has 6 steps, 2 matrix combinations
(`Release`, `Debug` × `x64`), all actions pinned to a major version, and the rendered build
command is exactly the one `BUILD.md` documents:

```
msbuild "redENGINE UI.sln" /t:Rebuild /p:Configuration=<Config> /p:Platform=x64 /m /nologo /clp:"Summary;Verbosity=normal"
```

Moving it into place takes one commit from an account that has the `workflows` permission —
yours. Until that happens this repository has **no CI and no compile proof**; everything in
this PR was verified statically.

---

## Enabling it — pick one

### A. Web UI (no local clone needed)

1. Open <https://github.com/EhabYT/EB-Fivem/actions> → **New workflow** →
   **set up a workflow yourself**.
2. GitHub opens `.github/workflows/main.yml` in the editor. Rename the file in the path box
   to `.github/workflows/build.yml`.
3. Replace the whole content with the content of
   [`ci/build.yml`](https://github.com/EhabYT/EB-Fivem/blob/arena/57915bfa-eb-fivem/ci/build.yml)
   (open it in raw view and copy).
4. **Commit changes** to `arena/57915bfa-eb-fivem`.
5. The workflow has `workflow_dispatch`, so you can start the first run immediately from the
   Actions tab without pushing anything else.

### B. Local git

```bat
git fetch origin arena/57915bfa-eb-fivem
git checkout arena/57915bfa-eb-fivem
ci\enable.bat
git commit -m "ci: enable the Windows build workflow"
git push origin arena/57915bfa-eb-fivem
```

`enable.bat` only copies `ci\build.yml` to `.github\workflows\build.yml` and stages it with
`git add`. It does not commit and does not push — those two lines are yours, so the commit
lands under your identity with your permissions. Run it from the repository root.

### C. Nothing at all

Leave it parked. The workflow stays reviewable as a normal file in the PR diff, and the
local `msbuild` command above is unchanged either way.

---

## What the workflow does

| Step | Purpose |
| --- | --- |
| Check out | `actions/checkout@v4` |
| Add MSBuild to PATH | `microsoft/setup-msbuild@v2` — MSBuild is on the runner but not on `PATH` |
| Report toolchain | prints the resolved VS install path, product version and `msbuild -version` |
| Build | `msbuild /t:Rebuild`, `shell: cmd`, `Release`+`Debug` × `x64`, `fail-fast: false` |
| Verify the artifact | fails if `x64\<Config>\redENGINE UI.dll` is missing; runs `dumpbin /exports` on it |
| Upload artifact | DLL + PDB + `rE.png`, `if-no-files-found: error`, 14 days retention |

Two deliberate choices worth knowing about:

* **`Win32` is not in the matrix.** The DLL is injected into FiveM, which is 64-bit only, so a
  Win32 artifact could never load. The configuration still exists in the `.vcxproj`; add it to
  the matrix if that ever changes.
* **`windows-latest` ships VS2022/v143, not v142.** The `.vcxproj` pins `PlatformToolset` to
  `v142` when it is installed and otherwise falls back to `$(DefaultPlatformToolset)`, so the
  first CI run exercises that fallback. That is the point — it is the branch nobody has tested.

`concurrency` cancels a run still in flight when the same ref is pushed again, and
`permissions: contents: read` keeps the job from writing to the repository.

---

## Keeping `ci/build.yml` and `.github/workflows/build.yml` in sync

They are meant to be identical, and once the workflow is enabled the copy under
`.github/workflows/` is the one GitHub actually runs. Either delete this folder at that
point, or treat `ci/build.yml` as the staged source and re-copy it after every edit:

```bat
copy /Y ci\build.yml .github\workflows\build.yml
```

If the two ever diverge, `.github/workflows/` wins — that is the file the runner reads.
