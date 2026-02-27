// Rotation Lobby Minor Scene — TEST VERSION 6
// Static frame counter to test if calloc'd memory is being clobbered

#include "RotationLobby.h"

static int frame_count = 0;

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
    frame_count = 0;
    ui = calloc(sizeof(LobbyUIState));
    load_msrb(ui->msrb);

    ui->local_port   = ui->msrb[OFST_LOCAL_PLAYER_INDEX];
    ui->player_count = ui->msrb[OFST_ROT_PLAYER_COUNT];

    // Camera
    GOBJ *cam_gobj = GObj_Create(2, 3, 128);
    COBJ *cam_cobj = COBJ_Alloc();
    GObj_AddObject(cam_gobj, 1, cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink, 0);
    CObj_SetOrtho(cam_cobj, 0.0f, 480.0f, 0.0f, 640.0f);
    CObj_SetViewport(cam_cobj, 0.0f, 640.0f, 0.0f, 480.0f);
    CObj_SetScissor(cam_cobj, 0, 480, 0, 640);
    cam_gobj->cobj_links = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 4);

    // Text — try Text_CreateText2 with canvas 0 (no canvas creation)
    ui->text = Text_CreateText2(0, 0, 5.0, 10.0, 0.0, 20.0, 5.0);
    Text_AddSubtext(ui->text, 0.0, 0.0, "ROTATION LOBBY");
    GXColor white = {0xFF, 0xFF, 0xFF, 0xFF};
    Text_SetColor(ui->text, 0, &white);
}

void minor_think(void)
{
    frame_count++;

    if (frame_count >= 300)
        Scene_ExitMinor();
}

void minor_exit(void *unload_data)
{
    if (ui) {
        if (ui->text) Text_Destroy(ui->text);
        HSD_Free(ui);
        ui = 0;
    }
}
