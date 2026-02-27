// Rotation Lobby Minor Scene — MINIMAL TEST VERSION
// Just a frame counter that exits after 120 frames.
// Used to isolate what's crashing.

#include "RotationLobby.h"

static int frame_count = 0;

void minor_load(void *load_data)
{
    frame_count = 0;
}

void minor_think(void)
{
    frame_count++;
    if (frame_count >= 120)
        Scene_ExitMinor();
}

void minor_exit(void *unload_data)
{
}
