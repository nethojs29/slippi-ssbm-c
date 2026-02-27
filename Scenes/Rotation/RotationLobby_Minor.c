// Rotation Lobby Minor Scene — TEST VERSION 5
// Combines: calloc, MSRB, camera (COBJ_Alloc), Text_CreateText, auto-exit

#include "RotationLobby.h"

// CObjThink — clears screen to black
void CObjThink(GOBJ *gobj)
{
    COBJ *cobj = gobj->hsd_object;
    if (!CObj_SetCurrent(cobj))
        return;
    CObj_SetEraseColor(0, 0, 0, 255);
    CObj_EraseScreen(cobj, 1, 0, 1);
    CObj_RenderGXLinks(gobj, 7);
    CObj_EndCurrent();
}

typedef struct {
    int frame_count;
    u8 local_port;
    u8 player_count;
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

    ui->local_port   = ui->msrb[OFST_LOCAL_PLAYER_INDEX];
    ui->player_count = ui->msrb[OFST_ROT_PLAYER_COUNT];

    // Camera — minimal, just for screen clearing
    GOBJ *cam_gobj = GObj_Create(2, 3, 128);
    COBJ *cam_cobj = COBJ_Alloc();
    GObj_AddObject(cam_gobj, 1, cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink, 0);
    CObj_SetOrtho(cam_cobj, 0.0f, 480.0f, 0.0f, 640.0f);
    CObj_SetViewport(cam_cobj, 0.0f, 640.0f, 0.0f, 480.0f);
    CObj_SetScissor(cam_cobj, 0, 480, 0, 640);
    cam_gobj->cobj_links = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 4);

    // Text — GameSetup style
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
