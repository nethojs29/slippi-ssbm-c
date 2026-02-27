// Rotation Lobby Minor Scene — lobby UI (mnFunction)
// Compiled to RotationLobby.dat via mnFunction symbol.
//
// Exports: minor_think, minor_load, minor_exit
//
// Self-contained: loads MSRB directly via EXI, no major scene dependency.
// This runs as a minor scene within the Slippi Online major (ID 8).

#include "RotationLobby.h"

// ---------------------------------------------------------------------------
// Layout constants (text canvas coordinate space)
// ---------------------------------------------------------------------------

// Top bar
#define TOP_Y           0.8
#define TOP_LABEL_X     1.0
#define TOP_TIMER_X     12.5
#define TOP_COUNT_X     22.0

// Main panel (left 70%)
#define PANEL_LEFT      1.0
#define PANEL_TOP       3.0
#define PANEL_WIDTH     19.0

// Matchup area
#define MATCH_HEADER_Y  3.5
#define P1_NAME_X       2.0
#define P1_NAME_Y       5.5
#define P1_CHAR_Y       7.0
#define P1_READY_Y      8.2
#define VS_X            9.5
#define VS_Y            6.0
#define P2_NAME_X       13.5
#define P2_NAME_Y       5.5
#define P2_CHAR_Y       7.0
#define P2_READY_Y      8.2

// Sidebar (right 30%)
#define SIDE_LEFT        21.5
#define SIDE_TOP         3.0
#define SIDE_WIDTH       8.0
#define SIDE_HEADER_Y    3.5
#define SIDE_FIRST_Y     5.0
#define SIDE_LINE_H      1.3

// Bottom bar
#define BOT_Y            19.5
#define BOT_GAME_X       1.5
#define BOT_PROMPT_X     8.0

// Timer
#define TIMER_TOTAL_FRAMES  1800  // 30 seconds at 60fps
#define SPECTATOR_WAIT      180   // 3 seconds

// ---------------------------------------------------------------------------
// Character name table
// ---------------------------------------------------------------------------
static const char *char_names[] = {
    "Captain Falcon",  // 0x00
    "DK",              // 0x01
    "Fox",             // 0x02
    "Mr. Game & Watch",// 0x03
    "Kirby",           // 0x04
    "Bowser",          // 0x05
    "Link",            // 0x06
    "Luigi",           // 0x07
    "Mario",           // 0x08
    "Marth",           // 0x09
    "Mewtwo",          // 0x0A
    "Ness",            // 0x0B
    "Peach",           // 0x0C
    "Pikachu",         // 0x0D
    "Ice Climbers",    // 0x0E
    "Jigglypuff",      // 0x0F
    "Samus",           // 0x10
    "Yoshi",           // 0x11
    "Zelda",           // 0x12
    "Sheik",           // 0x13
    "Falco",           // 0x14
    "Young Link",      // 0x15
    "Dr. Mario",       // 0x16
    "Roy",             // 0x17
    "Pichu",           // 0x18
    "Ganondorf",       // 0x19
};
#define NUM_CHARACTERS 26

// ---------------------------------------------------------------------------
// CObjThink — camera render callback
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// StockIcon frame helper
// ---------------------------------------------------------------------------
static void StockIcon_SetFrame(StockIcon *si, u8 charId, u8 charColor)
{
    u32 adjId = charId;
    if (charId == CKIND_SHEIK)
        adjId = 29;
    else if (charId > CKIND_SHEIK)
        adjId--;

    JOBJ_AddSetAnim(si->root_jobj, si->jobj_set, 0);
    JOBJ_ReqAnimAll(si->root_jobj, adjId + (30 * charColor));
    JOBJ_AnimAll(si->root_jobj);
    JOBJ_RemoveAnimAll(si->root_jobj);
    si->state.char_id = charId;
    si->state.color_id = charColor;
}

