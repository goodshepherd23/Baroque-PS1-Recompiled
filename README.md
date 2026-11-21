# Baroque Recompiled

<a href="https://recomplauncher.com/game/baroque-ps1-recompiled"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Get it on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="48"></picture></a>

A native PC build of **Baroque: Yuganda Mousou** (PlayStation, Sting, 1999), made
with the [PSXRecomp](https://github.com/mstan/psxrecomp) static recompiler and the
shared [recomp-ui](https://github.com/mstan/recomp-ui) launcher.

This is the PlayStation release. For the Sega Saturn original, see
[Baroque-SaturnRecomp](https://github.com/goodshepherd23/Baroque-SaturnRecomp).

You must supply your own disc: the Japanese release **SLPM-86328** as a CUE/BIN
dump (2 tracks) or a `.chd` made from one, **with the English v1.02 fan translation
applied**:

| Image | Serial | Tracks | Data track 01 MD5 |
|---|---|---|---|
| Baroque - Yuganda Mousou (English v1.02) | SLPM-86328 | 2 (data + audio) | `942542bdad4b70e13e52772eebdcf63d` |

Other images (the untranslated Japanese disc, other patch versions) are not
supported yet: the game code is generated from the disc's program, and the
translation changes it.

This repository and its release packages contain **no game disc, no BIOS dump, no
generated game code and no saves**. The game code is generated on your machine
from your disc.

No retail BIOS is needed: the build uses the bundled
[OpenBIOS](https://github.com/grumpycoders/pcsx-redux) image.

## Setup

1. Download the release ZIP for your platform and extract all of it into a
   writable folder.
2. Start `Baroque_Recompiled` (`.exe` on Windows).
3. Select `Baroque - Yuganda Mousou (English v1.02).cue` (or your `.chd`) in the
   setup wizard. Keep the CUE next to both BIN tracks.
4. Run **Generate & rebuild** and wait. The game starts when it finishes.

On Windows the setup wizard can download portable build tools. On Linux and macOS,
install CMake, Ninja, Python 3 and a C/C++ compiler first.

**With [RecompLauncher](https://recomplauncher.com):** install the recomp, drop your
`.cue` on its setup screen, and it checks the disc, generates and builds the game
without a window (`Baroque_Recompiled --prepare --game=<your .cue>`), and Play
starts it directly. The resolution, filtering, screen and V-Sync settings are on the
game's page. The release zip carries `recomplauncher.json` for this.

## CHD discs

A `.chd` made with MAME's `chdman createcd` from the CUE/BIN works too. Setup
unpacks it to the CUE/BIN set in the project's `disc` folder (the same place a
CUE/BIN is staged), checks the data track against the digest above and builds
from that; the game itself then plays straight from your `.chd`. A CHD of this
disc gives back the original tracks byte for byte.

## Logs

The game writes a log to `build-release/logs/` beside its executable
(`Baroque_Recompiled-<date>-<time>.log`, the newest five are kept). Please attach
the newest one to bug reports. Set `BAROQUE_NO_LOG=1` to turn it off.

## Controls

This is the Japanese release, so **Circle confirms and Cross cancels** in menus.
Keyboard and controller bindings are in the launcher's settings.

## Longer view distance (optional)

On the launcher's **Mods** page, the **Graphics** group has **Longer View Distance**
(off by default). In the tower it draws corridors four times as far and pushes the
darkness back to match, so about one more lit stretch of corridor shows where the
original goes black. Past that stretch there is a further limit it does not change.

It changes two things in the dungeon engine while it is loaded: the per-floor
far-clip distance (multiplied by 4) and the eight reads of the distance-fade table
(indexed by depth >> 10 instead of >> 8). Details are in `src/baroque_mods.c`.

## How the game's code runs

The main program (`SLPM_863.28`) is compiled to native code. The town, tower and
dungeon engines are separate programs the game loads from the disc while you play
(`BIN00`-`BIN02`). Those run on the built-in interpreter the first time you reach
them, and with `overlay_cache = true` the runtime records them and compiles them in
the background, so later visits run natively.

## Status

Tested on Windows 11 (Intel Iris Xe laptop) with a local build:

- Opening movies, title screen, main menu, memory card check and name entry.
- A new game into the first 3D area: walking and turning at full speed, HUD and
  English text correct.

Not tested yet: deep dungeon floors, saving and loading, the ending, Linux and
macOS gameplay. Please report problems in the issue tracker.

## Credits

- Baroque is (c) 1999 Sting. This project is not affiliated with Sting.
- The English v1.02 translation is a fan patch by its authors; this project does not
  include or distribute it.
- PSXRecomp and recomp-ui by their authors (see the submodules and
  `psxrecomp/THIRD_PARTY_ATTRIBUTION.md`). OpenBIOS from the PCSX-Redux project
  (MIT, see `bios/OpenBIOS.LICENSE`).

Developed with AI assistance (Claude).
