// Rotation Lobby Minor Scene — TEST VERSION 2
// Adds: calloc, MSRB load, frame counter, auto-exit
// No camera, no text — just testing allocation + EXI

#include "RotationLobby.h"

typedef struct {
    int frame_count;
    u8 local_port;
    u8 player_count;
    u8 is_active_player;
    u8 msrb[MSRB_TOTAL_SIZE];
} LobbyUIState;

static LobbyUIState *ui = 0;

static void load_msrb(u8 *buf)
{
    buf[0] = 0xB3;
    ExiSlippi_Transfer(buf, 1, ExiSlippi_TransferMode_WRITE);
    ExiSlippi_Transfer(buf, MSRB_TOTAL_SIZE, ExiSlippi_TransferMode_READ);
}

void minor_load(void *load_data)
{
    ui = calloc(sizeof(LobbyUIState));

    load_msrb(ui->msrb);

    ui->local_port   = ui->msrb[OFST_LOCAL_PLAYER_INDEX];
    ui->player_count = ui->msrb[OFST_ROT_PLAYER_COUNT];
}

void minor_think(void)
{
    if (!ui) return;
    ui->frame_count++;

    if (ui->frame_count >= 120)
        Scene_ExitMinor();
}

void minor_exit(void *unload_data)
{
    if (ui) {
        HSD_Free(ui);
        ui = 0;
    }
}