// ---------------------------------------------------------------------------
// Lobby UI state — fully self-contained, loads MSRB directly
// ---------------------------------------------------------------------------
typedef struct {
    int frame_count;
    int local_ready;
    int opponent_ready;
    u8 opp_char;
    u8 opp_color;
    int wait_count;

    // Rotation state (parsed from MSRB)
    u8 local_port;
    u8 player_count;
    u8 active_ports[2];
    u8 waiting_ports[ROT_MAX_WAITING];
    u8 games_played;
    u8 last_winner;
    u8 is_active_player;
    u8 is_spectator;
    u8 selected_char;
    u8 selected_color;

    // Raw MSRB buffer (for name lookups)
    u8 msrb[MSRB_TOTAL_SIZE];

    // Text canvas
    int canvas_id;

    // Top bar
    Text *lobby_name_text;
    Text *timer_text;
    Text *player_count_text;

    // Match panel
    Text *match_header_text;
    Text *p1_text;
    Text *p2_text;
    Text *p1_char_text;
    Text *p2_char_text;
    Text *p1_ready_text;
    Text *p2_ready_text;
    Text *vs_text;

    // Sidebar
    Text *side_header_text;
    Text *queue_texts[ROT_MAX_WAITING];

    // Bottom bar
    Text *game_text;
    Text *prompt_text;

    // 3D rendering
    HSD_Archive *gui_archive;
    StockIcon p1_icon;
    StockIcon p2_icon;
} LobbyUIState;

static LobbyUIState *ui = 0;

// ---------------------------------------------------------------------------
// MSRB helpers
// ---------------------------------------------------------------------------
static void load_msrb(u8 *buf)
{
    buf[0] = 0xB3;  // CMD_GET_MATCH_STATE
    ExiSlippi_Transfer(buf, 1, ExiSlippi_TransferMode_WRITE);
    ExiSlippi_Transfer(buf, MSRB_TOTAL_SIZE, ExiSlippi_TransferMode_READ);
}

static char *get_player_name(u8 *msrb, u8 port)
{
    if (port > 3) return "???";
    return (char *)(msrb + OFST_P1_NAME + port * MSRB_NAME_SIZE);
}

static char *get_connect_code(u8 *msrb, u8 port)
{
    if (port > 3) return "";
    return (char *)(msrb + OFST_P1_CONNECT_CODE + port * MSRB_CONNECT_CODE_SIZE);
}

// ---------------------------------------------------------------------------
// EXI communication for character sync
// ---------------------------------------------------------------------------
static void send_char_selection(u8 char_id, u8 color_id)
{
    ExiSlippi_CompleteStep_Query q;
    q.command = ExiSlippi_Command_GP_COMPLETE_STEP;
    q.step_idx = 0;
    q.char_selection = char_id;
    q.char_color_selection = color_id;
    q.stage_selections[0] = 0;
    q.stage_selections[1] = 0;
    ExiSlippi_Transfer(&q, sizeof(q), ExiSlippi_TransferMode_WRITE);
}

static int fetch_opponent_selection(u8 *char_id, u8 *color_id)
{
    ExiSlippi_FetchStep_Query q;
    q.command = ExiSlippi_Command_GP_FETCH_STEP;
    q.step_idx = 0;
    ExiSlippi_Transfer(&q, sizeof(q), ExiSlippi_TransferMode_WRITE);

    ExiSlippi_FetchStep_Response resp;
    ExiSlippi_Transfer(&resp, sizeof(resp), ExiSlippi_TransferMode_READ);

    if (resp.is_found) {
        *char_id = resp.char_selection;
        *color_id = resp.char_color_selection;
    }
    return resp.is_found;
}

