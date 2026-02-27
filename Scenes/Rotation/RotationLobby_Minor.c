// Rotation Lobby Minor Scene — TEST VERSION 3
// Adds: Text API (single text element)
// No camera — just testing if Text_CreateCanvas works

#include "RotationLobby.h"

typedef struct {
    int frame_count;
    u8 local_port;
    u8 player_count;
    u8 msrb[MSRB_TOTAL_SIZE];
    int canvas_id;
    Text *test_text;
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

    // Test: create a single text element
    GXColor white = {0xFF, 0xFF, 0xFF, 0xFF};
    ui->canvas_id = Text_CreateCanvas(0, 0, 0, 13, 80, 8, 0, 0);
    ui->test_text = Text_CreateText2(0, ui->canvas_id,
        5.0, 5.0, 0.0, 20.0, 2.0);
    Text_AddSubtext(ui->test_text, 0.0, 0.0, "ROTATION LOBBY");
    Text_SetColor(ui->test_text, 0, &white);
}

void minor_think(void)
{
    if (!ui) return;
    ui->frame_count++;

    if (ui->frame_count >= 300)
        Scene_ExitMinor();
}

void minor_exit(void *unload_data)
{
    if (!ui) return;

    if (ui->test_text) Text_Destroy(ui->test_text);

    HSD_Free(ui);
    ui = 0;
}
