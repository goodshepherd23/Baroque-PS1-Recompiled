/* Baroque enhancements: trusted mod plugins for mods/packages/baroque.enhancements.
 *
 * Longer view distance in the tower. The dungeon engine (BIN01/GAME.BIN, loaded
 * at 0x800CC200 while you play) has two limits:
 *
 *  1. A far-clip distance, set once per floor (SetFar at 0x800E1590 stores
 *     `far = a0 - near` at gp-0x7384, with a copy at gp-0x7388; gp = 0x800704E0).
 *     Anything farther is not drawn at all: the solid black at the end of long
 *     corridors.
 *  2. A distance fade: a 256-entry table at 0x80104DF4 indexed by depth >> 8 and
 *     handed to the GTE as IR0, so DCPL fades vertex colours toward black.
 *
 * Raising only the far-clip just draws more black, so both change together:
 * the far-clip is multiplied by 4, and the eight table reads use depth >> 10
 * (each `sra x, y, 2` before the lookup becomes `sra x, y, 4`), so the fade
 * reaches four times farther. About one more lit corridor segment shows.
 *
 * The far-clip is watched instead of patching SetFar: whenever the value
 * differs from the one written here, the game has just set it for a new floor,
 * so it is scaled again. The fade reads are code patches through
 * psx_mod_write_code_word, applied only while the original instruction is
 * still in place, so they follow GAME.BIN in and out of memory. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mod_plugins.h"
#include "rl_achievements.h"

#ifndef BQ_VERSION
#define BQ_VERSION "dev"
#endif

#define BQ_FAR_ADDR       0x8006915Cu   /* gp - 0x7384 */
#define BQ_FAR_COPY_ADDR  0x80069158u   /* gp - 0x7388 */
#define BQ_FAR_SCALE      4

static const struct { uint32_t pc, orig, patch; } kFadeReads[] = {
    { 0x800DF174u, 0x00141083u, 0x00141103u },   /* sra v0, s4, 2 -> 4 */
    { 0x800DF1D4u, 0x00052083u, 0x00052103u },   /* sra a0, a1, 2 -> 4 */
    { 0x800DF248u, 0x00051083u, 0x00051103u },   /* sra v0, a1, 2 -> 4 */
    { 0x800DF2D4u, 0x00051083u, 0x00051103u },   /* sra v0, a1, 2 -> 4 */
    { 0x800DFF24u, 0x00022083u, 0x00022103u },   /* sra a0, v0, 2 -> 4 */
    { 0x800DFF84u, 0x00072083u, 0x00072103u },   /* sra a0, a3, 2 -> 4 */
    { 0x800DFFF8u, 0x000A2083u, 0x000A2103u },   /* sra a0, t2, 2 -> 4 */
    { 0x800E0088u, 0x00092083u, 0x00092103u },   /* sra a0, t1, 2 -> 4 */
};
#define BQ_FADE_READS (sizeof(kFadeReads) / sizeof(kFadeReads[0]))

/* BQ_MOD_LOG=<file path> appends one line per change. Off by default. */
static void bq_log(const char* fmt, unsigned a, unsigned b, unsigned c) {
    const char* path = getenv("BQ_MOD_LOG");
    FILE* f;
    if (!path) return;
    f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "[baroque-mod] ");
    fprintf(f, fmt, a, b, c);
    fputc('\n', f);
    fclose(f);
}

static void bq_view_distance(void) {
    static int32_t s_written;
    unsigned i, orig = 0, patched = 0;
    int32_t far;
    if (!psx_mod_game_started()) return;

    /* Only touch the fade reads when all eight sites hold the dungeon engine's
     * code (original or already patched), so other code loaded at these
     * addresses is never changed. */
    for (i = 0; i < BQ_FADE_READS; i++) {
        uint32_t w = psx_mod_read_word(kFadeReads[i].pc);
        if (w == kFadeReads[i].orig) orig++;
        else if (w == kFadeReads[i].patch) patched++;
    }
    if (orig && orig + patched == BQ_FADE_READS) {
        for (i = 0; i < BQ_FADE_READS; i++)
            if (psx_mod_read_word(kFadeReads[i].pc) == kFadeReads[i].orig)
                psx_mod_write_code_word(kFadeReads[i].pc, kFadeReads[i].patch);
        bq_log("fade reads patched (%u sites, %u already)", orig, patched, 0);
    }

    if (orig + patched != BQ_FADE_READS) return;   /* not in the dungeon engine */
    far = (int32_t)psx_mod_read_word(BQ_FAR_ADDR);
    if (far > 0 && far != s_written && far < 0x7FFFFFFF / BQ_FAR_SCALE) {
        s_written = far * BQ_FAR_SCALE;
        psx_mod_write_word(BQ_FAR_ADDR, (uint32_t)s_written);
        psx_mod_write_word(BQ_FAR_COPY_ADDR, (uint32_t)s_written);
        bq_log("far-clip %u -> %u", (unsigned)far, (unsigned)s_written, 0);
    }
}

/* Recomp Achievements (third_party/rl-achievements). RecompLauncher hosts the
 * list of memory rules for this game; the library checks them once per frame
 * against PlayStation memory and reports unlocks to the launcher over a local
 * pipe. Started without RecompLauncher, every call returns straight away.
 *
 * Addresses in a PS1 list are rcheevos' PlayStation map: 0x000000-0x1FFFFF is
 * the 2 MB of RAM (kernel area included), 0x200000-0x2003FF the scratchpad. */
static uint32_t bq_ach_read(uint32_t address, uint8_t* buffer, uint32_t n,
                            void* ud) {
    uint32_t i;
    (void)ud;
    for (i = 0; i < n; ++i) {
        const uint32_t a = address + i;
        uint32_t guest;
        if (a < 0x200000u) guest = 0x80000000u + a;
        else if (a < 0x200400u) guest = 0x1F800000u + (a - 0x200000u);
        else break;
        buffer[i] = psx_mod_read_byte(guest);
    }
    return i;
}

static void bq_achievements(void) {
    static int s_started;
    static uint32_t s_loads;
    if (!psx_mod_game_started()) return;
    if (!s_started) {
        s_started = 1;
        s_loads = psx_savestate_load_count();
        rl_ach_set_info("baroque-ps1-recompiled", BQ_VERSION);
        rl_ach_init("psx", bq_ach_read, NULL);
    }
    /* A save state or rewind replaced the game: later unlocks are casual. */
    if (psx_savestate_load_count() != s_loads) {
        s_loads = psx_savestate_load_count();
        rl_ach_casual("save_state");
    }
    rl_ach_frame();
}

PSX_MOD_CONSTRUCTOR(baroque_register_mods) {
    psx_mod_register_vblank_plugin("baroque.enhancement.view-distance",
                                   bq_view_distance);
    psx_mod_register_vblank_plugin("baroque.achievements", bq_achievements);
}