// ---------------------------------------------------------------------------
// minor_load — called when the minor scene is entered
// Receives load_data pointer from ASM (currently NULL — we load MSRB ourselves)
// ---------------------------------------------------------------------------
void minor_load(void *load_data)
{
    ui = calloc(sizeof(LobbyUIState));

    // Initialize waiting ports to 0xFF
    for (int i = 0; i < ROT_MAX_WAITING; i++)
        ui->waiting_ports[i] = 0xFF;

    // Load MSRB directly via EXI
    load_msrb(ui->msrb);

    // Parse rotation state
    ui->local_port      = ui->msrb[OFST_LOCAL_PLAYER_INDEX];
    ui->player_count    = ui->msrb[OFST_ROT_PLAYER_COUNT];
    ui->active_ports[0] = ui->msrb[OFST_ROT_ACTIVE_P1];
    ui->active_ports[1] = ui->msrb[OFST_ROT_ACTIVE_P2];
    ui->games_played    = ui->msrb[OFST_ROT_GAMES_PLAYED];
    ui->last_winner     = ui->msrb[OFST_ROT_LAST_WINNER];
    ui->is_spectator    = ui->msrb[OFST_IS_SPECTATOR];

    for (int i = 0; i < ROT_MAX_WAITING; i++)
        ui->waiting_ports[i] = ui->msrb[OFST_ROT_QUEUE_START + i];

    ui->is_active_player =
        (ui->local_port == ui->active_ports[0] ||
         ui->local_port == ui->active_ports[1]);

    // Default character (Fox)
    ui->selected_char = 0x02;
    ui->selected_color = 0;

    // Count waiting players
    ui->wait_count = 0;
    for (int i = 0; i < ROT_MAX_WAITING; i++) {
        if (ui->waiting_ports[i] != 0xFF)
            ui->wait_count++;
    }

    // =================================================================
    // 3D rendering setup — camera, fog, lights from GameSetup_gui.dat
    // =================================================================
    ui->gui_archive = Archive_LoadFile("GameSetup_gui.dat");
    GUI_GameSetup *gui = Archive_GetPublicAddress(ui->gui_archive, "ScGamTour_scene_data");

    // Camera
    GOBJ *cam_gobj = GObj_Create(2, 3, 128);
    COBJ *cam_cobj = COBJ_LoadDesc(gui->cobjs[0]);
    GObj_AddObject(cam_gobj, 1, cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink, 0);
    cam_gobj->cobj_links = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);

    // Fog
    GOBJ *fog_gobj = GObj_Create(14, 2, 0);
    HSD_Fog *fog = Fog_LoadDesc(gui->fog[0]);
    GObj_AddObject(fog_gobj, 4, fog);
    GObj_AddGXLink(fog_gobj, GXLink_Fog, 0, 128);

    // Lights
    GOBJ *light_gobj = GObj_Create(3, 4, 128);
    LOBJ *lobj = LObj_LoadAll(gui->lights);
    GObj_AddObject(light_gobj, 2, lobj);
    GObj_AddGXLink(light_gobj, GXLink_LObj, 0, 128);

    // =================================================================
    // StockIcons for P1 and P2 character display
    // =================================================================
    ui->p1_icon.jobj_set = gui->jobjs[GUI_GameSetup_JOBJ_StockIcon];
    ui->p1_icon.gobj = JOBJ_LoadSet(0, ui->p1_icon.jobj_set, 0, 0, 3, 1, 0, 0);
    ui->p1_icon.root_jobj = ui->p1_icon.gobj->hsd_object;
    ui->p1_icon.root_jobj->trans.X = -8.0;
    ui->p1_icon.root_jobj->trans.Y = 4.0;
    ui->p1_icon.root_jobj->trans.Z = 0.0;

    ui->p2_icon.jobj_set = gui->jobjs[GUI_GameSetup_JOBJ_StockIcon];
    ui->p2_icon.gobj = JOBJ_LoadSet(0, ui->p2_icon.jobj_set, 0, 0, 3, 1, 0, 0);
    ui->p2_icon.root_jobj = ui->p2_icon.gobj->hsd_object;
    ui->p2_icon.root_jobj->trans.X = 8.0;
    ui->p2_icon.root_jobj->trans.Y = 4.0;
    ui->p2_icon.root_jobj->trans.Z = 0.0;

    // Set local player's icon
    {
        int is_p1 = (ui->local_port == ui->active_ports[0]);
        if (ui->is_active_player && is_p1)
            StockIcon_SetFrame(&ui->p1_icon, ui->selected_char, ui->selected_color);
        if (ui->is_active_player && !is_p1)
            StockIcon_SetFrame(&ui->p2_icon, ui->selected_char, ui->selected_color);
    }

    // =================================================================
    // Text UI
    // =================================================================
    GXColor white   = {0xFF, 0xFF, 0xFF, 0xFF};
    GXColor green   = {0x21, 0xBA, 0x45, 0xFF};
    GXColor gray    = {0x99, 0x99, 0x99, 0xFF};
    GXColor yellow  = {0xFF, 0xD7, 0x00, 0xFF};
    GXColor red     = {0xDB, 0x28, 0x28, 0xFF};
    GXColor cyan    = {0x00, 0xCC, 0xCC, 0xFF};

    ui->canvas_id = Text_CreateCanvas(0, 0, 0, 13, 80, 8, 0, 0);

    // --- TOP BAR ---
    ui->lobby_name_text = Text_CreateText2(0, ui->canvas_id,
        TOP_LABEL_X, TOP_Y, 0.0, 10.0, 1.5);
    char *code = get_connect_code(ui->msrb, ui->local_port);
    Text_AddSubtext(ui->lobby_name_text, 0.0, 0.0, "%s", code);
    Text_SetColor(ui->lobby_name_text, 0, &cyan);
    Text_SetScale(ui->lobby_name_text, 0, 0.9, 0.9);

    ui->timer_text = Text_CreateText2(0, ui->canvas_id,
        TOP_TIMER_X, TOP_Y, 0.0, 6.0, 1.5);
    Text_AddSubtext(ui->timer_text, 0.0, 0.0, "0:30");
    Text_SetColor(ui->timer_text, 0, &white);
    Text_SetScale(ui->timer_text, 0, 1.1, 1.1);

    ui->player_count_text = Text_CreateText2(0, ui->canvas_id,
        TOP_COUNT_X, TOP_Y, 0.0, 8.0, 1.5);
    Text_AddSubtext(ui->player_count_text, 0.0, 0.0,
        "%d Players", ui->player_count);
    Text_SetColor(ui->player_count_text, 0, &gray);
    Text_SetScale(ui->player_count_text, 0, 0.8, 0.8);

    // --- MATCH PANEL ---
    ui->match_header_text = Text_CreateText2(0, ui->canvas_id,
        PANEL_LEFT + 3.0, MATCH_HEADER_Y, 0.0, 16.0, 1.5);
    Text_AddSubtext(ui->match_header_text, 0.0, 0.0, "PLAYERS IN MATCH");
    Text_SetColor(ui->match_header_text, 0, &green);
    Text_SetScale(ui->match_header_text, 0, 0.9, 0.9);

    // Player 1
    char *p1_name = get_player_name(ui->msrb, ui->active_ports[0]);
    ui->p1_text = Text_CreateText2(0, ui->canvas_id,
        P1_NAME_X, P1_NAME_Y, 0.0, 7.0, 1.5);
    Text_AddSubtext(ui->p1_text, 0.0, 0.0, "%s", p1_name);
    Text_SetColor(ui->p1_text, 0, &white);
    Text_SetScale(ui->p1_text, 0, 1.0, 1.0);

    ui->p1_char_text = Text_CreateText2(0, ui->canvas_id,
        P1_NAME_X, P1_CHAR_Y, 0.0, 7.0, 1.5);
    Text_AddSubtext(ui->p1_char_text, 0.0, 0.0,
        "%s", (char *)char_names[ui->selected_char % NUM_CHARACTERS]);
    Text_SetColor(ui->p1_char_text, 0, &green);
    Text_SetScale(ui->p1_char_text, 0, 0.7, 0.7);

    ui->p1_ready_text = Text_CreateText2(0, ui->canvas_id,
        P1_NAME_X, P1_READY_Y, 0.0, 7.0, 1.5);
    if (ui->is_active_player && ui->local_port == ui->active_ports[0])
    {
        Text_AddSubtext(ui->p1_ready_text, 0.0, 0.0, "Picking...");
        Text_SetColor(ui->p1_ready_text, 0, &yellow);
    }
    else
    {
        Text_AddSubtext(ui->p1_ready_text, 0.0, 0.0, "...");
        Text_SetColor(ui->p1_ready_text, 0, &gray);
    }
    Text_SetScale(ui->p1_ready_text, 0, 0.55, 0.55);

    // VS
    ui->vs_text = Text_CreateText2(0, ui->canvas_id,
        VS_X, VS_Y, 0.0, 3.0, 2.0);
    Text_AddSubtext(ui->vs_text, 0.0, 0.0, "VS");
    Text_SetColor(ui->vs_text, 0, &red);
    Text_SetScale(ui->vs_text, 0, 1.5, 1.5);

    // Player 2
    char *p2_name = get_player_name(ui->msrb, ui->active_ports[1]);
    ui->p2_text = Text_CreateText2(0, ui->canvas_id,
        P2_NAME_X, P2_NAME_Y, 0.0, 7.0, 1.5);
    Text_AddSubtext(ui->p2_text, 0.0, 0.0, "%s", p2_name);
    Text_SetColor(ui->p2_text, 0, &white);
    Text_SetScale(ui->p2_text, 0, 1.0, 1.0);

    ui->p2_char_text = Text_CreateText2(0, ui->canvas_id,
        P2_NAME_X, P2_CHAR_Y, 0.0, 7.0, 1.5);
    Text_AddSubtext(ui->p2_char_text, 0.0, 0.0, "...");
    Text_SetColor(ui->p2_char_text, 0, &green);
    Text_SetScale(ui->p2_char_text, 0, 0.7, 0.7);

    ui->p2_ready_text = Text_CreateText2(0, ui->canvas_id,
        P2_NAME_X, P2_READY_Y, 0.0, 7.0, 1.5);
    if (ui->is_active_player && ui->local_port == ui->active_ports[1])
    {
        Text_AddSubtext(ui->p2_ready_text, 0.0, 0.0, "Picking...");
        Text_SetColor(ui->p2_ready_text, 0, &yellow);
    }
    else
    {
        Text_AddSubtext(ui->p2_ready_text, 0.0, 0.0, "...");
        Text_SetColor(ui->p2_ready_text, 0, &gray);
    }
    Text_SetScale(ui->p2_ready_text, 0, 0.55, 0.55);

    // --- SIDEBAR ---
    if (ui->wait_count > 0)
    {
        ui->side_header_text = Text_CreateText2(0, ui->canvas_id,
            SIDE_LEFT, SIDE_HEADER_Y, 0.0, SIDE_WIDTH, 1.5);
        Text_AddSubtext(ui->side_header_text, 0.0, 0.0, "WAITING AREA");
        Text_SetColor(ui->side_header_text, 0, &yellow);
        Text_SetScale(ui->side_header_text, 0, 0.7, 0.7);

        float y = SIDE_FIRST_Y;
        for (int i = 0; i < ui->wait_count; i++)
        {
            u8 port = ui->waiting_ports[i];
            if (port == 0xFF) continue;

            char *name = get_player_name(ui->msrb, port);
            ui->queue_texts[i] = Text_CreateText2(0, ui->canvas_id,
                SIDE_LEFT, y, 0.0, SIDE_WIDTH, 1.2);

            if (i == 0)
            {
                Text_AddSubtext(ui->queue_texts[i], 0.0, 0.0,
                    "Next: %s", name);
                Text_SetColor(ui->queue_texts[i], 0, &yellow);
            }
            else
            {
                Text_AddSubtext(ui->queue_texts[i], 0.0, 0.0,
                    "  %d. %s", i + 1, name);
                Text_SetColor(ui->queue_texts[i], 0, &gray);
            }
            Text_SetScale(ui->queue_texts[i], 0, 0.6, 0.6);
            y += SIDE_LINE_H;
        }
    }

    // --- BOTTOM BAR ---
    ui->game_text = Text_CreateText2(0, ui->canvas_id,
        BOT_GAME_X, BOT_Y, 0.0, 8.0, 1.5);
    Text_AddSubtext(ui->game_text, 0.0, 0.0,
        "Game %d", ui->games_played + 1);
    Text_SetColor(ui->game_text, 0, &gray);
    Text_SetScale(ui->game_text, 0, 0.7, 0.7);

    ui->prompt_text = Text_CreateText2(0, ui->canvas_id,
        BOT_PROMPT_X, BOT_Y, 0.0, 20.0, 1.5);
    if (ui->is_active_player)
    {
        Text_AddSubtext(ui->prompt_text, 0.0, 0.0,
            "D-Pad: Change Char    A: Confirm");
        Text_SetColor(ui->prompt_text, 0, &white);
    }
    else
    {
        Text_AddSubtext(ui->prompt_text, 0.0, 0.0,
            "Spectating - waiting for match");
        Text_SetColor(ui->prompt_text, 0, &gray);
        ui->local_ready = 1;
    }
    Text_SetScale(ui->prompt_text, 0, 0.6, 0.6);
}

