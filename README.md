# Baroque Recompiled

A native PC build of **Baroque: Yuganda Mousou** (PlayStation, Sting, 1999), made
with the [PSXRecomp](https://github.com/mstan/psxrecomp) static recompiler and the
shared [recomp-ui](https://github.com/mstan/recomp-ui) launcher.

This is the PlayStation release. For the Sega Saturn original, see
[Baroque-SaturnRecomp](https://github.com/goodshepherd23/Baroque-SaturnRecomp).

You must supply your own disc: the Japanese release **SLPM-86328** as a CUE/BIN
dump (2 tracks), **with the English v1.02 fan translation applied**:

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
3. Select `Baroque - Yuganda Mousou (English v1.02).cue` in the setup wizard.
   Keep the CUE next to both BIN tracks.
4. Run **Generate & rebuild** and wait. The game starts when it finishes.

On Windows the setup wizard can download portable build tools. On Linux and macOS,
install CMake, Ninja, Python 3 and a C/C++ compiler first.

## Controls

This is the Japanese release, so **Circle confirms and Cross cancels** in menus.
Keyboard and controller bindings are in the launcher's settings.

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
