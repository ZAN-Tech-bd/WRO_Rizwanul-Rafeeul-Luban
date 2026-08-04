# Provenance

This folder was originally cloned from an upstream GitHub repository and then
"flattened" (its nested `.git` history removed) so that it no longer appears as
a nested git repository inside the main project workspace. Removing the nested
`.git` restores editor checkpoint / snapshot support, which breaks whenever a
second `.git` directory is detected inside the workspace.

- **Upstream repo:** https://github.com/ThZihan/3.2-inch-TFT-LCD-ILI9481-480X320-display
- **Upstream HEAD at time of clone:** `941acb7` — "Add smooth smiling robot face for Mega 2560 + ILI9481 16-bit shield"
- **Clone depth:** shallow (`--depth 1`)
- **Contents preserved:** `LICENSE`, `README.md`, `THIRD_PARTY_LICENSES.md`, `SmilingRobotFace/SmilingRobotFace.ino`

## Re-cloning upstream (if you ever need the git history back)

```bash
git clone --depth 1 https://github.com/ThZihan/3.2-inch-TFT-LCD-ILI9481-480X320-display.git .standalone-display-repo
```

> Note: re-introducing a `.git` folder here will re-trigger the
> "Checkpoints are disabled because a nested git repository was detected"
> warning. If you need the history, keep the clone **outside** this workspace.