// ---------------------------------------------------------------------------
// minor_think — called every frame
// ---------------------------------------------------------------------------
void minor_think(void)
{
    if (!ui) return;
    ui->frame_count++;

    // --- Update timer display ---
    {
        int remaining;
        if (ui->is_active_player)
            remaining = TIMER_TOTAL_FRAMES - ui->frame_count;
        else
            remaining = SPECTATOR_WAIT - ui->frame_count;

        if (remaining < 0) remaining = 0;
        int secs = remaining / 60;

        if (ui->is_active_player)
        {
            if (secs <= 5)
            {
                GXColor red = {0xDB, 0x28, 0x28, 0xFF};
                Text_SetText(ui->timer_text, 0, "0:%02d", secs);
                Text_SetColor(ui->timer_text, 0, &red);
            }
            else if (secs <= 10)
            {
                GXColor yellow = {0xFF, 0xD7, 0x00, 0xFF};
                Text_SetText(ui->timer_text, 0, "0:%02d", secs);
                Text_SetColor(ui->timer_text, 0, &yellow);
            }
            else
            {
                Text_SetText(ui->timer_text, 0, "0:%02d", secs);
            }
        }
        else
        {
            Text_SetText(ui->timer_text, 0, "0:%02d", secs);
        }
    }

    // --- Active player input ---
    if (ui->is_active_player && !ui->local_ready)
    {
        int is_p1 = (ui->local_port == ui->active_ports[0]);
        Text *my_char_text = is_p1 ? ui->p1_char_text : ui->p2_char_text;
        Text *my_ready_text = is_p1 ? ui->p1_ready_text : ui->p2_ready_text;
        StockIcon *my_icon = is_p1 ? &ui->p1_icon : &ui->p2_icon;

        HSD_Pad *pad = PadGet(ui->local_port, PADGET_ENGINE);

        if (pad->down & HSD_BUTTON_DPAD_RIGHT)
        {
            ui->selected_char = (ui->selected_char + 1) % NUM_CHARACTERS;
            Text_SetText(my_char_text, 0,
                "%s", (char *)char_names[ui->selected_char]);
            StockIcon_SetFrame(my_icon, ui->selected_char, ui->selected_color);
        }
        if (pad->down & HSD_BUTTON_DPAD_LEFT)
        {
            ui->selected_char =
                (ui->selected_char + NUM_CHARACTERS - 1) % NUM_CHARACTERS;
            Text_SetText(my_char_text, 0,
                "%s", (char *)char_names[ui->selected_char]);
            StockIcon_SetFrame(my_icon, ui->selected_char, ui->selected_color);
        }
        if (pad->down & HSD_BUTTON_DPAD_UP)
        {
            ui->selected_color = (ui->selected_color + 1) % 6;
            Text_SetText(my_char_text, 0, "%s [%d]",
                (char *)char_names[ui->selected_char],
                ui->selected_color + 1);
            StockIcon_SetFrame(my_icon, ui->selected_char, ui->selected_color);
        }
        if (pad->down & HSD_BUTTON_DPAD_DOWN)
        {
            ui->selected_color = (ui->selected_color + 5) % 6;
            Text_SetText(my_char_text, 0, "%s [%d]",
                (char *)char_names[ui->selected_char],
                ui->selected_color + 1);
            StockIcon_SetFrame(my_icon, ui->selected_char, ui->selected_color);
        }

        if (pad->down & HSD_BUTTON_A)
        {
            ui->local_ready = 1;
            send_char_selection(ui->selected_char, ui->selected_color);

            GXColor green = {0x21, 0xBA, 0x45, 0xFF};
            Text_SetText(my_ready_text, 0, "READY");
            Text_SetColor(my_ready_text, 0, &green);

            Text_SetText(ui->prompt_text, 0, "Waiting for opponent...");
            GXColor gray = {0x99, 0x99, 0x99, 0xFF};
            Text_SetColor(ui->prompt_text, 0, &gray);
        }
    }

    // --- Poll for opponent's selection ---
    if (ui->is_active_player && !ui->opponent_ready)
    {
        if (ui->frame_count % 10 == 0)
        {
            u8 opp_char, opp_color;
            if (fetch_opponent_selection(&opp_char, &opp_color))
            {
                ui->opponent_ready = 1;
                ui->opp_char = opp_char;
                ui->opp_color = opp_color;

                int local_is_p1 =
                    (ui->local_port == ui->active_ports[0]);
                Text *opp_char_text = local_is_p1 ?
                    ui->p2_char_text : ui->p1_char_text;
                Text *opp_ready_text = local_is_p1 ?
                    ui->p2_ready_text : ui->p1_ready_text;
                StockIcon *opp_icon = local_is_p1 ?
                    &ui->p2_icon : &ui->p1_icon;

                if (opp_char < NUM_CHARACTERS)
                {
                    Text_SetText(opp_char_text, 0,
                        "%s", (char *)char_names[opp_char]);
                    StockIcon_SetFrame(opp_icon, opp_char, opp_color);
                }

                GXColor green = {0x21, 0xBA, 0x45, 0xFF};
                Text_SetText(opp_ready_text, 0, "READY");
                Text_SetColor(opp_ready_text, 0, &green);
            }
        }
    }

    // --- Check if we should advance ---
    int should_advance = 0;

    if (ui->is_active_player)
    {
        if (ui->local_ready && ui->opponent_ready)
            should_advance = 1;

        if (ui->frame_count >= TIMER_TOTAL_FRAMES)
        {
            if (!ui->local_ready)
                send_char_selection(ui->selected_char, ui->selected_color);
            should_advance = 1;
        }
    }
    else
    {
        if (ui->frame_count >= SPECTATOR_WAIT)
            should_advance = 1;
    }

    if (should_advance)
        Scene_ExitMinor();
}

