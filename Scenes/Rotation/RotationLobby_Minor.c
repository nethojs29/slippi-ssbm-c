// Rotation Lobby Minor Scene — TEST VERSION 4
// Test: Text_CreateText(0,0) instead of Text_CreateCanvas + Text_CreateText2

#include "RotationLobby.h"

typedef struct {
    int frame_count;
    u8 msrb[MSRB_TOTAL_SIZE];
    Text *text;
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

    // Try GameSetup-style text creation
    ui->text = Text_CreateText(0, 0);
    ui->text->kerning = 1;
    ui->text->align = 1;
    ui->text->use_aspect = 1;
    ui->text->scale.X = 0.01;
    ui->text->scale.Y = 0.01;

    Text_AddSubtext(ui->text, 0.0, 3.0, "ROTATION LOBBY");
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
    if (ui->text) Text_Destroy(ui->text);
    HSD_Free(ui);
    ui = 0;
}
