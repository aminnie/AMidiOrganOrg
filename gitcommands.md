# Git, build, and launch — quick reference

Copy-paste commands from the **repository root** (Windows **PowerShell**). Full detail: [README.md](README.md).

---

## Git

### See where you are

```powershell
git status -sb
git branch --show-current
git log --oneline -10
```

### Feature branch

```powershell
git checkout main
git pull origin main
git checkout -b feature/your-branch-name
```

### Commit

```powershell
git add <files>
git status
git commit -m "feat: short imperative description"
```

Use prefixes your team agrees on (`feat:`, `fix:`, `chore:`, …).

### First push of a branch (sets upstream)

```powershell
git push -u origin feature/your-branch-name
```

Later pushes on the same branch: `git push`  
GitHub usually prints a URL to open a pull request.

### After the PR is merged on GitHub

```powershell
git checkout main
git pull origin main
git branch -d feature/your-branch-name
```

Use `-D` only if you intend to delete a branch that was not merged.

### Optional: update your branch from latest `main`

```powershell
git fetch origin
git rebase origin/main
# resolve conflicts if needed, then:
git push --force-with-lease
```

---

## Prerequisites (build)

- **CMake** 3.22+
- **JUCE** source tree — set `JUCE_ROOT` to match your machine, for example:
  - `C:/JUCE` (common on Windows)
  - `./.deps/JUCE` if you clone JUCE into the repo

---

## Configure (one-time per build folder)

From the repo root (adjust `JUCE_ROOT` if needed):

```powershell
cmake -S . -B build -DJUCE_ROOT="C:/JUCE"
```

---

## Build the app (Windows / Visual Studio generator)

**Debug** (typical for development):

```powershell
cmake --build build --config Debug --target AMidiOrgan
```

Output:

- `build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe`

**Release**:

```powershell
cmake --build build --config Release --target AMidiOrgan
```

Output:

- `build/AMidiOrgan_artefacts/Release/AMidiOrgan.exe`

---

## Launch the app (Windows)

**Debug** — new process (does not block the terminal):

```powershell
Start-Process "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"
```

**Debug** — same terminal (foreground):

```powershell
& "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"
```

**Release**:

```powershell
Start-Process "build/AMidiOrgan_artefacts/Release/AMidiOrgan.exe"
```

---

## Tests (optional)

```powershell
cmake --build build --config Debug --target AMidiOrganTests
ctest --test-dir build -C Debug --output-on-failure
```

---

## Rebuild and relaunch (common dev loop)

If the linker reports **LNK1168** (“cannot open … AMidiOrgan.exe for writing”), the app is still running — close it or:

```powershell
Get-Process -Name "AMidiOrgan" -ErrorAction SilentlyContinue | Stop-Process -Force
cmake --build build --config Debug --target AMidiOrgan
Start-Process "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"
```

---

## Tips

| Issue | What to do |
|--------|------------|
| **LNK1168** / exe locked | Quit AMidiOrgan or run `Stop-Process` as above, then rebuild. |
| CMake / JUCE not found | Point `-DJUCE_ROOT=` at your JUCE checkout; see README. |
| macOS / Xcode | See **Build** in [README.md](README.md) for Xcode generator commands. |
| CI behavior | `.github/workflows/ci.yml` — builds tests + app on push/PR to `main`. |

---

## Clone (new machine)

```powershell
git clone https://github.com/aminnie/AMidiOrganOrg.git
cd AMidiOrganOrg
```

Use your fork URL if you use a fork.