// ---------------------------------------------------------------------------
// minor_exit — called when leaving the minor scene
// ---------------------------------------------------------------------------
void minor_exit(void *unload_data)
{
    if (!ui) return;

    // Destroy text objects
    if (ui->lobby_name_text)   Text_Destroy(ui->lobby_name_text);
    if (ui->timer_text)        Text_Destroy(ui->timer_text);
    if (ui->player_count_text) Text_Destroy(ui->player_count_text);
    if (ui->match_header_text) Text_Destroy(ui->match_header_text);
    if (ui->p1_text)           Text_Destroy(ui->p1_text);
    if (ui->p2_text)           Text_Destroy(ui->p2_text);
    if (ui->p1_char_text)      Text_Destroy(ui->p1_char_text);
    if (ui->p2_char_text)      Text_Destroy(ui->p2_char_text);
    if (ui->p1_ready_text)     Text_Destroy(ui->p1_ready_text);
    if (ui->p2_ready_text)     Text_Destroy(ui->p2_ready_text);
    if (ui->vs_text)           Text_Destroy(ui->vs_text);
    if (ui->side_header_text)  Text_Destroy(ui->side_header_text);
    if (ui->prompt_text)       Text_Destroy(ui->prompt_text);
    if (ui->game_text)         Text_Destroy(ui->game_text);

    for (int i = 0; i < ROT_MAX_WAITING; i++)
    {
        if (ui->queue_texts[i])
            Text_Destroy(ui->queue_texts[i]);
    }

    // Free archive
    if (ui->gui_archive)
    {
        Archive_Free(ui->gui_archive);
        ui->gui_archive = 0;
    }

    HSD_Free(ui);
    ui = 0;
}
