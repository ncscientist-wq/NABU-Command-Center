/* NABU Command Center - Phase 2D-A04C3 final pre-freeze candidate.
 * Creator: Derek Leger
 * Copyright (c) 2026 Derek Leger. All rights reserved.
 * Product Version: UNASSIGNED; Working Project Version: v0.8
 */
#include <conio.h>
#include <psg.h>
#include <graphics.h>
#include <video/tms99x8.h>
#include <arch/nabu/retronet.h>
#include <arch/nabu.h>
#include <interrupt.h>
#include <intrinsic.h>

#define BUILD_ID "NCC-SPLASH-260826-LV2"
#ifndef NCC_DIAG_FLIGHT_RECORDER
#define NCC_DIAG_FLIGHT_RECORDER 0
#endif
#define KEY_RIGHT 0xF0
#define KEY_LEFT 0xF1
#define KEY_UP 0xF2
#define KEY_DOWN 0xF3
#define KEY_ENTER 0x0D
#define KEY_ESCAPE 0x1B
/* NABU keyboard encoding chart: CTRL-H backspace and DEL compatibility. */
#define KEY_BACKSPACE 0x08
#define KEY_DELETE 0x7F
#define RESOURCE_NAME "ncc_time.dat"
#define RESOURCE_NAME_LENGTH 12
#define ZIP_REQUEST_NAME "ncc_zip.req"
#define ZIP_REQUEST_NAME_LENGTH 11
#define ZIP_REQUEST_LENGTH 17
#define WEATHER_NAME "ncc_weather.dat"
#define WEATHER_NAME_LENGTH 15
#define LOCATION_NAME "ncc_location.dat"
#define LOCATION_NAME_LENGTH 16
#define WEATHER_HISTORY_NAME "ncc_wxhist.dat"
#define WEATHER_HISTORY_NAME_LENGTH 14
#define EARTHQUAKE_NAME "ncc_quake.dat"
#define EARTHQUAKE_NAME_LENGTH 13
#define SPACE_WEATHER_NAME "ncc_space.dat"
#define SPACE_WEATHER_NAME_LENGTH 13
#define WEATHER_ALERT_NAME "ncc_alert.dat"
#define WEATHER_ALERT_NAME_LENGTH 13
#define SATELLITE_NAME "ncc_satellite.dat"
#define SATELLITE_NAME_LENGTH 17
#define AIRSPACE_NAME "ncc_airspace.dat"
#define AIRSPACE_NAME_LENGTH 16
#define MUSIC_NAME "ncc_music.dat"
#define MUSIC_NAME_LENGTH 13
#define MUSIC_RECORD_SIZE 512
#define MUSIC_MAX_EVENTS 82
#define MUSIC_EVENT_SIZE 6
#define AIRSPACE_MAX_AIRCRAFT 3
#define AIRSPACE_SLOT_SIZE 17
#define WEATHER_ALERT_TEXT_CAPACITY 38
#define RECORD_LENGTH 64
#define RECORD_CAPACITY 65
#define TEXT_CAPACITY 17
#define FIELD_COUNT 10
#define AUTO_REFRESH_SECONDS 60

/* Installed z88dk exports this mask accessor but omits it from arch/nabu.h. */
extern void ncc_install_minimal_vdp_isr(void);
extern unsigned char nabu_get_interrupts(void);
static void service_scheduler(void);

#define STATUS_NOT_CONFIGURED 0
#define STATUS_LIVE 1
#define STATUS_CACHED 2
#define STATUS_STALE 3
#define STATUS_OFFLINE 4
#define STATUS_INVALID 5

#define VIEW_DASHBOARD 0
#define VIEW_MODULE 1
#define VIEW_DIAGNOSTICS 2
#define VIEW_HELP 3
#define VIEW_IDLE 4
#define VIEW_ZIP 5

#define DETAIL_LEFT 8
#define DETAIL_RIGHT 172
#define DETAIL_TOP 40
#define DETAIL_BOTTOM 151
#define DETAIL_HORIZON 54
#define DETAIL_VANISH_X 90
#define VIEW_HUD_Y 42
#define VIEW_HUD_H 10
#define VIEW_GRAPHICS_TOP 53
#define VIEW_GRAPHICS_BOTTOM 158
#define AIRSPACE_PLOT_LEFT 8
#define AIRSPACE_PLOT_RIGHT 172
#define AIRSPACE_PLOT_TOP 53
#define AIRSPACE_PLOT_BOTTOM 158
#define AIRSPACE_MAP_LEFT 10
#define AIRSPACE_MAP_RIGHT 170
#define AIRSPACE_MAP_TOP 54
#define AIRSPACE_MAP_BOTTOM 157

#define DRAW_STAGE_BEGIN 0
#define DRAW_STAGE_HEADER 1
#define DRAW_STAGE_FRAME 2
#define DRAW_STAGE_STATIC 3
#define DRAW_STAGE_DYNAMIC 4
#define DRAW_STAGE_TEXT 5
#define DRAW_STAGE_COMPLETE 6

#define MINI_UPDATE_DIVISOR 900
#define MAX_UPDATE_DIVISOR 300
#define CLOCK_FRAMES_PER_SECOND 60
#define SPLASH_TIMEOUT_FRAMES 600U
#define SPLASH_EVENT_COUNT 32

#define WX_STAGE_IDLE 0
#define WX_STAGE_R_ENTERED 1
#define WX_STAGE_OPEN_START 2
#define WX_STAGE_OPEN_RETURNED 3
#define WX_STAGE_READ_START 4
#define WX_STAGE_READ_RETURNED 5
#define WX_STAGE_PARSE 6
#define WX_STAGE_ZIP_FAIL 7
#define WX_STAGE_TOKEN_FAIL 8
#define WX_STAGE_CRC_FAIL 9
#define WX_STAGE_ACCEPT 10
#define WX_STAGE_CACHE_COMMITTED 11
#define WX_STAGE_DISPLAY 12

#define SAFE_LEFT 8
#define SAFE_RIGHT 247
#define SAFE_TOP 4
#define SAFE_BOTTOM 183
#define HEADER_BOTTOM 28
#define TICKER_TOP 30
#define TICKER_BOTTOM 38
#define FOOTER_TOP 166
#define MICRO_WIDTH 5
#define MICRO_HEIGHT 7
#define MICRO_ADVANCE 6

#define DIAG_RING_DEPTH 32
#define DIAG_GUARD_SIZE 4
#define DIAG_CTX_NONE 0
#define DIAG_CTX_TIME 1
#define DIAG_CTX_WEATHER 2
#define DIAG_CTX_UI 3
#define DIAG_CTX_RENDER 4

#define DE_BOOT_START 0x01
#define DE_LOGGER_INIT 0x02
#define DE_FRAME_REG_BEGIN 0x03
#define DE_FRAME_REG_END 0x04
#define DE_BOOT_READY 0x05
#define DE_KEY_R 0x10
#define DE_ZIP_SUBMIT 0x11
#define DE_VIEW_OPEN 0x12
#define DE_VIEW_BACK 0x13
#define DE_VIEW_CHANGE 0x14
#define DE_MODE_CHANGE 0x15
#define DE_SELECTION_CHANGE 0x16
#define DE_TIME_DISPATCH 0x20
#define DE_TIME_OPEN_BEGIN 0x21
#define DE_TIME_OPEN_END 0x22
#define DE_TIME_READ_BEGIN 0x23
#define DE_TIME_READ_END 0x24
#define DE_TIME_PARSE_BEGIN 0x25
#define DE_TIME_PARSE_OK 0x26
#define DE_TIME_PARSE_FAIL 0x27
#define DE_TIME_CLOSE_BEGIN 0x28
#define DE_TIME_CLOSE_END 0x29
#define DE_TIME_CACHE_COMMIT 0x2A
#define DE_TIME_REFRESH_RETURN 0x2B
#define DE_WX_DISPATCH 0x30
#define DE_WX_OPEN_BEGIN 0x31
#define DE_WX_OPEN_END 0x32
#define DE_WX_READ_BEGIN 0x33
#define DE_WX_READ_END 0x34
#define DE_WX_PARSE_BEGIN 0x35
#define DE_WX_PARSE_OK 0x36
#define DE_WX_PARSE_FAIL 0x37
#define DE_WX_ZIP_FAIL 0x38
#define DE_WX_TOKEN_FAIL 0x39
#define DE_WX_INTEGRITY_FAIL 0x3A
#define DE_WX_CLOSE_BEGIN 0x3B
#define DE_WX_CLOSE_END 0x3C
#define DE_WX_CACHE_COMMIT 0x3D
#define DE_WX_REFRESH_RETURN 0x3E
#define DE_DRAW_DASH_BEGIN 0x50
#define DE_DRAW_DASH_END 0x51
#define DE_DRAW_WX_BEGIN 0x52
#define DE_DRAW_WX_END 0x53
#define DE_CLOCK_PATCH_BEGIN 0x60
#define DE_CLOCK_PATCH_END 0x61
#define DE_CLOCK_SYNC_COMMIT 0x62
#define DE_GUARD_FAIL 0x70

/* Global Command Center semantic theme mapped to installed TMS9918 colors. */
#define CC_BACKGROUND VDP_INK_BLACK
#define CC_PRIMARY_VECTOR VDP_INK_CYAN
#define CC_SECONDARY_VECTOR VDP_INK_LIGHT_BLUE
#define CC_PRIMARY_TEXT VDP_INK_WHITE
#define CC_SECONDARY_TEXT VDP_INK_CYAN
#define CC_SELECTED VDP_INK_LIGHT_YELLOW
#define CC_NOMINAL VDP_INK_LIGHT_GREEN
#define CC_WARNING VDP_INK_DARK_YELLOW
#define CC_ALERT VDP_INK_LIGHT_RED
#define CC_SPECIAL VDP_INK_MAGENTA

static const char *module_names[6] = {
    "EARTHQUAKE", "SPACE WEATHER", "SATELLITE / ISS",
    "AIRSPACE / ADS-B", "LOCAL WEATHER", "NABU SYSTEM / TASK MANAGER"
};
static const char *module_mock[6] = {
    "M4.2  MOCK REGION", "KP 3  SOLAR 440",
    "ISS EL 35 AZ 225", "AIRSPACE WAITING",
    "WEATHER WAITING", "MODE2 TASKS READY"
};
static const char *panel_label[6] = {
    "EARTHQUAKE", "SPACE WX", "SAT / ISS",
    "AIRSPACE", "LOCAL WX", "NABU TASK MGR"
};
static const char *panel_metric[6] = {
    "M4.2 D12", "KP3 SW440", "EL35 AZ225",
    "AS WAIT", "WX WAIT", "MODE2 TASK OK"
};
static const unsigned char panel_x[6] = {8, 89, 170, 8, 89, 170};
static const unsigned char panel_y[6] = {40, 40, 40, 96, 96, 96};
static const char *news_title[6] = {
    "LOCAL: PROFILE READY",
    "SPACE: SWPC READY",
    "ORBIT: PASS UPDATED",
    "AIR: CONTACTS UPDATED",
    "QUAKE: EVENTS REVIEWED",
    "SYSTEM: STORE64 READY"
};
static const char *news_summary[6] = {
    "DEMO LOCAL CONTEXT", "SWPC KP / WIND DATA", "MOCK OBJECT TRACKS",
    "LIVE AIRSPACE DATA", "MOCK DEPTH EVENTS", "REAL TRANSPORT STATE"
};

static const char *severity_text[6] = {"ADV","ADV","NOM","NOM","NOM","NOM"};

/* Sixteen-step signed fixed-point circle, scale 64. Shared by globe views. */
static const signed char circle_x16[16] = {64,59,45,24,0,-24,-45,-59,-64,-59,-45,-24,0,24,45,59};
static const signed char circle_y16[16] = {0,24,45,59,64,59,45,24,0,-24,-45,-59,-64,-59,-45,-24};

static const char *sat_name[4] = {"ISS","NOAA19","METEOR","STARLINK"};
static const char *sat_id[4] = {"25544","33591","40069","48274"};

/* 5x7 Command Center font: digits, A-Z, and used punctuation glyphs. */
static const unsigned char font_digit[10][7] = {
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
 {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
 {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
 {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
};
static const unsigned char font_letter[26][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
 {14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
 {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
 {14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
 {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
 {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
 {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
 {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
 {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static const char font_punct_char[11] = ".:/-+%#()_";
static const unsigned char font_punct[10][7] = {
 {0,0,0,0,0,12,12},{0,12,12,0,12,12,0},
 {1,2,2,4,8,8,16},{0,0,0,31,0,0,0},
 {0,4,4,31,4,4,0},{17,2,4,8,16,17,0},
 {10,31,10,10,31,10,0},{2,4,8,8,8,4,2},
 {8,4,2,2,2,4,8},{0,0,0,0,0,0,31}
};

static char record_buffer[RECORD_CAPACITY];
static char parse_buffer[RECORD_CAPACITY];
static char last_text[TEXT_CAPACITY + 1];
static char zip_code[6] = "90210";
static char zip_request[ZIP_REQUEST_LENGTH]={'Z','I','P','|','0','0','0','0','0','1','|','9','0','2','1','0','\n'};
static unsigned long zip_request_sequence = 0;
static unsigned char zip_request_valid = 1;
static unsigned char has_weather = 0;
static char weather_token[7];
static char weather_zip[6];
static char weather_temp[4];
static char weather_condition[9];
static char weather_wind[4];
static char weather_pressure[5];
static unsigned long weather_source_utc = 0;
static unsigned char has_location = 0;
static unsigned char location_resolved = 0;
static char location_token[7];
static char location_zip[6];
static char location_city[17] = "UNKNOWN";
static char location_state[3] = "--";
static char location_label[20] = "UNKNOWN";
static char location_short[11] = "UNRES";
static unsigned char weather_history_count = 0;
static unsigned char weather_history_temp[12];
static unsigned char weather_history_pressure[12];
static unsigned char weather_history_wind[12];
static unsigned long weather_history_utc = 0;
static char weather_diag[12] = "WX READY";
static unsigned char has_quake = 0;
static unsigned char quake_sequence = 0;
static unsigned char quake_local_count = 0;
static unsigned char quake_global_count = 0;
static unsigned char quake_mag10[4];
static unsigned char quake_depth[4];
static int quake_lat100[4];
static int quake_lon100[4];
static unsigned int quake_age_minutes[4];
static char quake_region[4][5];
static unsigned char quake_event_id[4];
static char quake_metric[11] = "EQ WAIT";
static char quake_mag_text[7] = "MAG---";
static char quake_depth_text[9] = "DEPTH---";
static unsigned char has_space_weather = 0;
static unsigned char space_sequence = 0, space_kp10 = 0, space_bt10 = 0, space_flux = 0, space_history_count = 0, space_severity = 0;
static unsigned int space_speed = 0;
static signed char space_bz10 = 0;
static unsigned char space_kp_history[12];
static unsigned int space_speed_history[12];
static unsigned long space_source_utc = 0;
static char space_metric[13] = "SW WAIT";
static char space_kp_text[6] = "KP---";
static char space_speed_text[7] = "SW----";
static unsigned char has_satellite = 0;
static unsigned char satellite_sequence = 0;
static int satellite_latitude100 = 0;
static int satellite_longitude100 = 0;
static unsigned int satellite_altitude_km = 0;
static unsigned long satellite_velocity_kmh = 0;
static unsigned long satellite_timestamp = 0;
static unsigned int satellite_footprint_km = 0;
static unsigned char satellite_visibility = 0;
static char satellite_identity[6] = "ISS";
static char satellite_norad[6] = "25544";
static char satellite_position_text[25] = "LAT---- LON----";
static char satellite_motion_text[25] = "ALT---- VEL----";
static char satellite_status_text[12] = "WAIT";
static unsigned char has_airspace = 0;
static unsigned char airspace_sequence = 0, airspace_state = 3, airspace_source = 2, airspace_count = 0;
static char airspace_callsign[AIRSPACE_MAX_AIRCRAFT][7];
static char airspace_icao[AIRSPACE_MAX_AIRCRAFT][7];
static unsigned char airspace_x[AIRSPACE_MAX_AIRCRAFT], airspace_y[AIRSPACE_MAX_AIRCRAFT];
static unsigned char airspace_alt100[AIRSPACE_MAX_AIRCRAFT], airspace_speed2[AIRSPACE_MAX_AIRCRAFT], airspace_heading2[AIRSPACE_MAX_AIRCRAFT];
static unsigned char has_weather_alert = 0;
static unsigned char weather_alert_sequence = 0, weather_alert_severity = 0, weather_alert_count = 0;
static unsigned char weather_alert_length = 0;
static unsigned long weather_alert_source_utc = 0;
static char weather_alert_text[WEATHER_ALERT_TEXT_CAPACITY+1];
static volatile unsigned char weather_stage = WX_STAGE_IDLE;
static unsigned char selected = 0;
static unsigned char news_index = 0;
static unsigned char console_global = 0;
static unsigned char selected_target = 0;
static unsigned char target_locked = 1;
static unsigned char current_view = VIEW_DASHBOARD;
static unsigned char sound_enabled = 1;
/* Default to a stable display. P remains available to opt into demo animation. */
static unsigned char scheduler_paused = 1;
static unsigned char task_page = 0;
static unsigned char visual_dirty = 0;
static unsigned char globe_phase = 0;
static unsigned int scheduler_ticks = 0;
static unsigned char mini_module = 0;
static unsigned char mini_phase = 0;
static unsigned char current_draw_stage = DRAW_STAGE_COMPLETE;
static unsigned char last_draw_stage = DRAW_STAGE_COMPLETE;
static unsigned char dirty_mask = 0;
static unsigned int full_draw_count = 0;
static unsigned int dirty_draw_count = 0;
static unsigned int static_draw_count = 0;
static unsigned int dynamic_draw_count = 0;
static unsigned int frame3d_count = 0;
static unsigned int globe_step_count = 0;
static unsigned int news_step_count = 0;
static unsigned char has_last_valid = 0;
static unsigned char current_status = STATUS_NOT_CONFIGURED;
static unsigned long last_sequence = 0;
static unsigned long last_utc = 0;
static unsigned int last_value = 0;
static unsigned int bytes_received = 0;
static int last_return_value = 0;
static const char *last_error = "NONE";
static const char *last_checkpoint = "T01 READY";
static const char *transient_status = "SYSTEM READY";

/* NABU/TMS9918 frame-sync timebase. The ISR only counts bounded events. */
static volatile unsigned char clock_frame_phase = 0;
static volatile unsigned char clock_seconds_pending = 0;
volatile unsigned char clock_frame_counter = 0;
static unsigned char clock_frame_base = 0;
static unsigned int clock_frame_accumulator = 0;
static unsigned char auto_refresh_elapsed = 0;
static unsigned char auto_refresh_pending = 0;
static unsigned char manual_refresh_pending = 0;
static unsigned char refresh_in_progress = 0;
volatile unsigned int ncc_gate0_read_count = 0;
volatile unsigned int ncc_gate0_short_read_count = 0;
volatile unsigned int ncc_gate0_open_fail_count = 0;
volatile unsigned int ncc_gate0_close_count = 0;
volatile unsigned char ncc_gate0_endurance_done = 0;
volatile unsigned int ncc_gate0_endurance_target = 5000;

typedef struct {
    unsigned char event_id;
    unsigned char sequence_low;
    unsigned char sequence_high;
    unsigned char context;
    unsigned char arg0;
    unsigned char arg1;
    unsigned char state;
    unsigned char tick;
} NccDiagEvent;

/* External linkage deliberately retains stable debugger-visible symbols. */
unsigned char ncc_diag_pre_guard[DIAG_GUARD_SIZE];
#if NCC_DIAG_FLIGHT_RECORDER
NccDiagEvent ncc_diag_ring[DIAG_RING_DEPTH];
#endif
unsigned char ncc_diag_post_guard[DIAG_GUARD_SIZE];
unsigned char ncc_diag_head;
unsigned int ncc_diag_sequence;
unsigned char ncc_diag_last_event;
unsigned char ncc_diag_context;
unsigned char ncc_diag_time_attempt;
unsigned char ncc_diag_weather_attempt;
unsigned char ncc_diag_guard_status;
unsigned char ncc_diag_last_selected;
unsigned char ncc_diag_last_view;
unsigned char ncc_diag_tick_snapshot;
static unsigned char ncc_diag_guard_divider;

#if NCC_DIAG_FLIGHT_RECORDER
static void diag_log(unsigned char event_id, unsigned char context,
                     unsigned char arg0, unsigned char arg1)
{
    NccDiagEvent *slot=&ncc_diag_ring[ncc_diag_head];
    unsigned int sequence=(unsigned int)(ncc_diag_sequence+1);
    slot->event_id=0;
    slot->sequence_low=(unsigned char)sequence;
    slot->sequence_high=(unsigned char)(sequence>>8);
    slot->context=context;
    slot->arg0=arg0;
    slot->arg1=arg1;
    slot->state=(unsigned char)((selected&7)|((current_view&7)<<3));
    slot->tick=clock_frame_phase;
    slot->event_id=event_id;
    ncc_diag_sequence=sequence;
    ncc_diag_last_event=event_id;
    ncc_diag_context=context;
    ncc_diag_last_selected=selected;
    ncc_diag_last_view=current_view;
    ncc_diag_tick_snapshot=clock_frame_phase;
    ncc_diag_head=(unsigned char)((ncc_diag_head+1)&(DIAG_RING_DEPTH-1));
}
#else
#define diag_log(event_id,context,arg0,arg1) ((void)0)
#endif

static void diag_init(void)
{
    unsigned char i;
    for(i=0;i<DIAG_GUARD_SIZE;++i) {
        ncc_diag_pre_guard[i]=(unsigned char)(0xA5+i);
        ncc_diag_post_guard[i]=(unsigned char)(0x5A-i);
    }
#if NCC_DIAG_FLIGHT_RECORDER
    for(i=0;i<DIAG_RING_DEPTH;++i) ncc_diag_ring[i].event_id=0;
#endif
    ncc_diag_head=0; ncc_diag_sequence=0; ncc_diag_last_event=0;
    ncc_diag_context=DIAG_CTX_NONE; ncc_diag_time_attempt=0;
    ncc_diag_weather_attempt=0; ncc_diag_guard_status=0;
    diag_log(DE_BOOT_START,DIAG_CTX_NONE,0,0);
    diag_log(DE_LOGGER_INIT,DIAG_CTX_NONE,DIAG_RING_DEPTH,sizeof(NccDiagEvent));
}

/* State-driven AY cue: one bounded scheduler step per main-loop iteration. */
static unsigned char cue_kind = 0;
static unsigned char cue_step = 0;
static unsigned int cue_ticks = 0;

/* WATCHFLOOR DRIVE: original local AY loop.
 * More active two-voice command-center music. Music owns B/C; UI cues retain channel A.
 * Star Sabre - Ingame was used only as a high-level energy/complexity reference;
 * this note data is an original composition.
 */
/* Fixed 512-byte MU01 buffer; Gateway converts complete MIDI to this bounded
 * NabuTracker-style timed two-voice event window. No allocation or ISR owner. */
static unsigned char music_buffer[MUSIC_RECORD_SIZE];
static unsigned char music_enabled = 0;
static unsigned char music_valid = 0;
static unsigned char music_event_count = 0;
static unsigned char music_source = 0;
static unsigned char music_position = 0;
static unsigned char music_wait = 0;
static unsigned char music_frame_mark = 0;
static unsigned char music_loop_pending = 0;
static unsigned char music_started = 0;

static void delay_loop(void)
{
    volatile unsigned int i;
    for (i = 0; i < 700; ++i) { }
}

static void clear_line(unsigned char row)
{
    gotoxy(0, row);
    textcolor(WHITE);
    cputs("                                ");
}

static void put_line(unsigned char row, unsigned char color, const char *text)
{
    clear_line(row);
    gotoxy(0, row);
    textcolor(color);
    cputs(text);
}

static void print_unsigned(unsigned long value)
{
    char digits[11];
    unsigned char count = 0;
    if (value == 0) { cputc('0'); return; }
    while ((value != 0) && (count < sizeof(digits))) {
        digits[count++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (count > 0) cputc(digits[--count]);
}

static unsigned int text_length(const char *text)
{
    unsigned int n = 0;
    while ((text[n] != 0) && (n < 32767)) ++n;
    return n;
}

static unsigned char text_equal(const char *a, const char *b)
{
    unsigned int i = 0;
    while ((a[i] != 0) && (b[i] != 0)) {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return (unsigned char)(a[i] == b[i]);
}

static void text_copy_bounded(char *dst, const char *src, unsigned int cap)
{
    unsigned int i = 0;
    if (cap == 0) return;
    while ((i + 1 < cap) && (src[i] != 0)) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static unsigned char decimal_value(const char *text, unsigned long maximum,
                                   unsigned long *result)
{
    unsigned long value = 0;
    unsigned int i = 0;
    unsigned char digit;
    if (text[0] == 0) return 0;
    while (text[i] != 0) {
        if ((text[i] < '0') || (text[i] > '9')) return 0;
        digit = (unsigned char)(text[i] - '0');
        if (value > ((maximum - digit) / 10)) return 0;
        value = value * 10 + digit;
        ++i;
    }
    *result = value;
    return 1;
}

static unsigned char hex_value4(const char *text, unsigned int *result)
{
    unsigned int value = 0;
    unsigned char i, nibble;
    if (text_length(text) != 4) return 0;
    for (i = 0; i < 4; ++i) {
        if ((text[i] >= '0') && (text[i] <= '9')) nibble = text[i] - '0';
        else if ((text[i] >= 'A') && (text[i] <= 'F')) nibble = 10 + text[i] - 'A';
        else if ((text[i] >= 'a') && (text[i] <= 'f')) nibble = 10 + text[i] - 'a';
        else return 0;
        value = (unsigned int)((value << 4) | nibble);
    }
    *result = value;
    return 1;
}

static unsigned int checksum16(const char *data, unsigned int length)
{
    unsigned int sum = 0, i;
    for (i = 0; i < length; ++i) sum += (unsigned char)data[i];
    return sum;
}

static const char *status_text(void)
{
    switch (current_status) {
        case STATUS_LIVE: return "LIVE";
        case STATUS_CACHED: return "CACHED";
        case STATUS_STALE: return "STALE";
        case STATUS_OFFLINE: return "OFFLINE";
        case STATUS_INVALID: return "INVALID";
        default: return "NOT CFG";
    }
}

static void ay_note(unsigned int period)
{
    unsigned char mixer = (unsigned char)get_psg(7);
    /* Enable A tone and disable A noise without disturbing music B/C bits. */
    set_psg(7, (mixer & 0xf6) | 0x08);
    set_psg(0, period & 0xff);
    set_psg(1, (period >> 8) & 0x0f);
    set_psg(8, 10);
}

static void ay_silence(void) { set_psg(8, 0); }

static void stop_cue(void)
{
    ay_silence();
    cue_kind=0; cue_step=0; cue_ticks=0;
}

static void weather_diag_set(const char *text)
{
    text_copy_bounded(weather_diag,text,sizeof(weather_diag));
}

static void weather_diag_dispatch(void)
{
    weather_diag[0]='R'; weather_diag[1]=' '; weather_diag[2]='S'; weather_diag[3]='=';
    weather_diag[4]=(char)('0'+selected); weather_diag[5]=' '; weather_diag[6]='V'; weather_diag[7]='=';
    weather_diag[8]=(char)('0'+current_view); weather_diag[9]=0;
}

static unsigned char weather_matches_selection(void)
{
    unsigned char i;
    if(!has_weather || !zip_request_valid || !text_equal(weather_zip,zip_code)) return 0;
    for(i=0;i<6;++i) if(weather_token[i]!=zip_request[4+i]) return 0;
    return 1;
}

static unsigned char location_matches_selection(void)
{
    unsigned char i;
    if(!has_location || !zip_request_valid || !text_equal(location_zip,zip_code)) return 0;
    for(i=0;i<6;++i) if(location_token[i]!=zip_request[4+i]) return 0;
    return 1;
}

static void format_location_labels(void)
{
    unsigned char i=0, j=0, city_limit;
    if(!location_resolved) {
        text_copy_bounded(location_label,"UNKNOWN",sizeof(location_label));
        text_copy_bounded(location_short,"UNKNOWN",sizeof(location_short));
        return;
    }
    while(location_city[i] && j<16) location_label[j++]=location_city[i++];
    location_label[j++]=' '; location_label[j++]=location_state[0]; location_label[j++]=location_state[1]; location_label[j]=0;
    city_limit=7; i=0; j=0;
    while(location_city[i] && j<city_limit) location_short[j++]=location_city[i++];
    location_short[j++]=' '; location_short[j++]=location_state[0]; location_short[j++]=location_state[1]; location_short[j]=0;
}

static unsigned char quake_count(void) { return console_global?quake_global_count:quake_local_count; }
static unsigned char quake_slot(void)
{
    unsigned char count=quake_count();
    if(!count) return 0;
    return (unsigned char)((console_global?quake_local_count:0)+(selected_target%count));
}
static int signed16_le(const unsigned char *data)
{
    unsigned int value=(unsigned int)data[0]|((unsigned int)data[1]<<8);
    if(value&0x8000U) return -(int)(((~value)+1U)&0xffffU);
    return (int)value;
}
static void format_quake_text(unsigned char slot)
{
    unsigned char mag=quake_mag10[slot], depth=quake_depth[slot];
    quake_mag_text[0]='M'; quake_mag_text[1]='A'; quake_mag_text[2]='G'; quake_mag_text[3]=(char)('0'+mag/10); quake_mag_text[4]='.'; quake_mag_text[5]=(char)('0'+mag%10); quake_mag_text[6]=0;
    quake_depth_text[0]='D'; quake_depth_text[1]='E'; quake_depth_text[2]='P'; quake_depth_text[3]='T'; quake_depth_text[4]='H'; quake_depth_text[5]=(char)('0'+depth/100); quake_depth_text[6]=(char)('0'+(depth/10)%10); quake_depth_text[7]=(char)('0'+depth%10); quake_depth_text[8]=0;
    quake_metric[0]='M'; quake_metric[1]=(char)('0'+mag/10); quake_metric[2]='.'; quake_metric[3]=(char)('0'+mag%10); quake_metric[4]=' '; quake_metric[5]='D'; quake_metric[6]=(char)('0'+depth/100); quake_metric[7]=(char)('0'+(depth/10)%10); quake_metric[8]=(char)('0'+depth%10); quake_metric[9]=0;
}
static unsigned char validate_quake_record(const unsigned char *data, unsigned int length)
{
    unsigned char i, count;
    unsigned int calculated=0, received;
    if(length!=RECORD_LENGTH || data[0]!='E'||data[1]!='Q'||data[2]!='0'||data[3]!='1'||data[62]!='E'||data[63]!=10) return 0;
    count=(unsigned char)(data[5]+data[6]);
    if(data[5]>2 || data[6]>2 || count>4) return 0;
    for(i=0;i<60;++i) calculated+=(unsigned int)data[i];
    received=(unsigned int)data[60]|((unsigned int)data[61]<<8);
    if(calculated!=received) return 0;
    quake_sequence=data[4]; quake_local_count=data[5]; quake_global_count=data[6];
    for(i=0;i<count;++i) {
        unsigned char offset=(unsigned char)(8+i*13), j;
        quake_mag10[i]=data[offset]; quake_depth[i]=data[offset+1]; quake_lat100[i]=signed16_le(&data[offset+2]); quake_lon100[i]=signed16_le(&data[offset+4]);
        quake_age_minutes[i]=(unsigned int)data[offset+6]|((unsigned int)data[offset+7]<<8);
        for(j=0;j<4;++j) quake_region[i][j]=(char)data[offset+8+j];
        quake_region[i][4]=0; quake_event_id[i]=data[offset+12];
    }
    has_quake=(unsigned char)(count!=0); selected_target=0;
    if(has_quake) format_quake_text(quake_slot());
    return 1;
}

static void format_space_text(void)
{
    unsigned int speed=space_speed;
    if(speed>9999U) speed=9999U;
    space_kp_text[0]='K'; space_kp_text[1]='P'; space_kp_text[2]=(char)('0'+space_kp10/10); space_kp_text[3]='.'; space_kp_text[4]=(char)('0'+space_kp10%10); space_kp_text[5]=0;
    space_speed_text[0]='S'; space_speed_text[1]='W'; space_speed_text[2]=(char)('0'+speed/1000); space_speed_text[3]=(char)('0'+(speed/100)%10); space_speed_text[4]=(char)('0'+(speed/10)%10); space_speed_text[5]=(char)('0'+speed%10); space_speed_text[6]=0;
    space_metric[0]='K'; space_metric[1]='P'; space_metric[2]=(char)('0'+space_kp10/10); space_metric[3]='.'; space_metric[4]=(char)('0'+space_kp10%10); space_metric[5]=' '; space_metric[6]='S'; space_metric[7]='W'; space_metric[8]=(char)('0'+(speed/100)%10); space_metric[9]=(char)('0'+(speed/10)%10); space_metric[10]=(char)('0'+speed%10); space_metric[11]=0;
}

static unsigned char validate_space_record(const unsigned char *data, unsigned int length)
{
    unsigned char i, count; unsigned int calculated=0, received;
    if(length!=RECORD_LENGTH || data[0]!='S'||data[1]!='W'||data[2]!='0'||data[3]!='1'||data[62]!='E'||data[63]!=10) return 0;
    count=data[11]; if(count==0 || count>12) return 0;
    for(i=0;i<60;++i) calculated+=(unsigned int)data[i];
    received=(unsigned int)data[60]|((unsigned int)data[61]<<8); if(calculated!=received) return 0;
    space_sequence=data[4]; space_kp10=data[5]; space_speed=(unsigned int)data[6]|((unsigned int)data[7]<<8);
    space_bt10=data[8]; space_bz10=(signed char)data[9]; space_flux=data[10]; space_history_count=count;
    for(i=0;i<count;++i) { space_kp_history[i]=data[12+i]; space_speed_history[i]=(unsigned int)data[24+i*2]|((unsigned int)data[25+i*2]<<8); }
    space_source_utc=(unsigned long)data[48]|((unsigned long)data[49]<<8)|((unsigned long)data[50]<<16)|((unsigned long)data[51]<<24);
    space_severity=data[52]; has_space_weather=1; format_space_text(); return 1;
}

static unsigned char validate_weather_alert_record(const unsigned char *data, unsigned int length)
{
    unsigned char i, text_length;
    unsigned int calculated=0, received;
    if(length!=RECORD_LENGTH || data[0]!='W'||data[1]!='A'||data[2]!='0'||data[3]!='1'||data[63]!=10) return 0;
    for(i=0;i<6;++i) if(data[4+i]!=(unsigned char)zip_request[4+i]) return 0;
    for(i=0;i<5;++i) if(data[10+i]!=(unsigned char)zip_code[i]) return 0;
    text_length=data[18];
    if(text_length==0 || text_length>WEATHER_ALERT_TEXT_CAPACITY || data[16]>4) return 0;
    for(i=0;i<61;++i) calculated+=(unsigned int)data[i];
    received=(unsigned int)data[61]|((unsigned int)data[62]<<8);
    if(calculated!=received) return 0;
    for(i=0;i<text_length;++i) if(data[23+i]<32 || data[23+i]>126) return 0;
    weather_alert_sequence=data[15]; weather_alert_severity=data[16]; weather_alert_count=data[17];
    weather_alert_length=text_length;
    weather_alert_source_utc=(unsigned long)data[19]|((unsigned long)data[20]<<8)|((unsigned long)data[21]<<16)|((unsigned long)data[22]<<24);
    for(i=0;i<text_length;++i) weather_alert_text[i]=(char)data[23+i];
    weather_alert_text[text_length]=0;
    has_weather_alert=1;
    return 1;
}

static void music_silence(void)
{
    set_psg(9,0); set_psg(10,0);
    set_psg(7,(unsigned char)get_psg(7)|0x06);
}

static void music_event(unsigned int period_b, unsigned int period_c,
                        unsigned char volume)
{
    unsigned char mixer=(unsigned char)get_psg(7);

    if(period_b!=0) {
        set_psg(2,period_b&0xff);
        set_psg(3,(period_b>>8)&0x0f);
        set_psg(9,(unsigned char)(volume>>4));
    } else {
        set_psg(9,0);
    }

    if(period_c!=0) {
        set_psg(4,period_c&0xff);
        set_psg(5,(period_c>>8)&0x0f);
        set_psg(10,(unsigned char)(volume&0x0f));
    } else {
        set_psg(10,0);
    }

    /* Preserve A cue state; enable B/C tone and disable B/C noise. */
    set_psg(7,(mixer&0xc9)|0x30);
}

static void service_music(void)
{
    unsigned int offset, period_b, period_c;
    unsigned char duration, volume, now, elapsed, processed;
    if(!music_enabled) return;
    if(!music_valid) return;
    now=clock_frame_counter;
    elapsed=(unsigned char)(now-music_frame_mark);
    if(elapsed==0) return;
    music_frame_mark=now;
    if(!music_started) { music_started=1; elapsed=0; }
    for(processed=0;processed<MUSIC_MAX_EVENTS;++processed) {
        if(music_wait) {
            if(elapsed<music_wait) { music_wait=(unsigned char)(music_wait-elapsed); return; }
            elapsed=(unsigned char)(elapsed-music_wait); music_wait=0;
        }
        if(music_loop_pending) { music_silence(); music_loop_pending=0; }
        offset=(unsigned int)12U+(unsigned int)music_position*MUSIC_EVENT_SIZE;
        duration=music_buffer[offset];
        period_b=(unsigned int)music_buffer[offset+1]|((unsigned int)music_buffer[offset+2]<<8);
        period_c=(unsigned int)music_buffer[offset+3]|((unsigned int)music_buffer[offset+4]<<8);
        volume=music_buffer[offset+5];
        music_event(period_b,period_c,volume);
        music_wait=duration;
        ++music_position;
        if(music_position>=music_event_count) { music_position=0; music_loop_pending=1; }
        if(elapsed==0) return;
    }
}

static void start_cue(unsigned char kind)
{
    stop_cue();
    if (!sound_enabled) return;
    cue_kind = kind;
    cue_step = 0;
    cue_ticks = 1;
}

static void service_sound(void)
{
    /* Original compact cues: startup fanfare and distinct one/two-note actions. */
    static const unsigned int notes[8] = {453,381,303,254,227,339,285,214};
    static const unsigned char cue_length[9] = {0,5,1,2,1,2,2,2,2};
    static const unsigned char cue_base[9] = {0,0,5,2,6,1,3,4,0};
    unsigned char limit;
    if (cue_kind == 0) return;
    if (cue_ticks > 0) { --cue_ticks; return; }
    limit = cue_length[cue_kind<9?cue_kind:0];
    if (cue_step >= limit) { stop_cue(); return; }
    ay_note(notes[(cue_base[cue_kind]+cue_step)&7]);
    ++cue_step;
    cue_ticks = (cue_kind==2)?10:22;
}

static void count_inc(unsigned int *value)
{
    if(*value!=65535U) ++*value;
}

static void draw_stage(unsigned char stage)
{
    last_draw_stage=current_draw_stage;
    current_draw_stage=stage;
}

static void draw_header(const char *subtitle)
{
    textbackground(BLUE);
    textcolor(YELLOW);
    clrscr();
    gotoxy(5, 0); cputs("NABU COMMAND CENTER");
    gotoxy(1, 1); textcolor(LIGHTCYAN); cputs("PHASE 2 NATIVE INTEGRATION");
    gotoxy(1, 2); textcolor(WHITE); cputs(BUILD_ID);
    gotoxy(1, 3); textcolor(LIGHTGREEN); cputs(subtitle);
}

static void cc_color(unsigned char foreground)
{
    vdp_color(foreground,CC_BACKGROUND,CC_BACKGROUND);
}

static unsigned char cc_status_color(void)
{
    if ((current_status==STATUS_LIVE) || (current_status==STATUS_CACHED)) return CC_NOMINAL;
    if (current_status==STATUS_STALE) return CC_WARNING;
    if ((current_status==STATUS_OFFLINE) || (current_status==STATUS_INVALID)) return CC_ALERT;
    return CC_SECONDARY_TEXT;
}

static const char *cc_status_short(void)
{
    if(current_status==STATUS_LIVE) return "LIVE";
    if(current_status==STATUS_CACHED) return "CACHE";
    if(current_status==STATUS_STALE) return "STALE";
    if(current_status==STATUS_OFFLINE) return "OFF";
    if(current_status==STATUS_INVALID) return "INV";
    return "NC";
}

static unsigned char micro_glyph_row(char c, unsigned char row)
{
    unsigned char i;
    if ((c>='0') && (c<='9')) return font_digit[c-'0'][row];
    if ((c>='A') && (c<='Z')) return font_letter[c-'A'][row];
    for (i=0;i<10;++i) if (font_punct_char[i]==c) return font_punct[i][row];
    return 0;
}

static void micro_char(unsigned char x, unsigned char y, char c)
{
    unsigned char row, col, bits;
    for (row=0;row<MICRO_HEIGHT;++row) {
        bits=micro_glyph_row(c,row);
        for (col=0;col<MICRO_WIDTH;++col)
            if (bits & (unsigned char)(16>>col)) plot(x+col,y+row);
    }
}

static void micro_text(unsigned char x, unsigned char y, const char *text)
{
    while ((*text!=0) && (x<=SAFE_RIGHT-MICRO_WIDTH)) {
        micro_char(x,y,*text++);
        x=(unsigned char)(x+MICRO_ADVANCE);
    }
}

static void micro_unsigned(unsigned char x, unsigned char y, unsigned long value)
{
    char digits[11];
    unsigned char count=0;
    if (value==0) { micro_char(x,y,'0'); return; }
    while ((value!=0) && (count<10)) { digits[count++]=(char)('0'+value%10); value/=10; }
    while (count>0) { micro_char(x,y,digits[--count]); x=(unsigned char)(x+MICRO_ADVANCE); }
}

static char diag_hex(unsigned char value)
{
    value&=15;
    return (char)(value<10?'0'+value:'A'+value-10);
}

static void diag_visible(unsigned char event_id, unsigned char context,
                         unsigned char attempt, unsigned char handle)
{
#if NCC_DIAG_FLIGHT_RECORDER
    char text[21];
    unsigned char x=(current_view==VIEW_DASHBOARD)?128:122;
    unsigned char y=(current_view==VIEW_DASHBOARD)?156:176;
    unsigned char row;
    text[0]='D'; text[1]=diag_hex((unsigned char)(ncc_diag_sequence>>12));
    text[2]=diag_hex((unsigned char)(ncc_diag_sequence>>8));
    text[3]=diag_hex((unsigned char)(ncc_diag_sequence>>4));
    text[4]=diag_hex((unsigned char)ncc_diag_sequence); text[5]=' ';
    text[6]=(context==DIAG_CTX_TIME)?'T':(context==DIAG_CTX_WEATHER?'W':'G');
    text[7]=' ';
    text[8]=diag_hex((unsigned char)(event_id>>4)); text[9]=diag_hex(event_id);
    text[10]=' '; text[11]='A'; text[12]=diag_hex((unsigned char)(attempt>>4));
    text[13]=diag_hex(attempt); text[14]=' '; text[15]='H';
    text[16]=diag_hex((unsigned char)(handle>>4)); text[17]=diag_hex(handle);
    text[18]=0;
    cc_color(CC_BACKGROUND);
    for(row=y;row<(unsigned char)(y+7);++row) undraw(x,row,SAFE_RIGHT,row);
    cc_color(event_id==DE_GUARD_FAIL?CC_ALERT:CC_WARNING); micro_text(x,y,text);
#else
    (void)event_id; (void)context; (void)attempt; (void)handle;
#endif
}

static void diag_check_guards(void)
{
    unsigned char i,bad=0;
    if(++ncc_diag_guard_divider!=0) return;
    for(i=0;i<DIAG_GUARD_SIZE;++i) {
        if(ncc_diag_pre_guard[i]!=(unsigned char)(0xA5+i)) bad|=1;
        if(ncc_diag_post_guard[i]!=(unsigned char)(0x5A-i)) bad|=2;
    }
    if(bad && ncc_diag_guard_status==0) {
        ncc_diag_guard_status=bad;
        diag_log(DE_GUARD_FAIL,DIAG_CTX_NONE,bad,0);
        diag_visible(DE_GUARD_FAIL,DIAG_CTX_NONE,0,bad);
    }
}

static unsigned char leap_year(unsigned int year)
{
    return (unsigned char)(((year%4)==0) && (((year%100)!=0) || ((year%400)==0)));
}

static void clock_format_utc(unsigned long utc)
{
    static const unsigned char month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned long days=utc/86400UL;
    unsigned long seconds=utc%86400UL;
    unsigned int year=1970;
    unsigned char month=0, day, hour, minute, second, dim;
    while(days>=(unsigned long)(leap_year(year)?366:365)) {
        days-=(unsigned long)(leap_year(year)?366:365); ++year;
    }
    while(month<11) {
        dim=month_days[month]; if(month==1 && leap_year(year)) ++dim;
        if(days<dim) break;
        days-=dim; ++month;
    }
    day=(unsigned char)(days+1);
    hour=(unsigned char)(seconds/3600UL);
    minute=(unsigned char)((seconds%3600UL)/60UL);
    second=(unsigned char)(seconds%60UL);
    last_text[0]=(char)('0'+((month+1)/10)); last_text[1]=(char)('0'+((month+1)%10));
    last_text[2]='-'; last_text[3]=(char)('0'+(day/10)); last_text[4]=(char)('0'+(day%10));
    last_text[5]=' '; last_text[6]=(char)('0'+(hour/10)); last_text[7]=(char)('0'+(hour%10));
    last_text[8]=':'; last_text[9]=(char)('0'+(minute/10)); last_text[10]=(char)('0'+(minute%10));
    last_text[11]=':'; last_text[12]=(char)('0'+(second/10)); last_text[13]=(char)('0'+(second%10));
    last_text[14]='Z'; last_text[15]=0;
}

static void clock_draw_value(void)
{
    const char *value=has_last_valid?last_text:"UNSYNC";
    unsigned char y;
    cc_color(CC_BACKGROUND);
    for(y=13;y<20;++y) undraw(86,y,174,y);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(86,13,value);
}

static void clock_frame_isr(void)
{
    ++clock_frame_counter;
}

static void service_clock(void)
{
    unsigned char now;
    unsigned char elapsed;
    unsigned char pending=0;
    unsigned char schedule_seconds;
    intrinsic_di(); now=clock_frame_counter; intrinsic_ei();
    elapsed=(unsigned char)(now-clock_frame_base);
    clock_frame_base=now;
    clock_frame_accumulator=(unsigned int)(clock_frame_accumulator+elapsed);
    while(clock_frame_accumulator>=CLOCK_FRAMES_PER_SECOND) {
        clock_frame_accumulator-=CLOCK_FRAMES_PER_SECOND;
        if(pending<255) ++pending;
    }
    clock_frame_phase=(unsigned char)clock_frame_accumulator;
    clock_seconds_pending=pending;
    schedule_seconds=pending;
    while(schedule_seconds--) {
        if(++auto_refresh_elapsed>=AUTO_REFRESH_SECONDS) {
            auto_refresh_elapsed=0;
            if(auto_refresh_pending<255) ++auto_refresh_pending;
        }
    }
    if(!has_last_valid || pending==0) return;
    last_utc+=(unsigned long)pending;
    clock_format_utc(last_utc);
    clock_draw_value();
}

static void weather_stage_draw(void)
{
    unsigned char y;
    if(current_view==VIEW_MODULE && selected==4) {
        cc_color(CC_BACKGROUND); for(y=137;y<145;++y) undraw(178,y,SAFE_RIGHT,y);
        cc_color(CC_SECONDARY_TEXT); micro_text(180,137,"WX STAGE");
        cc_color(CC_NOMINAL); micro_unsigned(234,137,weather_stage);
    } else if(current_view==VIEW_DASHBOARD) {
        cc_color(CC_BACKGROUND); for(y=154;y<163;++y) undraw(128,y,SAFE_RIGHT,y);
        cc_color(CC_SECONDARY_TEXT); micro_text(130,155,"WX STAGE");
        cc_color(CC_NOMINAL); micro_unsigned(184,155,weather_stage);
    }
}

static void weather_set_stage(unsigned char stage)
{
    weather_stage=stage;
    weather_stage_draw();
}

static void micro_hline(unsigned char x1, unsigned char x2, unsigned char y)
{
    draw(x1,y,x2,y);
}

static void micro_vline(unsigned char x, unsigned char y1, unsigned char y2)
{
    draw(x,y1,x,y2);
}

/* Public-domain Revolutionary-era marching air "Yankee Doodle", one complete
 * eight-bar phrase in C.  Thirty-two eighth notes at 120 BPM occupy 8.0 s. */
static const unsigned int splash_period[7]={427,381,339,320,285,254,227};
static const unsigned char splash_note[SPLASH_EVENT_COUNT]={
 0,0,1,2,0,2,1,1, 0,0,1,2,0,0,6,4,
 0,0,1,2,3,2,1,0, 6,4,5,6,0,0,0,0
};

static void splash_stop(void)
{
    set_psg(8,0); set_psg(9,0);
    set_psg(0,0); set_psg(1,0); set_psg(6,0);
    set_psg(7,(unsigned char)get_psg(7)|0x13);
}

static void splash_play(unsigned char event)
{
    unsigned char mixer;
    ay_note(splash_period[splash_note[event]]);
    mixer=(unsigned char)get_psg(7); set_psg(6,7);
    set_psg(7,(unsigned char)((mixer|0x02)&0xef));
    set_psg(9,(event&3)?8:11);
}

static void draw_splash(void)
{
    vdp_set_mode(mode_2); cc_color(CC_BACKGROUND); clg(); cc_color(CC_NOMINAL);
    micro_hline(3,252,3); micro_hline(3,252,188); micro_vline(3,3,188); micro_vline(252,3,188);
    micro_hline(6,249,6); micro_hline(6,249,185); micro_vline(6,6,185); micro_vline(249,6,185);
    micro_text(62,12,"NABU PERSONAL COMPUTER"); micro_text(68,20,"// INFORMATION SYSTEM");
    micro_hline(10,245,28); cc_color(CC_WARNING); micro_text(116,34,"NABU");
    cc_color(CC_PRIMARY_TEXT); micro_text(107,48,"COMMAND"); micro_text(110,58,"CENTER");
    cc_color(CC_NOMINAL); micro_hline(18,82,94); micro_hline(174,238,94);
    micro_hline(107,148,87); micro_hline(107,148,105); micro_vline(107,87,105); micro_vline(148,87,105);
    micro_hline(103,153,96); micro_vline(128,83,109); cc_color(CC_WARNING); micro_text(119,93,"NCC");
    cc_color(CC_NOMINAL); micro_hline(23,232,117); micro_hline(23,232,158); micro_vline(23,117,158); micro_vline(232,117,158);
    cc_color(CC_PRIMARY_TEXT); micro_text(95,122,"VERSION 1.0");
    cc_color(CC_NOMINAL); micro_text(41,134,"DEREK LEGER AKA (SUPER_DEREK)");
    cc_color(CC_WARNING); micro_text(95,146,"26 AUG 2026");
    cc_color(CC_NOMINAL); micro_hline(10,245,162); micro_text(59,164,"NATIVE SOFTWARE FOR THE");
    micro_text(65,172,"NABU PERSONAL COMPUTER");
    cc_color(CC_PRIMARY_TEXT); micro_text(89,181,"PRESS ANY KEY");
}

static void run_startup_splash(void)
{
    unsigned int elapsed=0;
    unsigned char last,now,event=0,note_frames=0,drum_frames=0;
    draw_splash(); intrinsic_di(); last=clock_frame_counter; intrinsic_ei();
    splash_play(0); event=1; note_frames=15; drum_frames=3;
    while(elapsed<SPLASH_TIMEOUT_FRAMES) {
        if(getk()!=0) { splash_stop(); while(getk()!=0) delay_loop(); return; }
        intrinsic_di(); now=clock_frame_counter; intrinsic_ei();
        while(last!=now) {
            ++last; ++elapsed;
            if(drum_frames && !--drum_frames) set_psg(9,0);
            if(event<SPLASH_EVENT_COUNT && !note_frames) {
                splash_play(event++); note_frames=15; drum_frames=3;
            }
            if(note_frames) --note_frames;
        }
        delay_loop();
    }
    splash_stop();
}

static void clear_pixel_band(unsigned char y1, unsigned char y2)
{
    while (y1<=y2) { undraw(SAFE_LEFT,y1,SAFE_RIGHT,y1); ++y1; }
}

static void clear_detail_plot(void)
{
    unsigned char y;
    for(y=41;y<=158;++y) undraw(SAFE_LEFT,y,DETAIL_RIGHT,y);
}

/* Clip a line to the protected viewport top; bottom/side inputs are bounded. */
static void view_line(int x1, int y1, int x2, int y2)
{
    int x;
    if(y1<VIEW_GRAPHICS_TOP && y2<VIEW_GRAPHICS_TOP) return;
    if(y1<VIEW_GRAPHICS_TOP) {
        x=x1+(x2-x1)*(VIEW_GRAPHICS_TOP-y1)/(y2-y1);
        x1=x; y1=VIEW_GRAPHICS_TOP;
    } else if(y2<VIEW_GRAPHICS_TOP) {
        x=x2+(x1-x2)*(VIEW_GRAPHICS_TOP-y2)/(y1-y2);
        x2=x; y2=VIEW_GRAPHICS_TOP;
    }
    draw(x1,y1,x2,y2);
}

static void draw_view_hud(const char *identifier)
{
    unsigned char y;
    for(y=VIEW_HUD_Y;y<(VIEW_HUD_Y+VIEW_HUD_H);++y) undraw(SAFE_LEFT,y,DETAIL_RIGHT,y);
    cc_color(CC_SECONDARY_TEXT); micro_text(SAFE_LEFT,VIEW_HUD_Y+1,identifier);
}

static void selection_corners(unsigned char index, unsigned char erase)
{
    unsigned char x=panel_x[index], y=panel_y[index], r=(unsigned char)(x+75), b=(unsigned char)(y+55);
    if (erase) {
        undraw(x,y,x+8,y); undraw(x,y,x,y+8); undraw(r-8,y,r,y); undraw(r,y,r,y+8);
        undraw(x,b-8,x,b); undraw(x,b,x+8,b); undraw(r-8,b,r,b); undraw(r,b-8,r,b);
    } else {
        draw(x,y,x+8,y); draw(x,y,x,y+8); draw(r-8,y,r,y); draw(r,y,r,y+8);
        draw(x,b-8,x,b); draw(x,b,x+8,b); draw(r-8,b,r,b); draw(r,b-8,r,b);
    }
}

static const char *profile_name(void)
{
    if(location_matches_selection()) return location_label;
    return "UNKNOWN";
}

static const char *profile_short(void)
{
    if(location_matches_selection()) return location_short;
    return "UNRES";
}

static void draw_music_header(void)
{
    unsigned char y;
    cc_color(CC_BACKGROUND);
    for(y=SAFE_TOP;y<SAFE_TOP+7;++y) undraw(145,y,SAFE_RIGHT,y);
    cc_color((music_enabled&&music_valid)?CC_NOMINAL:CC_WARNING);
    if(!music_enabled) micro_text(151,SAFE_TOP,"MUSIC STREAM OFF");
    else if(!music_valid) micro_text(151,SAFE_TOP,"MUSIC UNAVAILABLE");
    else micro_text(151,SAFE_TOP,music_source==2?"MUSIC ON CACHE":"MUSIC ON MUTOPIA");
}

static void draw_command_header(void)
{
    draw_stage(DRAW_STAGE_HEADER);
    cc_color(CC_PRIMARY_TEXT);
    micro_text(SAFE_LEFT,SAFE_TOP,"NABU COMMAND CENTER");
    draw_music_header();
    cc_color(CC_SECONDARY_TEXT);
    micro_text(SAFE_LEFT,13,"ATOMIC CLOCK ");
    micro_text(86,13,has_last_valid?last_text:"UNSYNC");
    cc_color(sound_enabled?CC_NOMINAL:CC_WARNING);
    micro_text(194,13,sound_enabled?"SOUND ON":"SOUND OFF");
    cc_color(CC_SECONDARY_TEXT); micro_text(SAFE_LEFT,21,"ZIP");
    cc_color(CC_NOMINAL); micro_text(32,21,zip_code);
    cc_color(CC_SECONDARY_TEXT); micro_text(68,21,profile_short());
    micro_text(132,21,console_global?"GLOBAL":"LOCAL"); micro_text(172,21,"NET");
    cc_color(cc_status_color()); micro_text(196,21,cc_status_short());
    cc_color(CC_SECONDARY_VECTOR);
    micro_hline(SAFE_LEFT,SAFE_RIGHT,HEADER_BOTTOM);
}

static void draw_detail_frame(const char *title)
{
    draw_stage(DRAW_STAGE_BEGIN); count_inc(&full_draw_count);
    vdp_set_mode(mode_2);
    cc_color(CC_PRIMARY_VECTOR);
    clg();
    draw_command_header();
    draw_stage(DRAW_STAGE_FRAME); count_inc(&static_draw_count);
    /* Full title owns x=8..174. Source/status is reserved to the right rail. */
    cc_color(CC_PRIMARY_TEXT); micro_text(SAFE_LEFT,31,title);
    cc_color(CC_SECONDARY_VECTOR); micro_hline(SAFE_LEFT,SAFE_RIGHT,39);
    micro_vline(175,41,158); micro_hline(SAFE_LEFT,SAFE_RIGHT,160);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(SAFE_LEFT,164,selected==5?"LEFT/RIGHT PAGE   ESC BACK":"LEFT/RIGHT TARGET ENTER LOCK");
    micro_text(SAFE_LEFT,176,BUILD_ID);
}

static void draw_telemetry_rail(const char *status, const char *metric1,
                                const char *metric2, unsigned char targets)
{
    const char *rail_severity=severity_text[selected];
    const char *rail_source="MOCK";
    if(selected==0) rail_source=has_quake?"USGS":"WAIT";
    else if(selected==1) {
        rail_source=has_space_weather?"SWPC":"WAIT";
        rail_severity=has_space_weather?(space_severity>=2?"STORM":(space_severity?"ELEV":"NOM")):"WAIT";
    } else if(selected==4) rail_source=weather_matches_selection()?"NWS":"WAIT";
    cc_color(CC_SECONDARY_TEXT); micro_text(180,43,"STATUS");
    cc_color(selected<2?CC_WARNING:CC_NOMINAL); micro_text(180,51,status);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,61,"SEV");
    cc_color(selected<2?CC_WARNING:CC_NOMINAL); micro_text(210,61,rail_severity);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,71,"SOURCE");
    cc_color(CC_SPECIAL);
    micro_text(180,79,rail_source);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,89,"PROFILE");
    micro_text(180,97,console_global?"GLOBAL":profile_short());
    micro_text(180,107,metric1); micro_text(180,117,metric2);
    micro_text(180,127,target_locked?"LOCK":"UNLOCK");
    if (targets) { micro_text(180,137,"TARGET"); micro_unsigned(222,137,(unsigned long)(selected_target+1)); micro_char(234,137,'/'); micro_unsigned(240,137,targets); }
    cc_color(cc_status_color()); micro_text(180,149,status_text());
}

static void clear_airspace_plot(void)
{
    unsigned char y;
    for(y=AIRSPACE_PLOT_TOP;y<=AIRSPACE_PLOT_BOTTOM;++y)
        undraw(AIRSPACE_PLOT_LEFT,y,AIRSPACE_PLOT_RIGHT,y);
}

static void draw_airspace_terrain(void)
{
    cc_color(CC_SECONDARY_VECTOR);
    /* Original five converging rails, expanded to the full safe lower surface. */
    draw(8,53,172,53);
    draw(90,53,10,157); draw(90,53,42,157); draw(90,53,90,157);
    draw(90,53,138,157); draw(90,53,170,157);
    draw(68,76,112,76);
    draw(38,110,142,110);
    draw(10,157,170,157);
}

static void draw_aircraft_marker(unsigned char index, int x, int y, int ground)
{
    int dx=0, dy=-6;
    unsigned int heading=(unsigned int)airspace_heading2[index]*2u;
    if(heading>=45 && heading<135) { dx=6; dy=0; }
    else if(heading>=135 && heading<225) { dx=0; dy=6; }
    else if(heading>=225 && heading<315) { dx=-6; dy=0; }
    cc_color(index==(selected_target%airspace_count)?CC_SELECTED:CC_PRIMARY_VECTOR);
    draw(x-4,y-3,x+4,y);
    draw(x+4,y,x-4,y+3);
    draw(x,y+3,x,ground);
    draw(x-2,ground,x+2,ground);
    draw(x,y,x+dx,y+dy);
    if (index==(selected_target%airspace_count)) {
        /* High-contrast selected-target corner brackets. */
        draw(x-8,y-8,x-3,y-8); draw(x-8,y-8,x-8,y-3);
        draw(x+3,y-8,x+8,y-8); draw(x+8,y-8,x+8,y-3);
        draw(x-8,y+3,x-8,y+8); draw(x-8,y+8,x-3,y+8);
        draw(x+3,y+8,x+8,y+8); draw(x+8,y+3,x+8,y+8); plot(x,y);
    }
}

static unsigned char airspace_labels_overlap(unsigned char x1, unsigned char y1,
                                             unsigned char x2, unsigned char y2)
{
    return (unsigned char)(!((unsigned int)x1+35u<x2 || (unsigned int)x2+35u<x1 ||
                             (unsigned int)y1+6u<y2 || (unsigned int)y2+6u<y1));
}

static void airspace_declutter_markers(unsigned char *plot_x, unsigned char *plot_y)
{
    static const signed char dx[9]={0,10,-10,0,0,10,-10,10,-10};
    static const signed char dy[9]={0,0,0,10,-10,8,8,-8,-8};
    unsigned char placed[AIRSPACE_MAX_AIRCRAFT], order, index, candidate, prior, accepted;
    int x, y, delta_x, delta_y;
    for(index=0;index<AIRSPACE_MAX_AIRCRAFT;++index) placed[index]=0;
    for(order=0;order<airspace_count;++order) {
        index=(unsigned char)((selected_target+order)%airspace_count);
        accepted=0;
        for(candidate=0;candidate<9 && !accepted;++candidate) {
            x=(int)airspace_x[index]+dx[candidate]; y=(int)airspace_y[index]+dy[candidate];
            if(x<18 || x>162 || y<68 || y>148) continue;
            accepted=1;
            for(prior=0;prior<AIRSPACE_MAX_AIRCRAFT;++prior) if(placed[prior]) {
                delta_x=x-(int)plot_x[prior]; if(delta_x<0) delta_x=-delta_x;
                delta_y=y-(int)plot_y[prior]; if(delta_y<0) delta_y=-delta_y;
                if(delta_x<18 && delta_y<14) accepted=0;
            }
            if(accepted) { plot_x[index]=(unsigned char)x; plot_y[index]=(unsigned char)y; placed[index]=1; }
        }
        if(!placed[index]) { plot_x[index]=airspace_x[index]; plot_y[index]=airspace_y[index]; placed[index]=1; }
    }
}

static void draw_airspace_labels(const unsigned char *plot_x, const unsigned char *plot_y)
{
    static const signed char dx[4]={7,-42,-17,-17};
    static const signed char dy[4]={-3,-3,-12,7};
    unsigned char label_x[AIRSPACE_MAX_AIRCRAFT], label_y[AIRSPACE_MAX_AIRCRAFT], visible[AIRSPACE_MAX_AIRCRAFT];
    unsigned char order, index, candidate, prior, accepted;
    int x, y;
    for(index=0;index<AIRSPACE_MAX_AIRCRAFT;++index) visible[index]=0;
    for(order=0;order<airspace_count;++order) {
        index=(unsigned char)((selected_target+order)%airspace_count);
        accepted=0;
        for(candidate=0;candidate<4 && !accepted;++candidate) {
            x=(int)plot_x[index]+dx[candidate]; y=(int)plot_y[index]+dy[candidate];
            if(x<AIRSPACE_MAP_LEFT+2 || x+35>AIRSPACE_MAP_RIGHT-2 || y<AIRSPACE_MAP_TOP+2 || y+6>AIRSPACE_MAP_BOTTOM-2) continue;
            accepted=1;
            for(prior=0;prior<AIRSPACE_MAX_AIRCRAFT;++prior)
                if(visible[prior] && airspace_labels_overlap((unsigned char)x,(unsigned char)y,label_x[prior],label_y[prior])) accepted=0;
            if(accepted) { label_x[index]=(unsigned char)x; label_y[index]=(unsigned char)y; visible[index]=1; }
        }
    }
    for(index=0;index<airspace_count;++index) if(visible[index]) {
        cc_color(index==(selected_target%airspace_count)?CC_SELECTED:CC_PRIMARY_TEXT);
        micro_text(label_x[index],label_y[index],airspace_callsign[index]);
    }
}

static void draw_adsb_plot(void)
{
    unsigned char i;
    unsigned char plot_x[AIRSPACE_MAX_AIRCRAFT], plot_y[AIRSPACE_MAX_AIRCRAFT];
    int x, y, ground;
    clear_airspace_plot();
    draw_airspace_terrain();
    draw_stage(DRAW_STAGE_DYNAMIC); count_inc(&dynamic_draw_count);
    for(i=0;i<airspace_count;++i) { plot_x[i]=airspace_x[i]; plot_y[i]=airspace_y[i]; }
    if(airspace_count) airspace_declutter_markers(plot_x,plot_y);
    for (i=0;i<airspace_count;++i) {
        x=plot_x[i]; y=plot_y[i];
        ground=y+12+(int)(airspace_alt100[i]/8);
        if(ground>152) ground=152;
        if(plot_x[i]!=airspace_x[i] || plot_y[i]!=airspace_y[i]) {
            cc_color(CC_SECONDARY_VECTOR); draw(airspace_x[i],airspace_y[i],plot_x[i],plot_y[i]);
        }
        draw_aircraft_marker(i,x,y,ground);
    }
    if(airspace_count) draw_airspace_labels(plot_x,plot_y);
    draw_view_hud("AIRSPACE VECTOR");
    draw_stage(DRAW_STAGE_TEXT);
    if(!airspace_count) { cc_color(CC_WARNING); micro_text(56,102,"NO AIRCRAFT"); }
}

static void draw_airspace_rail(void)
{
    unsigned char row;
    unsigned char slot=airspace_count?(unsigned char)(selected_target%airspace_count):0;
    for(row=43;row<=157;++row) undraw(180,row,247,row);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,43,"STATUS");
    cc_color(airspace_state==1?CC_NOMINAL:(airspace_state==2?CC_WARNING:CC_ALERT));
    micro_text(180,51,airspace_state==1?"LIVE":(airspace_state==2?"STALE":"OFFLINE"));
    cc_color(CC_SECONDARY_TEXT); micro_text(180,61,"SOURCE");
    cc_color(CC_SPECIAL); micro_text(180,69,airspace_source==1?"LOCAL":"ADSBLOL");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,81,"COUNT"); cc_color(CC_NOMINAL); micro_unsigned(216,81,airspace_count);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,93,"CALL"); cc_color(CC_NOMINAL); micro_text(210,93,airspace_count?airspace_callsign[slot]:"--");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,105,"ICAO"); cc_color(CC_NOMINAL); micro_text(210,105,airspace_count?airspace_icao[slot]:"--");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,117,"ALT"); cc_color(CC_NOMINAL); if(airspace_count) micro_unsigned(204,117,(unsigned long)airspace_alt100[slot]*100UL); else micro_text(204,117,"--");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,129,"SPD"); cc_color(CC_NOMINAL); if(airspace_count) micro_unsigned(204,129,(unsigned long)airspace_speed2[slot]*2UL); else micro_text(204,129,"--");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,141,"HDG"); cc_color(CC_NOMINAL); if(airspace_count) micro_unsigned(204,141,(unsigned long)airspace_heading2[slot]*2UL); else micro_text(204,141,"--");
}

static void draw_adsb_detail(void)
{
    draw_detail_frame("AIRSPACE / ADS-B");
    draw_adsb_plot();
    draw_airspace_rail();
}

static void draw_global_status(void)
{
    unsigned char y;
    cc_color(CC_BACKGROUND);
    for (y=12;y<=19;++y) undraw(136,y,SAFE_RIGHT,y);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(138,13,"ZIP");
    cc_color(CC_NOMINAL);
    micro_text(162,13,zip_code);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(198,13,"SND");
    cc_color(sound_enabled?CC_NOMINAL:CC_WARNING);
    micro_text(222,13,sound_enabled?"ON":"OFF");
}

static void draw_panel_border(unsigned char index, unsigned char highlighted)
{
    cc_color(highlighted?CC_SELECTED:CC_PRIMARY_VECTOR);
    selection_corners(index,(unsigned char)!highlighted);
}

static void draw_panel_art(unsigned char index)
{
    unsigned char x=panel_x[index], y=panel_y[index], i;
    char weather_metric[10];
    cc_color(CC_PRIMARY_TEXT);
    micro_text(x+3,y+3,panel_label[index]);
    cc_color(CC_SECONDARY_VECTOR);
    micro_hline(x+2,x+74,y+12);
    cc_color(CC_NOMINAL);
    if(index==0 && has_quake) {
        format_quake_text(quake_slot()); micro_text(x+3,y+46,quake_metric);
    } else if(index==1 && has_space_weather) {
        micro_text(x+3,y+46,space_metric);
    } else if(index==3 && has_airspace) {
        micro_text(x+3,y+46,airspace_state==1?"LIVE ":(airspace_state==2?"STALE ":"OFF "));
        micro_unsigned(x+39,y+46,airspace_count);
    } else if(index==4 && weather_matches_selection()) {
        weather_metric[0]=weather_temp[0]; weather_metric[1]=weather_temp[1]; weather_metric[2]=weather_temp[2];
        weather_metric[3]='F'; weather_metric[4]=' '; weather_metric[5]='W';
        weather_metric[6]=weather_wind[0]; weather_metric[7]=weather_wind[1]; weather_metric[8]=weather_wind[2];
        weather_metric[9]=0; micro_text(x+3,y+46,weather_metric);
    } else micro_text(x+3,y+46,panel_metric[index]);
    cc_color(CC_PRIMARY_VECTOR);

    if (index==0) {
        draw(x+6,y+30,x+20,y+22); draw(x+20,y+22,x+35,y+34); draw(x+35,y+34,x+51,y+20); draw(x+51,y+20,x+70,y+29);
        circle(x+24,y+29,2,1); draw(x+24,y+31,x+24,y+40);
    } else if (index==1) {
        draw(x+5,y+39,x+72,y+39); draw(x+5,y+34,x+18,y+31); draw(x+18,y+31,x+30,y+20);
        draw(x+30,y+20,x+43,y+30); draw(x+43,y+30,x+57,y+17); draw(x+57,y+17,x+72,y+24);
    } else if (index==2) {
        circle(x+38,y+29,16,1); circle(x+38,y+29,7,1); draw(x+52,y+20,x+61,y+25); draw(x+61,y+25,x+53,y+31);
    } else if (index==3) {
        draw(x+38,y+17,x+38,y+41); draw(x+7,y+41,x+38,y+17); draw(x+69,y+41,x+38,y+17);
        draw(x+15,y+34,x+61,y+34); draw(x+24,y+27,x+52,y+27);
        draw(x+28,y+24,x+33,y+27); draw(x+33,y+27,x+28,y+30);
    } else if (index==4) {
        draw(x+5,y+39,x+18,y+34); draw(x+18,y+34,x+31,y+37); draw(x+31,y+37,x+44,y+21);
        draw(x+44,y+21,x+57,y+27); draw(x+57,y+27,x+72,y+17); draw(x+65,y+39,x+65,y+20);
        draw(x+61,y+25,x+65,y+20); draw(x+69,y+25,x+65,y+20);
    } else {
        /* Actual shell-state task lanes. */
        for(i=0;i<4;++i) {
            draw(x+7,y+18+i*7,x+14,y+18+i*7);
            draw(x+18,y+18+i*7,x+50+(i&1)*12,y+18+i*7);
        }
        circle(x+64,y+29,7,1); draw(x+64,y+29,x+64,y+23);
    }
}

static void draw_mini_activity(unsigned char index)
{
    unsigned char x=panel_x[index], y=(unsigned char)(panel_y[index]+15);
    unsigned char old_x=(unsigned char)(x+5+((mini_phase+4+index*2)%10)*6);
    unsigned char new_x=(unsigned char)(x+5+((mini_phase+index*2)%10)*6);
    /* Reserved three-pixel activity lane between title rule and panel art. */
    undraw(old_x,y,old_x+3,y);
    cc_color(index==selected?CC_SELECTED:CC_NOMINAL);
    draw(new_x,y,new_x+3,y);
}

static void draw_news(void)
{
    clear_pixel_band(TICKER_TOP,TICKER_BOTTOM);
    cc_color(CC_SPECIAL);
    micro_text(SAFE_LEFT,TICKER_TOP,"NEWS/");
    cc_color(CC_SECONDARY_TEXT);
    micro_text(44,TICKER_TOP,news_title[news_index]);
    cc_color(CC_SECONDARY_VECTOR);
    micro_hline(SAFE_LEFT,SAFE_RIGHT,TICKER_BOTTOM);
}

static void draw_dashboard(void)
{
    unsigned char i;
    diag_log(DE_DRAW_DASH_BEGIN,DIAG_CTX_RENDER,selected,current_status);
    draw_stage(DRAW_STAGE_BEGIN); count_inc(&full_draw_count);
    vdp_set_mode(mode_2);
    cc_color(CC_PRIMARY_VECTOR);
    clg();
    draw_command_header();
    draw_news();
    draw_stage(DRAW_STAGE_FRAME); count_inc(&static_draw_count);
    cc_color(CC_SECONDARY_VECTOR);
    micro_vline(85,40,151); micro_vline(166,40,151); micro_hline(SAFE_LEFT,SAFE_RIGHT,95);
    for (i = 0; i < 6; ++i) draw_panel_art(i);
    draw_panel_border(selected,1);
    cc_color(CC_SECONDARY_VECTOR); micro_hline(SAFE_LEFT,SAFE_RIGHT,154);
    cc_color(cc_status_color());
    micro_text(SAFE_LEFT,156,status_text());
    cc_color(CC_SECONDARY_TEXT);
    micro_text(104,156,transient_status);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(SAFE_LEFT,168,"ARROWS SEL ENTER OPEN R REFRESH");
    micro_text(SAFE_LEFT,176,BUILD_ID);
    draw_stage(DRAW_STAGE_COMPLETE); dirty_mask=0;
    visual_dirty=0;
    diag_log(DE_DRAW_DASH_END,DIAG_CTX_RENDER,selected,current_status);
}

static void update_selection(unsigned char old_selection)
{
    count_inc(&dirty_draw_count); dirty_mask|=2;
    draw_panel_border(old_selection,0);
    draw_panel_art(old_selection);
    draw_panel_border(selected,1);
}

static void update_transport_region(void)
{
    if (current_view==VIEW_DASHBOARD) {
        count_inc(&dirty_draw_count); dirty_mask|=4;
        clear_pixel_band(155,163);
        cc_color(cc_status_color());
        micro_text(SAFE_LEFT,156,status_text());
        cc_color(CC_SECONDARY_TEXT);
        micro_text(104,156,last_checkpoint);
    }
}

static unsigned char split_fields(char *buffer, char **fields,
                                  unsigned char expected)
{
    unsigned char count = 0;
    unsigned int i = 0;
    fields[count++] = buffer;
    while (buffer[i] != 0) {
        if (buffer[i] == '|') {
            buffer[i] = 0;
            if (count >= expected) return 0;
            fields[count++] = &buffer[i + 1];
        }
        ++i;
    }
    return (unsigned char)(count == expected);
}

static unsigned char validate_record(const char *input, unsigned int input_length)
{
    char *fields[FIELD_COUNT];
    unsigned int i, separators = 0, body_length = 0;
    unsigned int calculated, received, logical_length;
    unsigned long sequence, utc, declared, value;
    if ((input_length == 0) || (input_length >= RECORD_CAPACITY)) {
        current_status = STATUS_INVALID; last_error = "EMPTY OR OVERSIZED RECORD"; return 0;
    }
    for (i = 0; i < input_length; ++i) parse_buffer[i] = input[i];
    parse_buffer[input_length] = 0;
    while ((input_length > 0) && ((parse_buffer[input_length-1] == '\n') ||
           (parse_buffer[input_length-1] == '\r'))) parse_buffer[--input_length] = 0;
    logical_length = input_length;
    for (i = 0; i < logical_length; ++i) if (parse_buffer[i] == '|') {
        ++separators; if (separators == 8) body_length = i + 1;
    }
    if ((separators != 9) || (body_length == 0)) {
        current_status = STATUS_INVALID; last_error = "FIELD OR END TRUNCATION"; return 0;
    }
    calculated = checksum16(parse_buffer, body_length);
    if (!split_fields(parse_buffer, fields, FIELD_COUNT)) {
        current_status = STATUS_INVALID; last_error = "FIELD COUNT INVALID"; return 0;
    }
    if (!text_equal(fields[0], "NCC")) { current_status=STATUS_INVALID; last_error="BAD MAGIC"; return 0; }
    if (!text_equal(fields[1], "1")) { current_status=STATUS_INVALID; last_error="UNSUPPORTED VERSION"; return 0; }
    if (!text_equal(fields[2], "TIME")) { current_status=STATUS_INVALID; last_error="BAD RECORD TYPE"; return 0; }
    if (!decimal_value(fields[3],999999UL,&sequence)) { current_status=STATUS_INVALID; last_error="SEQUENCE INVALID"; return 0; }
    if (!decimal_value(fields[4],4294967295UL,&utc)) { current_status=STATUS_INVALID; last_error="UTC INVALID"; return 0; }
    if (!decimal_value(fields[5],TEXT_CAPACITY,&declared)) { current_status=STATUS_INVALID; last_error="LENGTH INVALID"; return 0; }
    if (!decimal_value(fields[6],9999UL,&value)) { current_status=STATUS_INVALID; last_error="VALUE INVALID"; return 0; }
    if (text_length(fields[7]) != declared) { current_status=STATUS_INVALID; last_error="DECLARED LENGTH MISMATCH"; return 0; }
    if (!hex_value4(fields[8],&received)) { current_status=STATUS_INVALID; last_error="INTEGRITY FIELD INVALID"; return 0; }
    if (received != calculated) { current_status=STATUS_INVALID; last_error="INTEGRITY MISMATCH"; return 0; }
    if (!text_equal(fields[9],"END")) { current_status=STATUS_INVALID; last_error="END MARKER INVALID"; return 0; }
    if (has_last_valid && sequence < last_sequence) { current_status=STATUS_INVALID; last_error="SEQUENCE ROLLBACK"; return 0; }
    if (has_last_valid && sequence > last_sequence && utc < last_utc) { current_status=STATUS_STALE; last_error="STALE UTC ROLLBACK"; return 0; }
    current_status = (has_last_valid && sequence == last_sequence) ? STATUS_CACHED : STATUS_LIVE;
    last_sequence=sequence; last_utc=utc; last_value=(unsigned int)value;
    clock_format_utc(last_utc);
    intrinsic_di(); clock_frame_base=clock_frame_counter; clock_frame_accumulator=0; clock_frame_phase=0; clock_seconds_pending=0; intrinsic_ei();
    has_last_valid=1; last_error="NONE";
    return 1;
}

static unsigned int protected_file_read(unsigned char handle, unsigned char *buffer,
                                        unsigned int offset, unsigned long read_offset,
                                        unsigned int length)
{
    unsigned char saved_mask;
    unsigned int result;
    intrinsic_di();
    saved_mask=nabu_get_interrupts();
    nabu_disable_interrupt(0xff);
    nabu_enable_interrupt(NABU_INT_HCCA_RX);
    intrinsic_ei();
    result=rn_fileHandleRead(handle,buffer,offset,read_offset,length);
    intrinsic_di();
    nabu_disable_interrupt(0xff);
    nabu_enable_interrupt(saved_mask);
    intrinsic_ei();
    return result;
}

static unsigned char validate_music_record(const unsigned char *data)
{
    unsigned int i, calculated=0, received;
    if(data[0]!='M'||data[1]!='U'||data[2]!='0'||data[3]!='1'||data[4]!=1) return 0;
    if(data[5]==0||data[5]>MUSIC_MAX_EVENTS||(data[6]!=1)||
       (data[7]!=1&&data[7]!=2)) return 0;
    if(data[507]!='E'||data[508]!='N'||data[509]!='D'||data[510]!='!'||data[511]!=10) return 0;
    for(i=0;i<504;++i) calculated+=(unsigned int)data[i];
    received=(unsigned int)data[504]|((unsigned int)data[505]<<8);
    if(calculated!=received) return 0;
    for(i=0;i<data[5];++i) {
        unsigned int offset=(unsigned int)12U+i*MUSIC_EVENT_SIZE;
        unsigned int pb=(unsigned int)data[offset+1]|((unsigned int)data[offset+2]<<8);
        unsigned int pc=(unsigned int)data[offset+3]|((unsigned int)data[offset+4]<<8);
        if(data[offset]==0||pb>4095U||pc>4095U) return 0;
    }
    music_event_count=data[5]; music_source=data[7];
    music_position=0; music_wait=0; music_loop_pending=0; music_started=0;
    music_frame_mark=clock_frame_counter; return 1;
}

static void refresh_music(void)
{
    unsigned char handle, chunk;
    unsigned int received;
    if(music_valid) return; /* R refreshes status without restarting a valid track. */
    handle=rn_fileOpen(MUSIC_NAME_LENGTH,MUSIC_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    if(handle==0xff) return;
    for(chunk=0;chunk<8;++chunk) {
        received=protected_file_read(handle,music_buffer,(unsigned int)chunk*RECORD_LENGTH,
                                     (unsigned long)chunk*RECORD_LENGTH,RECORD_LENGTH);
        if(received!=RECORD_LENGTH) { rn_fileHandleClose(handle); return; }
    }
    rn_fileHandleClose(handle);
    music_valid=validate_music_record(music_buffer);
}

static void gate0_endurance(void)
{
    unsigned int i;
    unsigned char handle;
    char *name;
    unsigned char name_length;
    unsigned int received;
    ncc_gate0_read_count=0;
    ncc_gate0_short_read_count=0;
    ncc_gate0_open_fail_count=0;
    ncc_gate0_close_count=0;
    ncc_gate0_endurance_done=0;
    for(i=0;i<ncc_gate0_endurance_target;++i) {
        if(i&1) { name=(char *)WEATHER_NAME; name_length=WEATHER_NAME_LENGTH; }
        else { name=(char *)RESOURCE_NAME; name_length=RESOURCE_NAME_LENGTH; }
        handle=rn_fileOpen(name_length,name,OPEN_FILE_FLAG_READONLY,0xff);
        if(handle==0xff) { ++ncc_gate0_open_fail_count; continue; }
        received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
        ++ncc_gate0_read_count;
        if(received!=RECORD_LENGTH) ++ncc_gate0_short_read_count;
        rn_fileHandleClose(handle);
        ++ncc_gate0_close_count;
        service_clock();
        service_sound();
        service_scheduler();
        (void)getk();
        diag_check_guards();
    }
    ncc_gate0_endurance_done=1;
}

static void refresh_transport(void)
{
    unsigned char handle;
    ++ncc_diag_time_attempt;
    diag_log(DE_TIME_DISPATCH,DIAG_CTX_TIME,ncc_diag_time_attempt,0);
    last_checkpoint="T02 OPEN START"; update_transport_region();
    bytes_received=0;
    diag_log(DE_TIME_OPEN_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,0xff);
    diag_visible(DE_TIME_OPEN_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,0xff);
    handle=rn_fileOpen(RESOURCE_NAME_LENGTH,RESOURCE_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    diag_log(DE_TIME_OPEN_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    diag_visible(DE_TIME_OPEN_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    last_return_value=handle; last_checkpoint="T03 OPEN OK"; update_transport_region();
    last_checkpoint="T04 READ START"; update_transport_region();
    diag_log(DE_TIME_READ_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    diag_visible(DE_TIME_READ_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    bytes_received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    ++ncc_gate0_read_count;
    if(bytes_received!=RECORD_LENGTH) ++ncc_gate0_short_read_count;
    diag_log(DE_TIME_READ_END,DIAG_CTX_TIME,(unsigned char)bytes_received,(unsigned char)(bytes_received>>8));
    diag_visible(DE_TIME_READ_END,DIAG_CTX_TIME,(unsigned char)bytes_received,handle);
    if (bytes_received != RECORD_LENGTH) {
        last_checkpoint="T10 CLOSE START";
        diag_log(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        diag_visible(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        rn_fileHandleClose(handle);
        diag_log(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        diag_visible(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        last_checkpoint="T11 CLOSE OK";
        current_status=has_last_valid?STATUS_CACHED:STATUS_OFFLINE; last_error="READ NOT 64 BYTES"; update_transport_region();
        diag_log(DE_TIME_REFRESH_RETURN,DIAG_CTX_TIME,ncc_diag_time_attempt,current_status); return;
    }
    record_buffer[RECORD_LENGTH]=0; last_checkpoint="T05 READ OK";
    last_checkpoint="T06 PARSE START";
    diag_log(DE_TIME_PARSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,0);
    last_checkpoint="T08 VALIDATE START";
    if (!validate_record(record_buffer,bytes_received)) {
        diag_log(DE_TIME_PARSE_FAIL,DIAG_CTX_TIME,ncc_diag_time_attempt,current_status);
        last_checkpoint="T10 CLOSE START";
        diag_log(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        diag_visible(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        rn_fileHandleClose(handle);
        diag_log(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        diag_visible(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
        last_checkpoint="T11 CLOSE OK"; update_transport_region();
        diag_log(DE_TIME_REFRESH_RETURN,DIAG_CTX_TIME,ncc_diag_time_attempt,current_status); return;
    }
    last_checkpoint="T07 PARSE OK";
    diag_log(DE_TIME_PARSE_OK,DIAG_CTX_TIME,ncc_diag_time_attempt,current_status);
    diag_log(DE_TIME_CACHE_COMMIT,DIAG_CTX_TIME,(unsigned char)last_sequence,(unsigned char)last_utc);
    diag_log(DE_CLOCK_SYNC_COMMIT,DIAG_CTX_TIME,(unsigned char)last_utc,(unsigned char)(last_utc>>8));
    last_checkpoint="T09 VALIDATE OK";
    diag_log(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    diag_visible(DE_TIME_CLOSE_BEGIN,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    rn_fileHandleClose(handle);
    diag_log(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    diag_visible(DE_TIME_CLOSE_END,DIAG_CTX_TIME,ncc_diag_time_attempt,handle);
    last_checkpoint="T12 DISPLAY OK"; update_transport_region();
    diag_log(DE_TIME_REFRESH_RETURN,DIAG_CTX_TIME,ncc_diag_time_attempt,current_status);
}

static void append_weather_sample(int temp_f, unsigned long pressure_hpa,
                                  unsigned long wind_mph, unsigned long source_utc)
{
    unsigned char i;
    int temp_encoded=temp_f+100;
    long pressure_encoded=(long)pressure_hpa-900L;
    if(temp_encoded<0) temp_encoded=0; else if(temp_encoded>255) temp_encoded=255;
    if(pressure_encoded<0) pressure_encoded=0; else if(pressure_encoded>255) pressure_encoded=255;
    if(wind_mph>255UL) wind_mph=255UL;
    if(weather_history_count<12) ++weather_history_count;
    else {
        for(i=1;i<12;++i) {
            weather_history_temp[i-1]=weather_history_temp[i];
            weather_history_pressure[i-1]=weather_history_pressure[i];
            weather_history_wind[i-1]=weather_history_wind[i];
        }
    }
    i=(unsigned char)(weather_history_count-1);
    weather_history_temp[i]=(unsigned char)temp_encoded;
    weather_history_pressure[i]=(unsigned char)pressure_encoded;
    weather_history_wind[i]=(unsigned char)wind_mph;
    weather_history_utc=source_utc;
}

static unsigned char validate_location_record(const unsigned char *data, unsigned int length)
{
    unsigned char i, city_length, state_length, status, ch;
    unsigned int calculated=0, received;
    char city[17], state[3], token[7], selected_zip[6];
    if(length!=RECORD_LENGTH) return 0;
    if(data[0]!='L'||data[1]!='C'||data[2]!='0'||data[3]!='1'||data[63]!='\n') return 0;
    if(data[59]!='E'||data[60]!='N'||data[61]!='D'||data[62]!='!') return 0;
    for(i=0;i<57;++i) calculated=(unsigned int)(calculated+data[i]);
    calculated&=0xffffU;
    received=(unsigned int)data[57]|((unsigned int)data[58]<<8);
    if(received!=calculated) return 0;
    for(i=4;i<15;++i) if(data[i]<'0'||data[i]>'9') return 0;
    for(i=36;i<57;++i) if(data[i]!=0) return 0;
    status=data[15]; city_length=data[16]; state_length=data[17];
    if(status>1||city_length==0||city_length>16||state_length!=2) return 0;
    for(i=0;i<city_length;++i) {
        ch=data[18+i];
        if(!((ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch==' '||ch=='-')) return 0;
        city[i]=(char)ch;
    }
    city[city_length]=0;
    state[0]=(char)data[34]; state[1]=(char)data[35]; state[2]=0;
    if(status) {
        if(state[0]<'A'||state[0]>'Z'||state[1]<'A'||state[1]>'Z') return 0;
    } else if(!text_equal(city,"UNKNOWN")||state[0]!='-'||state[1]!='-') return 0;
    for(i=0;i<6;++i) token[i]=(char)data[4+i]; token[6]=0;
    for(i=0;i<5;++i) selected_zip[i]=(char)data[10+i]; selected_zip[5]=0;
    if(!text_equal(selected_zip,zip_code)||!zip_request_valid) return 0;
    for(i=0;i<6;++i) if(token[i]!=zip_request[4+i]) return 0;
    text_copy_bounded(location_token,token,sizeof(location_token));
    text_copy_bounded(location_zip,selected_zip,sizeof(location_zip));
    text_copy_bounded(location_city,city,sizeof(location_city));
    text_copy_bounded(location_state,state,sizeof(location_state));
    location_resolved=status; has_location=1; format_location_labels();
    return 1;
}

static void refresh_location(void)
{
    unsigned char handle;
    unsigned int count;
    handle=rn_fileOpen(LOCATION_NAME_LENGTH,LOCATION_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    count=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(count==RECORD_LENGTH) validate_location_record((unsigned char *)record_buffer,count);
}

static unsigned char validate_weather_record(const char *input, unsigned int input_length)
{
    char *fields[12];
    unsigned int i, separators=0, body_length=0, calculated, received;
    unsigned long token_value, zip_value, wind_value, pressure_value, utc_value, temp_value;
    unsigned char negative=0;
    weather_set_stage(WX_STAGE_PARSE);
    if(input_length!=RECORD_LENGTH){ weather_diag_set("WX READ"); current_status=STATUS_INVALID; last_error="WEATHER NOT 64 BYTES"; return 0; }
    for(i=0;i<input_length;++i) parse_buffer[i]=input[i];
    parse_buffer[input_length]=0;
    if(parse_buffer[63]!='\n'){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER LF MISSING"; return 0; }
    parse_buffer[63]=0;
    for(i=0;i<63;++i) if(parse_buffer[i]=='|'){ ++separators; if(separators==10) body_length=i+1; }
    if((separators!=11)||(body_length==0)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER FIELD COUNT"; return 0; }
    calculated=checksum16(parse_buffer,body_length);
    if(!split_fields(parse_buffer,fields,12)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER SPLIT"; return 0; }
    if(!text_equal(fields[0],"NCC")||!text_equal(fields[1],"1")||!text_equal(fields[2],"WX")){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER ENVELOPE"; return 0; }
    if((text_length(fields[3])!=6)||!decimal_value(fields[3],999999UL,&token_value)){ weather_diag_set("WX TOKEN"); current_status=STATUS_INVALID; last_error="WEATHER TOKEN"; return 0; }
    if((text_length(fields[4])!=5)||!decimal_value(fields[4],99999UL,&zip_value)){ weather_diag_set("WX ZIP"); current_status=STATUS_INVALID; last_error="WEATHER ZIP"; return 0; }
    if(!text_equal(fields[4],zip_code)){ weather_set_stage(WX_STAGE_ZIP_FAIL); weather_diag_set("WX ZIP"); current_status=STATUS_INVALID; last_error="WEATHER ZIP MISMATCH"; return 0; }
    if(!zip_request_valid){ weather_set_stage(WX_STAGE_TOKEN_FAIL); weather_diag_set("WX TOKEN"); current_status=STATUS_INVALID; last_error="NO CURRENT ZIP REQUEST"; return 0; }
    for(i=0;i<6;++i) if(fields[3][i]!=zip_request[4+i]){ weather_set_stage(WX_STAGE_TOKEN_FAIL); weather_diag_set("WX TOKEN"); current_status=STATUS_INVALID; last_error="WEATHER TOKEN MISMATCH"; return 0; }
    if(text_length(fields[5])!=3){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER TEMP"; return 0; }
    if(fields[5][0]=='-'){ negative=1; if(!decimal_value(&fields[5][1],99UL,&temp_value)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER TEMP"; return 0; } }
    else if(!decimal_value(fields[5],999UL,&temp_value)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER TEMP"; return 0; }
    if(text_length(fields[6])!=8){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER CONDITION"; return 0; }
    if((text_length(fields[7])!=3)||!decimal_value(fields[7],999UL,&wind_value)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER WIND"; return 0; }
    if((text_length(fields[8])!=4)||!decimal_value(fields[8],9999UL,&pressure_value)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER PRESSURE"; return 0; }
    if((text_length(fields[9])!=10)||!decimal_value(fields[9],4294967295UL,&utc_value)){ weather_diag_set("WX TYPE"); current_status=STATUS_INVALID; last_error="WEATHER UTC"; return 0; }
    if(!hex_value4(fields[10],&received)||received!=calculated||!text_equal(fields[11],"END")){ weather_set_stage(WX_STAGE_CRC_FAIL); weather_diag_set("WX CRC"); current_status=STATUS_INVALID; last_error="WEATHER INTEGRITY"; return 0; }
    weather_set_stage(WX_STAGE_ACCEPT);
    text_copy_bounded(weather_token,fields[3],sizeof(weather_token));
    text_copy_bounded(weather_zip,fields[4],sizeof(weather_zip));
    text_copy_bounded(weather_temp,fields[5],sizeof(weather_temp));
    text_copy_bounded(weather_condition,fields[6],sizeof(weather_condition));
    for(i=0;i<8;++i) if(weather_condition[i]=='_') weather_condition[i]=' ';
    text_copy_bounded(weather_wind,fields[7],sizeof(weather_wind));
    text_copy_bounded(weather_pressure,fields[8],sizeof(weather_pressure));
    weather_source_utc=utc_value;
    append_weather_sample(negative?-(int)temp_value:(int)temp_value,
                          pressure_value,wind_value,utc_value);
    has_weather=1; weather_set_stage(WX_STAGE_CACHE_COMMITTED); weather_diag_set("WX ACCEPT"); current_status=STATUS_LIVE; last_error="NONE";
    (void)negative; (void)token_value; (void)zip_value; (void)wind_value; (void)pressure_value;
    return 1;
}

static void refresh_weather(void)
{
    unsigned char handle;
    ++ncc_diag_weather_attempt;
    diag_log(DE_WX_DISPATCH,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,0);
    weather_diag_set("WX OPEN"); weather_set_stage(WX_STAGE_OPEN_START); last_checkpoint="T02 OPEN START"; update_transport_region(); bytes_received=0;
    diag_log(DE_WX_OPEN_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,0xff);
    diag_visible(DE_WX_OPEN_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,0xff);
    handle=rn_fileOpen(WEATHER_NAME_LENGTH,WEATHER_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    diag_log(DE_WX_OPEN_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    diag_visible(DE_WX_OPEN_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    weather_set_stage(WX_STAGE_OPEN_RETURNED); last_return_value=handle; last_checkpoint="T03 OPEN OK"; update_transport_region();
    weather_diag_set("WX READ"); weather_set_stage(WX_STAGE_READ_START); last_checkpoint="T04 READ START"; update_transport_region();
    diag_log(DE_WX_READ_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    diag_visible(DE_WX_READ_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    bytes_received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    ++ncc_gate0_read_count;
    if(bytes_received!=RECORD_LENGTH) ++ncc_gate0_short_read_count;
    diag_log(DE_WX_READ_END,DIAG_CTX_WEATHER,(unsigned char)bytes_received,(unsigned char)(bytes_received>>8));
    diag_visible(DE_WX_READ_END,DIAG_CTX_WEATHER,(unsigned char)bytes_received,handle);
    weather_set_stage(WX_STAGE_READ_RETURNED);
    if(bytes_received!=RECORD_LENGTH){
        diag_log(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        diag_visible(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        rn_fileHandleClose(handle);
        diag_log(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        diag_visible(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        current_status=has_weather?STATUS_CACHED:STATUS_OFFLINE; last_error="WEATHER READ NOT 64"; last_checkpoint="T11 CLOSE OK"; update_transport_region();
        diag_log(DE_WX_REFRESH_RETURN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,current_status); return;
    }
    record_buffer[RECORD_LENGTH]=0; last_checkpoint="T05 READ OK";
    diag_log(DE_WX_PARSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
    if(!validate_weather_record(record_buffer,bytes_received)){
        if(weather_stage==WX_STAGE_ZIP_FAIL) diag_log(DE_WX_ZIP_FAIL,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
        else if(weather_stage==WX_STAGE_TOKEN_FAIL) diag_log(DE_WX_TOKEN_FAIL,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
        else if(weather_stage==WX_STAGE_CRC_FAIL) diag_log(DE_WX_INTEGRITY_FAIL,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
        else diag_log(DE_WX_PARSE_FAIL,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
        diag_log(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        diag_visible(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        rn_fileHandleClose(handle);
        diag_log(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        diag_visible(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
        last_checkpoint="T11 CLOSE OK"; update_transport_region();
        diag_log(DE_WX_REFRESH_RETURN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,current_status); return;
    }
    diag_log(DE_WX_PARSE_OK,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
    diag_log(DE_WX_CACHE_COMMIT,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,weather_stage);
    diag_log(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    diag_visible(DE_WX_CLOSE_BEGIN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    rn_fileHandleClose(handle);
    diag_log(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    diag_visible(DE_WX_CLOSE_END,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,handle);
    last_checkpoint="T12 DISPLAY OK"; update_transport_region();
    diag_log(DE_WX_REFRESH_RETURN,DIAG_CTX_WEATHER,ncc_diag_weather_attempt,current_status);
}

static void refresh_earthquake(void)
{
    unsigned char handle;
    unsigned int received;
    handle=rn_fileOpen(EARTHQUAKE_NAME_LENGTH,EARTHQUAKE_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    if(handle==0xff) return;
    received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(received==RECORD_LENGTH) validate_quake_record((unsigned char *)record_buffer,received);
}

static void refresh_space_weather(void)
{
    unsigned char handle; unsigned int received;
    handle=rn_fileOpen(SPACE_WEATHER_NAME_LENGTH,SPACE_WEATHER_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    if(handle==0xff) return;
    received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(received==RECORD_LENGTH) validate_space_record((unsigned char *)record_buffer,received);
}

static unsigned int satellite_u16le(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned long satellite_u32le(const unsigned char *p)
{
    return (unsigned long)p[0] |
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

static int satellite_s16le(const unsigned char *p)
{
    unsigned int raw = satellite_u16le(p);
    if (raw & 0x8000u) return -(int)(65536UL - raw);
    return (int)raw;
}

static void satellite_copy_fixed(char *dst, const unsigned char *src, unsigned char length)
{
    unsigned char i;
    for (i = 0; i < length; ++i) dst[i] = (char)src[i];
    while (length && dst[length - 1] == ' ') --length;
    dst[length] = 0;
}

static char *satellite_append_uint(char *p, unsigned long value)
{
    char reverse[11];
    unsigned char count = 0;
    do {
        reverse[count++] = (char)('0' + (value % 10UL));
        value /= 10UL;
    } while (value && count < sizeof(reverse));
    while (count) *p++ = reverse[--count];
    *p = 0;
    return p;
}

static char *satellite_append_signed100(char *p, int value)
{
    unsigned int magnitude;
    if (value < 0) {
        *p++ = '-';
        magnitude = (unsigned int)(-value);
    } else {
        *p++ = '+';
        magnitude = (unsigned int)value;
    }
    p = satellite_append_uint(p, magnitude / 100u);
    *p++ = '.';
    *p++ = (char)('0' + ((magnitude / 10u) % 10u));
    *p++ = (char)('0' + (magnitude % 10u));
    *p = 0;
    return p;
}

static void satellite_format_text(void)
{
    char *p;
    p = satellite_position_text;
    *p++='L'; *p++='A'; *p++='T';
    p = satellite_append_signed100(p, satellite_latitude100);
    *p++=' '; *p++='L'; *p++='O'; *p++='N';
    satellite_append_signed100(p, satellite_longitude100);

    p = satellite_motion_text;
    *p++='A'; *p++='L'; *p++='T';
    p = satellite_append_uint(p, satellite_altitude_km);
    *p++=' '; *p++='V';
    satellite_append_uint(p, satellite_velocity_kmh);

    if (satellite_visibility == 1) satellite_status_text[0]='D', satellite_status_text[1]='A', satellite_status_text[2]='Y', satellite_status_text[3]=0;
    else if (satellite_visibility == 2) satellite_status_text[0]='E', satellite_status_text[1]='C', satellite_status_text[2]='L', satellite_status_text[3]=0;
    else satellite_status_text[0]='U', satellite_status_text[1]='N', satellite_status_text[2]='K', satellite_status_text[3]=0;
}

static unsigned char validate_satellite_record(const unsigned char *data)
{
    unsigned char i;
    unsigned int checksum = 0;
    if (data[0]!='S' || data[1]!='A' || data[2]!='0' || data[3]!='1') return 0;
    if (data[5] != 1 || data[62] != 'E' || data[63] != '\n') return 0;
    for (i = 0; i < 60; ++i) checksum = (unsigned int)(checksum + data[i]);
    if (checksum != satellite_u16le(data + 60)) return 0;
    satellite_sequence = data[4];
    satellite_latitude100 = satellite_s16le(data + 6);
    satellite_longitude100 = satellite_s16le(data + 8);
    satellite_altitude_km = satellite_u16le(data + 10);
    satellite_velocity_kmh = (unsigned long)satellite_u16le(data + 12) * 10UL;
    satellite_timestamp = satellite_u32le(data + 14);
    satellite_copy_fixed(satellite_identity, data + 18, 5);
    satellite_copy_fixed(satellite_norad, data + 23, 5);
    satellite_visibility = data[28];
    satellite_footprint_km = satellite_u16le(data + 29);
    has_satellite = 1;
    satellite_format_text();
    return 1;
}

static unsigned char validate_airspace_record(const unsigned char *data)
{
    unsigned char i, j, count, state, source, offset;
    unsigned int checksum=0;
    char callsign[AIRSPACE_MAX_AIRCRAFT][7], icao[AIRSPACE_MAX_AIRCRAFT][7];
    unsigned char xpos[AIRSPACE_MAX_AIRCRAFT], ypos[AIRSPACE_MAX_AIRCRAFT];
    unsigned char altitude[AIRSPACE_MAX_AIRCRAFT], speed[AIRSPACE_MAX_AIRCRAFT], heading[AIRSPACE_MAX_AIRCRAFT];
    if(data[0]!='A' || data[1]!='S' || data[2]!='0' || data[3]!='1') return 0;
    state=data[5]; source=data[6]; count=data[7];
    if(state<1 || state>3 || source<1 || source>2 || count>AIRSPACE_MAX_AIRCRAFT) return 0;
    if(data[62]!='E' || data[63]!='\n') return 0;
    for(i=0;i<60;++i) checksum=(unsigned int)(checksum+data[i]);
    if(checksum!=satellite_u16le(data+60)) return 0;
    for(i=0;i<count;++i) {
        offset=(unsigned char)(8+i*AIRSPACE_SLOT_SIZE);
        for(j=0;j<12;++j) if(data[offset+j]!=' ' && !((data[offset+j]>='A' && data[offset+j]<='Z') || (data[offset+j]>='0' && data[offset+j]<='9'))) return 0;
        if(data[offset+12]<14 || data[offset+12]>164 || data[offset+13]<48 || data[offset+13]>145 || data[offset+16]>179) return 0;
        satellite_copy_fixed(callsign[i],data+offset,6);
        satellite_copy_fixed(icao[i],data+offset+6,6);
        if(!callsign[i][0] || !icao[i][0]) return 0;
        xpos[i]=data[offset+12]; ypos[i]=data[offset+13]; altitude[i]=data[offset+14]; speed[i]=data[offset+15]; heading[i]=data[offset+16];
    }
    airspace_sequence=data[4]; airspace_state=state; airspace_source=source; airspace_count=count;
    for(i=0;i<count;++i) {
        for(j=0;j<7;++j) { airspace_callsign[i][j]=callsign[i][j]; airspace_icao[i][j]=icao[i][j]; }
        airspace_x[i]=xpos[i]; airspace_y[i]=ypos[i]; airspace_alt100[i]=altitude[i]; airspace_speed2[i]=speed[i]; airspace_heading2[i]=heading[i];
    }
    has_airspace=1;
    if(!count) selected_target=0; else if(selected_target>=count) selected_target=0;
    return 1;
}

static void refresh_satellite(void)
{
    unsigned char handle;
    unsigned int received;
    handle = rn_fileOpen(SATELLITE_NAME_LENGTH, SATELLITE_NAME, OPEN_FILE_FLAG_READONLY, 0xff);
    if (handle == 0xff) return;
    received = protected_file_read(handle, (unsigned char *)record_buffer, 0, 0, RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if (received == RECORD_LENGTH) validate_satellite_record((const unsigned char *)record_buffer);
}

static void refresh_airspace(void)
{
    unsigned char handle;
    unsigned int received;
    handle=rn_fileOpen(AIRSPACE_NAME_LENGTH,AIRSPACE_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    if(handle==0xff) return;
    received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(received==RECORD_LENGTH) validate_airspace_record((const unsigned char *)record_buffer);
}

static void refresh_weather_alert(void)
{
    unsigned char handle; unsigned int received;
    handle=rn_fileOpen(WEATHER_ALERT_NAME_LENGTH,WEATHER_ALERT_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    if(handle==0xff) return;
    received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(received==RECORD_LENGTH) validate_weather_alert_record((unsigned char *)record_buffer,received);
}

static void request_manual_refresh(void)
{
    if(manual_refresh_pending<255) ++manual_refresh_pending;
}

static unsigned char service_global_refresh(void)
{
    if(refresh_in_progress) return 0;
    if(manual_refresh_pending) --manual_refresh_pending;
    else if(auto_refresh_pending) --auto_refresh_pending;
    else return 0;
    refresh_in_progress=1;
    weather_diag_dispatch();
    weather_set_stage(WX_STAGE_R_ENTERED);
    stop_cue();
    refresh_transport();
    refresh_weather();
    refresh_location();
    refresh_earthquake();
    refresh_space_weather();
    refresh_weather_alert();
    refresh_satellite();
    refresh_airspace();
    refresh_music();
    refresh_in_progress=0;
    return 1;
}

static unsigned char validate_weather_history(const unsigned char *data, unsigned int length)
{
    unsigned char i, count;
    unsigned int calculated=0, received;
    if(length!=RECORD_LENGTH || data[0]!='W' || data[1]!='X' || data[2]!='H' || data[3]!='1') return 0;
    for(i=0;i<6;++i) if(data[4+i]!=(unsigned char)weather_token[i]) return 0;
    for(i=0;i<5;++i) if(data[10+i]!=(unsigned char)weather_zip[i]) return 0;
    count=data[15]; if(count==0 || count>12 || data[63]!=10) return 0;
    if(data[59]!='E'||data[60]!='N'||data[61]!='D'||data[62]!='!') return 0;
    for(i=0;i<57;++i) calculated+=(unsigned int)data[i];
    received=(unsigned int)data[57]|((unsigned int)data[58]<<8);
    if(calculated!=received) return 0;
    weather_history_count=count;
    weather_history_utc=(unsigned long)data[17]|((unsigned long)data[18]<<8)|((unsigned long)data[19]<<16)|((unsigned long)data[20]<<24);
    for(i=0;i<count;++i){
        weather_history_temp[i]=data[21+i*3];
        weather_history_pressure[i]=data[22+i*3];
        weather_history_wind[i]=data[23+i*3];
    }
    return 1;
}

static void refresh_weather_history(void)
{
    unsigned char handle;
    unsigned int received;
    if(!has_weather) return;
    handle=rn_fileOpen(WEATHER_HISTORY_NAME_LENGTH,WEATHER_HISTORY_NAME,OPEN_FILE_FLAG_READONLY,0xff);
    received=protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH);
    rn_fileHandleClose(handle);
    if(received==RECORD_LENGTH) validate_weather_history((unsigned char *)record_buffer,received);
}

/* Shared bounded pseudo-3D globe. All coordinates are small integer/fixed-point. */
static void globe_point(unsigned char longitude, signed char latitude,
                        unsigned char radius, int *sx, int *sy, int *depth)
{
    unsigned char a=(unsigned char)((longitude+globe_phase)&15);
    int lat_scale=64-((latitude<0?-latitude:latitude)*3);
    int x=((int)circle_x16[a]*lat_scale)/64;
    int z=((int)circle_y16[a]*lat_scale)/64;
    *sx=90+(x*radius)/64;
    *sy=98-((int)latitude*radius)/64-(z*radius)/256;
    *depth=z;
}

static void draw_globe(void)
{
    signed char lat;
    unsigned char i;
    int x1,y1,z1,x2,y2,z2;
    count_inc(&frame3d_count); count_inc(&dynamic_draw_count); draw_stage(DRAW_STAGE_DYNAMIC);
    cc_color(CC_PRIMARY_VECTOR); circle(90,98,43,1);
    /* latitude rings */
    for(lat=-28;lat<=28;lat+=28) {
        globe_point(0,lat,42,&x1,&y1,&z1);
        for(i=0;i<16;++i) {
            globe_point((unsigned char)((i+1)&15),lat,42,&x2,&y2,&z2);
            cc_color((z1+z2)>=0?CC_PRIMARY_VECTOR:CC_SECONDARY_VECTOR);
            draw(x1,y1,x2,y2);
            x1=x2; y1=y2; z1=z2;
        }
    }
    /* longitude arcs */
    for(i=0;i<16;i+=4) {
        signed char p;
        globe_point(i,-56,42,&x1,&y1,&z1);
        for(p=-48;p<=56;p+=8) {
            globe_point(i,p,42,&x2,&y2,&z2);
            cc_color((z1+z2)>=0?CC_PRIMARY_VECTOR:CC_SECONDARY_VECTOR);
            draw(x1,y1,x2,y2); x1=x2; y1=y2; z1=z2;
        }
    }
}

static void draw_globe_marker(unsigned char longitude, signed char latitude,
                              unsigned char highlighted)
{
    int x,y,z;
    globe_point(longitude,latitude,42,&x,&y,&z);
    cc_color(highlighted?CC_SELECTED:(z>=0?CC_WARNING:CC_SECONDARY_VECTOR));
    circle(x,y,highlighted?5:2,1);
    if(highlighted) plot(x,y);
}

static void draw_quake_global_plot(void)
{
    unsigned char i, slot, count=quake_global_count;
    draw_globe();
    for(i=0;i<count;++i) {
        int latitude, longitude; slot=(unsigned char)(quake_local_count+i); latitude=quake_lat100[slot]/100; longitude=quake_lon100[slot];
        if(latitude>56) latitude=56; if(latitude<-56) latitude=-56;
        draw_globe_marker((unsigned char)(((long)(longitude+18000)*16L)/36000L),(signed char)latitude,i==(selected_target%count));
    }
    if(count) { slot=quake_slot(); cc_color(CC_SELECTED); micro_text(10,150,quake_region[slot]); }
    else { cc_color(CC_WARNING); micro_text(10,150,"NO GLOBAL EVENTS"); }
    draw_view_hud("SEISMIC GLOBAL");
}

static void draw_quake_local_plot(void)
{
    unsigned char i, slot=quake_slot();
    int marker_x=90, stem_bottom=126;
    cc_color(CC_SECONDARY_VECTOR);
    /* One coherent perspective surface. */
    draw_stage(DRAW_STAGE_STATIC); count_inc(&static_draw_count);
    view_line(10,80,90,58); view_line(90,58,170,80); view_line(10,80,10,128); view_line(170,80,170,128);
    draw(10,128,90,151); draw(170,128,90,151);
    for(i=0;i<5;++i) { view_line(90,58,18+i*34,128); draw(10,80+i*10,170,80+i*10); }
    /* Selected surface epicenter, depth stem, and clearly hanging hypocenter ball. */
    draw_stage(DRAW_STAGE_DYNAMIC); count_inc(&dynamic_draw_count);
    if(quake_local_count) {
        marker_x=20+(int)(((long)(quake_lon100[slot]+18000)*140L)/36000L); stem_bottom=96+(quake_depth[slot]>60?30:quake_depth[slot]/2);
        cc_color(CC_ALERT); circle(marker_x,83,5+(globe_phase&1),1); draw(marker_x-6,83,marker_x+6,83); draw(marker_x,77,marker_x,89);
        cc_color(CC_SELECTED); draw(marker_x,89,marker_x,stem_bottom); circle(marker_x,stem_bottom+7,7,1); circle(marker_x,stem_bottom+7,3,1);
        cc_color(CC_SECONDARY_TEXT); micro_text(105,126,"HYPOCENTER"); cc_color(CC_SELECTED); micro_text(10,150,quake_region[slot]);
    } else { cc_color(CC_WARNING); micro_text(10,150,"NO LOCAL EVENTS"); }
    draw_view_hud("SEISMIC LOCAL");
}

static void draw_quake_detail(void)
{
    unsigned char count=quake_count(), slot=quake_slot();
    const char *severity="NO DATA";
    draw_detail_frame(console_global?"EARTHQUAKE / GLOBAL":"EARTHQUAKE / LOCAL TERRAIN");
    if(console_global) draw_quake_global_plot(); else draw_quake_local_plot();
    if(count) { format_quake_text(slot); severity=quake_mag10[slot]>=60?"ALERT":(quake_mag10[slot]>=40?"ADVISORY":"NOMINAL"); }
    draw_telemetry_rail(severity,count?quake_mag_text:"MAG---",count?quake_depth_text:"DEPTH---",count);
}

static void draw_space_plot(void)
{
    unsigned char i, x1, x2, y1, y2; unsigned int speed;
    cc_color(CC_ALERT); micro_hline(10,170,97);
    if(has_space_weather && space_history_count>1) {
        cc_color(CC_NOMINAL);
        for(i=0;i+1<space_history_count;++i) {
            x1=(unsigned char)(12+i*156/(space_history_count-1)); x2=(unsigned char)(12+(i+1)*156/(space_history_count-1));
            y1=(unsigned char)(132-(unsigned int)space_kp_history[i]*7/10); y2=(unsigned char)(132-(unsigned int)space_kp_history[i+1]*7/10); draw(x1,y1,x2,y2);
        }
        cc_color(CC_WARNING);
        for(i=0;i+1<space_history_count;++i) {
            speed=space_speed_history[i]; if(speed<250U) speed=250U; if(speed>800U) speed=800U; y1=(unsigned char)(134-(speed-250U)*60U/550U);
            speed=space_speed_history[i+1]; if(speed<250U) speed=250U; if(speed>800U) speed=800U; y2=(unsigned char)(134-(speed-250U)*60U/550U);
            x1=(unsigned char)(12+i*156/(space_history_count-1)); x2=(unsigned char)(12+(i+1)*156/(space_history_count-1)); draw(x1,y1,x2,y2);
        }
    }
    cc_color(CC_NOMINAL); micro_text(10,44,"KP 0-9 GREEN");
    cc_color(CC_SECONDARY_TEXT); micro_text(100,44,"NOW");
    cc_color(CC_NOMINAL); micro_text(124,44,has_space_weather?space_kp_text:"KP---");
    cc_color(CC_WARNING); micro_text(10,53,"WIND KM/S YELLOW");
    cc_color(CC_SECONDARY_TEXT); micro_text(112,53,"NOW");
    cc_color(CC_WARNING); if(has_space_weather) micro_unsigned(136,53,space_speed); else micro_text(136,53,"---");
    cc_color(CC_SECONDARY_TEXT); micro_text(10,141,"OLDER -> NEWEST");
    cc_color(CC_ALERT); micro_text(10,150,"RED = KP5 STORM");
}

static void draw_space_detail(void)
{
    draw_detail_frame("SPACE WEATHER / TRENDS");
    draw_space_plot();
    draw_telemetry_rail(has_space_weather?(space_severity>=2?"STORM":(space_severity?"ELEVATED":"NOMINAL")):"NO DATA",has_space_weather?space_kp_text:"KP---",has_space_weather?space_speed_text:"SW----",0);
}

static void draw_satellite_plot(void)
{
 int x1,y1,z1,x2,y2,z2,i;
    draw_globe();
    /* Two projected orbit planes with front/back depth colors. */
    globe_point(0,(signed char)(circle_y16[2]/3),55,&x1,&y1,&z1);
    for(i=0;i<16;++i) {
        globe_point((unsigned char)((i+1)&15),(signed char)(circle_y16[(i+3)&15]/3),55,&x2,&y2,&z2);
        cc_color((z1+z2)>=0?CC_NOMINAL:CC_SECONDARY_VECTOR); draw(x1,y1,x2,y2);
        x1=x2; y1=y2; z1=z2;
    }
 if(has_satellite) {
  draw_globe_marker((unsigned char)((satellite_longitude100 + 18000) / 2250),
                    (signed char)(satellite_latitude100 / 100),1);
 }
 draw_view_hud("ORBITAL VECTOR");
}

static void draw_satellite_rail(void)
{
    char latitude[12]="LAT ", longitude[12]="LON ";
    char altitude[12]="ALT ", velocity[12]="VEL ";
    char *p;
    unsigned char y;
    for(y=43;y<158;++y) undraw(180,y,SAFE_RIGHT,y);
    p=latitude+4; satellite_append_signed100(p,satellite_latitude100);
    p=longitude+4; satellite_append_signed100(p,satellite_longitude100);
    p=altitude+4; satellite_append_uint(p,satellite_altitude_km);
    p=velocity+4; satellite_append_uint(p,satellite_velocity_kmh);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(180,43,has_satellite?"STATUS LIVE":"STATUS WAIT");
    micro_text(180,55,has_satellite?"SOURCE LIVE":"SOURCE WAIT");
    micro_text(180,67,"TARGET "); cc_color(CC_NOMINAL); micro_text(222,67,has_satellite?satellite_identity:"WAIT");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,79,"NORAD "); cc_color(CC_NOMINAL); micro_text(216,79,has_satellite?satellite_norad:"-----");
    cc_color(CC_NOMINAL); micro_text(180,91,has_satellite?latitude:"LAT --");
    micro_text(180,103,has_satellite?longitude:"LON --");
    micro_text(180,115,has_satellite?altitude:"ALT --");
    micro_text(180,127,has_satellite?velocity:"VEL --");
    cc_color(CC_SECONDARY_TEXT); micro_text(180,139,target_locked?"LOCK 1/1":"UNLOCK 1/1");
    micro_text(180,151,"VIS "); cc_color(CC_NOMINAL); micro_text(204,151,has_satellite?satellite_status_text:"WAIT");
}

static void draw_satellite_detail(void)
{
    draw_detail_frame("SATELLITE / ISS 3D");
    draw_satellite_plot();
    draw_satellite_rail();
}

static void draw_weather_plot(void)
{
    unsigned char i, x, y1, y2, y3;
    int scaled;
    cc_color(CC_PRIMARY_VECTOR);
    draw(10,54,164,54); draw(10,80,164,80); draw(10,106,164,106); draw(10,134,164,134);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(12,43,"TEMP F"); micro_text(70,43,"PRESS DEV"); micro_text(132,43,"WIND");
    if(weather_matches_selection()) {
        if(weather_history_count>1){
            for(i=1;i<weather_history_count;++i){
                x=(unsigned char)(14+i*12);
                scaled=76-(((int)weather_history_temp[i]-80)/4); if(scaled<52) scaled=52; if(scaled>77) scaled=77; y1=(unsigned char)scaled;
                scaled=105-(((int)weather_history_pressure[i]-110)/3); if(scaled<80) scaled=80; if(scaled>105) scaled=105; y2=(unsigned char)scaled;
                scaled=76-(((int)weather_history_temp[i-1]-80)/4); if(scaled<52) scaled=52; if(scaled>77) scaled=77;
                cc_color(CC_PRIMARY_VECTOR); draw((unsigned char)(x-12),(unsigned char)scaled,x,y1);
                scaled=105-(((int)weather_history_pressure[i-1]-110)/3); if(scaled<80) scaled=80; if(scaled>105) scaled=105;
                cc_color(CC_SPECIAL); draw((unsigned char)(x-12),(unsigned char)scaled,x,y2);
                y3=(unsigned char)(132-(weather_history_wind[i]>20?20:weather_history_wind[i]));
                cc_color(CC_SELECTED); draw(x,132,x,y3);
            }
        }
        cc_color(CC_NOMINAL); micro_text(12,142,weather_temp); micro_char(30,142,'F');
        micro_text(42,142,weather_condition); micro_text(94,142,"W"); micro_text(100,142,weather_wind);
        micro_text(124,142,"P"); micro_text(130,142,weather_pressure);
        cc_color(CC_SECONDARY_TEXT); micro_text(12,150,"H"); cc_color(CC_NOMINAL); micro_unsigned(18,150,weather_history_count);
        cc_color(CC_SECONDARY_TEXT); micro_text(42,150,"AGE"); cc_color(CC_NOMINAL);
        micro_unsigned(66,150,(has_last_valid&&last_utc>weather_source_utc)?last_utc-weather_source_utc:0); micro_text(96,150,"S");
    } else {
        cc_color(CC_WARNING); micro_text(52,84,"AWAITING MATCHED DATA");
        cc_color(CC_SECONDARY_TEXT); micro_text(12,142,"NO TREND DATA PRESENT");
    }
}

static void draw_weather_alert_summary(void)
{
    unsigned char i, shown;
    for(i=150;i<157;++i) undraw(10,i,164,i);
    cc_color(weather_alert_severity>=3?CC_ALERT:(weather_alert_severity>=2?CC_WARNING:CC_SECONDARY_TEXT));
    micro_text(12,150,"NWS");
    cc_color(CC_NOMINAL);
    if(!has_weather_alert) { micro_text(36,150,"ALERTS WAIT"); return; }
    shown=weather_alert_length;
    if(shown>20) shown=20;
    for(i=0;i<shown;++i)
        micro_char((unsigned char)(36+i*6),150,weather_alert_text[i]);
}

static void draw_weather_detail(void)
{
    char metric1[13], metric2[13], title[28];
    const char *place=profile_name();
    unsigned char i=0,j=0;
    diag_log(DE_DRAW_WX_BEGIN,DIAG_CTX_RENDER,selected,has_weather);
    text_copy_bounded(title,"WEATHER ",sizeof(title));
    j=8; while(i<19 && j<sizeof(title)-1 && place[i]) title[j++]=place[i++]; title[j]=0;
    draw_detail_frame(title);
    draw_weather_plot();
    draw_weather_alert_summary();
    if(weather_matches_selection()){
        i=0; j=0;
        while(i<3 && j<sizeof(metric1)-1 && weather_temp[i]) metric1[j++]=weather_temp[i++];
        if(j<sizeof(metric1)-1) metric1[j++]='F';
        if(j<sizeof(metric1)-1) metric1[j++]=' ';
        i=0;
        while(i<8 && j<sizeof(metric1)-1 && weather_condition[i]) metric1[j++]=weather_condition[i++];
        metric1[j]=0;
        metric2[0]='W'; metric2[1]=weather_wind[0]; metric2[2]=weather_wind[1]; metric2[3]=weather_wind[2]; metric2[4]=' ';
        metric2[5]='P'; metric2[6]=weather_pressure[0]; metric2[7]=weather_pressure[1]; metric2[8]=weather_pressure[2]; metric2[9]=weather_pressure[3]; metric2[10]=0;
        weather_diag_set("WX DISPLAY"); weather_set_stage(WX_STAGE_DISPLAY);
        draw_telemetry_rail("LIVE",metric1,metric2,0);
    } else draw_telemetry_rail("NO DATA",weather_diag,"W--- P----",0);
    diag_log(DE_DRAW_WX_END,DIAG_CTX_RENDER,selected,has_weather);
}

static void task_label(unsigned char x, unsigned char y, const char *text)
{
    cc_color(CC_SECONDARY_TEXT); micro_text(x,y,text);
}

static void task_value_text(unsigned char x, unsigned char y, const char *text)
{
    cc_color(CC_NOMINAL); micro_text(x,y,text);
}

static void task_value_number(unsigned char x, unsigned char y, unsigned long value)
{
    cc_color(CC_NOMINAL); micro_unsigned(x,y,value);
}

static void clear_task_value(unsigned char x1, unsigned char x2, unsigned char y)
{
    unsigned char row;
    for(row=y;row<y+7;++row) undraw(x1,row,x2,row);
}

static void draw_task_heading(const char *scope)
{
    cc_color(CC_NOMINAL); micro_text(10,43,scope);
    cc_color(CC_SELECTED); micro_text(124,43,"PAGE");
    micro_unsigned(155,43,(unsigned long)(task_page+1)); micro_text(161,43,"/3");
}

static void draw_task_rail(const char *source)
{
    unsigned char row;
    for(row=41;row<=158;++row) undraw(177,row,SAFE_RIGHT,row);
    cc_color(CC_SECONDARY_TEXT); micro_text(180,43,"SCHED");
    cc_color(scheduler_paused?CC_WARNING:CC_NOMINAL); micro_text(180,51,scheduler_paused?"PAUSED":"RUN");
    task_label(180,63,"SOURCE"); cc_color(CC_SPECIAL); micro_text(180,71,source);
    task_label(180,83,"LAST I/O"); cc_color(cc_status_color()); micro_text(180,91,cc_status_short());
    task_label(180,103,"T/W BYTES"); task_value_number(234,103,bytes_received);
    task_label(180,115,"TIME SEQ");
    if(has_last_valid) task_value_number(228,115,last_sequence); else { cc_color(CC_WARNING); micro_text(228,115,"N/A"); }
}

static void draw_task_live_values(void)
{
    clear_task_value(76,172,53); task_value_text(76,53,"TASK MGR");
    clear_task_value(76,172,62); task_value_text(76,62,console_global?"GLOBAL":"LOCAL");
    clear_task_value(76,172,71); task_value_text(76,71,zip_code);
    clear_task_value(76,96,80); task_value_number(76,80,current_draw_stage);
    clear_task_value(148,172,80); task_value_number(148,80,last_draw_stage);
    clear_task_value(100,172,89); task_value_number(100,89,dirty_mask);
    clear_task_value(100,172,98); task_value_number(100,98,full_draw_count);
    clear_task_value(100,172,107); task_value_number(100,107,dirty_draw_count);
    clear_task_value(100,172,116); task_value_number(100,116,static_draw_count);
    clear_task_value(100,172,125); task_value_number(100,125,dynamic_draw_count);
    clear_task_value(100,172,134); task_value_number(100,134,frame3d_count);
    clear_task_value(100,172,143); task_value_number(100,143,globe_step_count);
    clear_task_value(100,172,152); task_value_number(100,152,news_step_count);
}

static void draw_task_live(void)
{
    draw_task_heading("APP RUNTIME LIVE");
    task_label(10,53,"SCREEN"); task_label(10,62,"MODE"); task_label(10,71,"ZIP");
    task_label(10,80,"STAGE CUR"); task_label(112,80,"PREV");
    task_label(10,89,"DIRTY MASK"); task_label(10,98,"FULL DRAW");
    task_label(10,107,"DIRTY DRAW"); task_label(10,116,"STATIC DRAW");
    task_label(10,125,"DYNAMIC DRAW"); task_label(10,134,"SAT FRAME");
    task_label(10,143,"MODULE STEP"); task_label(10,152,"NEWS STEP");
    draw_task_live_values(); draw_task_rail("APP");
}

static void draw_task_hardware(void)
{
    draw_task_heading("HW CONST / ALLOC");
    task_label(10,55,"CPU"); task_value_text(52,55,"Z80A");
    task_label(96,55,"CLOCK"); task_value_text(132,55,"3.58MHZ");
    task_label(10,67,"RAM"); task_value_text(52,67,"64K");
    task_label(96,67,"VRAM"); task_value_text(132,67,"16K");
    task_label(10,79,"VIDEO"); task_value_text(52,79,"TMS9918A");
    task_label(102,79,"MODE L"); task_value_text(138,79,"GFX II");
    task_label(10,91,"AUDIO"); task_value_text(46,91,"AY-3-8910");
    task_label(102,91,"SPR CALC"); task_value_text(158,91,"0");
    task_label(10,103,"MAP D+B"); task_value_text(88,103,"1104 B");
    task_label(10,115,"USER DATA"); cc_color(CC_WARNING); micro_text(76,115,"UNAVAILABLE");
    task_label(10,127,"NET BUF CALC"); task_value_number(88,127,(unsigned long)(sizeof(record_buffer)+sizeof(parse_buffer)));
    task_label(112,127,"FONT C"); task_value_number(154,127,(unsigned long)(10*7+26*7+9*7));
    task_label(10,139,"LINK GAP"); cc_color(CC_WARNING); micro_text(76,139,"UNAVAILABLE");
    task_label(10,151,"VRAM CALC"); task_value_number(76,151,13056UL);
    draw_task_rail("CONST/MAP");
}

static void draw_task_internal_values(void)
{
    clear_task_value(64,76,91); task_value_number(64,91,frame3d_count);
    clear_task_value(142,172,91); task_value_number(142,91,globe_step_count);
    clear_task_value(52,94,103); task_value_number(52,103,dirty_mask);
    clear_task_value(138,172,103); task_value_number(138,103,news_index);
    clear_task_value(52,94,115); task_value_text(52,115,zip_code);
    clear_task_value(138,172,115); task_value_text(138,115,console_global?"GLOBAL":"LOCAL");
    clear_task_value(52,94,127); task_value_text(52,127,sound_enabled?"ON":"OFF");
    clear_task_value(138,172,127); task_value_text(138,127,scheduler_paused?"YES":"NO");
    clear_task_value(64,172,139); task_value_text(64,139,"TASK MGR");
}

static void draw_task_internals(void)
{
    draw_task_heading("APP INTERNAL LIVE");
    task_label(10,55,"VIDEO"); task_value_text(52,55,"TMS9918A");
    task_label(102,55,"MODE L"); task_value_text(144,55,"GFX II");
    task_label(10,67,"SPR CALC"); task_value_text(64,67,"0");
    task_label(102,67,"FONT C"); task_value_text(144,67,"5X7");
    task_label(10,79,"GLOBE V"); cc_color(CC_WARNING); micro_text(64,79,"N/A");
    task_label(102,79,"GLOBE E"); cc_color(CC_WARNING); micro_text(156,79,"N/A");
    task_label(10,91,"SAT FRAME"); task_label(82,91,"MODULE STEP");
    task_label(10,103,"DIRTY"); task_label(96,103,"NEWS");
    task_label(10,115,"ZIP"); task_label(96,115,"UI MODE");
    task_label(10,127,"SOUND"); task_label(96,127,"PAUSE");
    task_label(10,139,"MODULE");
    draw_task_internal_values(); draw_task_rail("APP");
}

static void update_task_dynamic(void)
{
    if(task_page==0) draw_task_live_values();
    else if(task_page==2) draw_task_internal_values();
    if(task_page!=1) draw_task_rail("APP");
}

static void draw_task_detail(void)
{
    draw_detail_frame("NABU SYSTEM / TASK MANAGER");
    if(task_page==0) draw_task_live();
    else if(task_page==1) draw_task_hardware();
    else draw_task_internals();
}

static void draw_module(void)
{
    if(selected==0) draw_quake_detail();
    else if(selected==1) draw_space_detail();
    else if(selected==2) draw_satellite_detail();
    else if(selected==3) draw_adsb_detail();
    else if(selected==4) draw_weather_detail();
    else draw_task_detail();
    draw_stage(DRAW_STAGE_COMPLETE); dirty_mask=0;
    visual_dirty=0;
}

static void update_maximized_dynamic(void)
{
    count_inc(&dirty_draw_count); dirty_mask|=8;
    globe_phase=(unsigned char)((globe_phase+1)&15);
    count_inc(&globe_step_count);
    if(selected==5) {
        update_task_dynamic();
    } else {
        clear_detail_plot();
        if(selected==0) { if(console_global) draw_quake_global_plot(); else draw_quake_local_plot(); }
        else if(selected==1) draw_space_plot();
        else if(selected==2) draw_satellite_plot();
        else if(selected==3) { draw_adsb_plot(); draw_airspace_rail(); }
        else { draw_weather_plot(); draw_weather_alert_summary(); }
    }
    dirty_mask=0; draw_stage(DRAW_STAGE_COMPLETE);
}

static void draw_diagnostics(void)
{
    vdp_set_mode(mode_2); cc_color(CC_PRIMARY_VECTOR); clg(); draw_command_header();
    cc_color(CC_PRIMARY_TEXT); micro_text(8,32,"DIAGNOSTICS / ACTUAL STATE");
    cc_color(CC_SECONDARY_VECTOR); micro_hline(8,247,40); micro_vline(128,44,159);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(8,46,"BUILD ID BELOW");
    micro_text(8,58,"GFX MODE2 256X192"); micro_text(8,70,"MODULE"); micro_text(56,70,panel_label[selected]);
    micro_text(8,82,"MODE"); micro_text(44,82,console_global?"GLOBAL":"LOCAL");
    micro_text(8,94,"ZIP"); micro_text(38,94,zip_code); micro_text(8,106,"SOUND"); micro_text(50,106,sound_enabled?"ON":"OFF");
    micro_text(134,46,"STATE"); cc_color(cc_status_color()); micro_text(176,46,status_text());
    cc_color(CC_SECONDARY_TEXT); micro_text(134,58,"RETURN"); micro_unsigned(182,58,(unsigned long)(last_return_value<0?-last_return_value:last_return_value));
    micro_text(134,70,"BYTES"); micro_unsigned(176,70,bytes_received); micro_text(194,70,"/64");
    micro_text(134,82,"SEQ"); if(has_last_valid) micro_unsigned(164,82,last_sequence); else micro_text(164,82,"--");
    micro_text(134,94,"TARGET"); micro_unsigned(182,94,(unsigned long)(selected_target+1));
    micro_text(134,106,"LOCK"); micro_text(170,106,target_locked?"YES":"NO");
    micro_text(8,114,"SCHED"); micro_text(50,114,scheduler_paused?"PAUSED":"RUN");
    micro_text(134,114,"FULL"); micro_unsigned(170,114,full_draw_count);
    micro_text(134,124,"DIRTY"); micro_unsigned(176,124,dirty_draw_count);
    micro_text(8,122,"CHECK"); micro_text(50,122,last_checkpoint);
    micro_text(8,134,"ERROR"); cc_color(last_error[0]=='N'?CC_NOMINAL:CC_ALERT); micro_text(50,134,last_error);
    cc_color(CC_ALERT); micro_text(8,150,"LIMIT OPEN MAY BLOCK OFFLINE");
    cc_color(CC_SECONDARY_TEXT); micro_text(8,164,BUILD_ID); micro_text(8,176,"ESC/M DASHBOARD");
}

static void draw_help(void)
{
    vdp_set_mode(mode_2); cc_color(CC_PRIMARY_VECTOR); clg(); draw_command_header();
    cc_color(CC_PRIMARY_TEXT); micro_text(8,32,"HELP / VERIFIED CONTROLS");
    cc_color(CC_SECONDARY_VECTOR); micro_hline(8,247,40);
    cc_color(CC_SECONDARY_TEXT);
    micro_text(8,48,"ARROWS DASHBOARD NO WRAP"); micro_text(8,60,"ENTER OPEN / TARGET LOCK");
    micro_text(8,72,"ESC OR M DASHBOARD"); micro_text(8,84,"R STORE64 REFRESH");
    micro_text(8,96,"TASK MGR LEFT/RIGHT PAGE"); micro_text(8,108,"N NEWS  Z ZIP  B MUSIC STREAM");
    micro_text(8,120,"L/G MODE X SOUND P PAUSE"); micro_text(8,132,"D DIAG H HELP Q SAFE IDLE");
    cc_color(CC_SPECIAL); micro_text(8,144,"LIVE TIME WX EQ SPACEWX");
    cc_color(CC_SECONDARY_TEXT); micro_text(8,154,BUILD_ID);
    cc_color(CC_NOMINAL); micro_text(8,164,"MINI SLOW / MAX FASTER");
    cc_color(CC_SECONDARY_TEXT); micro_text(8,176,"ESC/M DASHBOARD");
}

static void draw_idle(void)
{
    vdp_set_mode(mode_2); cc_color(CC_PRIMARY_VECTOR); clg(); draw_command_header();
    cc_color(CC_PRIMARY_TEXT); micro_text(76,58,"SAFE IDLE");
    cc_color(CC_NOMINAL); micro_text(49,82,"NO NEW TRANSPORT CALLS");
    cc_color(CC_SECONDARY_TEXT); micro_text(61,98,"ANIMATION STOPPED");
    micro_text(55,114,"KEYBOARD RESPONSIVE");
    cc_color(cc_status_color()); micro_text(8,146,status_text());
    cc_color(CC_SECONDARY_TEXT); micro_text(61,176,"M RETURN DASHBOARD");
    stop_cue();
}

static void zip_draw_cell(unsigned char index, char digit)
{
    unsigned char row;
    unsigned char x=(unsigned char)(68+index*MICRO_ADVANCE);
    for(row=0;row<MICRO_HEIGHT;++row)
        undraw(x,(unsigned char)(91+row),(unsigned char)(x+MICRO_WIDTH-1),(unsigned char)(91+row));
    cc_color(CC_SELECTED);
    if(digit) micro_char(x,91,digit);
    else draw(x,(unsigned char)(91+MICRO_HEIGHT-1),(unsigned char)(x+MICRO_WIDTH-1),(unsigned char)(91+MICRO_HEIGHT-1));
}

static void zip_entry(void)
{
    char entered[6];
    unsigned char count=0;
    unsigned char handle;
    unsigned char i;
    unsigned char digit;
    unsigned long sequence;
    int key;
    vdp_set_mode(mode_2); cc_color(CC_PRIMARY_VECTOR); clg(); draw_command_header();
    cc_color(CC_PRIMARY_TEXT); micro_text(8,32,"ZIP / LOCATION PROFILE");
    cc_color(CC_SECONDARY_VECTOR); micro_hline(8,247,40);
    cc_color(CC_SECONDARY_TEXT); micro_text(8,54,"ENTER EXACTLY FIVE DIGITS");
    micro_text(8,68,"SEND BOUNDED GATEWAY REQUEST");
    for(i=0;i<5;++i) zip_draw_cell(i,0);
    cc_color(CC_SECONDARY_TEXT); micro_text(8,124,"ENTER ACCEPT  ESC/M CANCEL");
    for (;;) {
        key=getk();
        if ((key==KEY_ESCAPE) || (key=='m') || (key=='M')) { transient_status="ZIP CANCELLED"; return; }
        if ((key=='r') || (key=='R')) {
            diag_log(DE_KEY_R,DIAG_CTX_UI,selected,current_view);
            request_manual_refresh();
            service_global_refresh();
        } else if ((key==KEY_BACKSPACE) || (key==KEY_DELETE)) {
            if(count>0) {
                --count; entered[count]=0;
                zip_draw_cell(count,0);
            }
        } else if ((key>='0') && (key<='9') && (count<5)) {
            entered[count]=(char)key; zip_draw_cell(count,(char)key); ++count;
        } else if (key==KEY_ENTER) {
            if(count!=5){ cc_color(CC_ALERT); micro_text(8,108,"INVALID ZIP - NEED 5 DIGITS"); transient_status="INVALID ZIP"; start_cue(8); }
            else {
                entered[5]=0; text_copy_bounded(zip_code,entered,sizeof(zip_code));
                /* A new selection cannot present the prior ZIP's accepted cache. */
                zip_request_valid=0;
                ++zip_request_sequence;
                if(zip_request_sequence>999999UL) zip_request_sequence=1;
                sequence=zip_request_sequence;
                zip_request[0]='Z'; zip_request[1]='I'; zip_request[2]='P'; zip_request[3]='|';
                for(i=0;i<6;++i) {
                    digit=(unsigned char)(sequence%10UL);
                    zip_request[9-i]=(char)('0'+digit);
                    sequence/=10UL;
                }
                zip_request[10]='|';
                for(i=0;i<5;++i) zip_request[11+i]=zip_code[i];
                zip_request[16]='\n';
                stop_cue();
                handle=rn_fileOpen(ZIP_REQUEST_NAME_LENGTH,ZIP_REQUEST_NAME,OPEN_FILE_FLAG_READWRITE,0xff);
                rn_fileHandleEmptyFile(handle);
                rn_fileHandleAppend(handle,0,ZIP_REQUEST_LENGTH,zip_request);
                rn_fileHandleClose(handle);
                zip_request_valid=1;
                diag_log(DE_ZIP_SUBMIT,DIAG_CTX_UI,(unsigned char)zip_request_sequence,(unsigned char)(zip_request_sequence>>8));
                transient_status="ZIP REQUEST ATTEMPTED"; start_cue(5);
                while(cue_kind!=0){ service_sound(); delay_loop(); }
                return;
            }
        }
        service_clock(); service_sound(); service_scheduler(); delay_loop();
    }
}

static void redraw_current(void)
{
    if (current_view==VIEW_DASHBOARD) draw_dashboard();
    else if (current_view==VIEW_MODULE) draw_module();
    else if (current_view==VIEW_DIAGNOSTICS) draw_diagnostics();
    else if (current_view==VIEW_HELP) draw_help();
    else if (current_view==VIEW_IDLE) draw_idle();
}

static void service_scheduler(void)
{
    unsigned int divisor;
    service_music();
    if(service_global_refresh() && (current_view==VIEW_DASHBOARD || current_view==VIEW_MODULE)) redraw_current();
    if(scheduler_paused || (current_view!=VIEW_DASHBOARD && current_view!=VIEW_MODULE)) return;
    divisor=(current_view==VIEW_MODULE)?MAX_UPDATE_DIVISOR:MINI_UPDATE_DIVISOR;
    if(++scheduler_ticks<divisor) return;
    scheduler_ticks=0;
    if(current_view==VIEW_MODULE) {
        update_maximized_dynamic();
    } else {
        mini_phase=(unsigned char)((mini_phase+1)%10);
        count_inc(&dirty_draw_count); count_inc(&dynamic_draw_count); dirty_mask|=16;
        draw_mini_activity(mini_module);
        mini_module=(unsigned char)((mini_module+1)%6);
        if(mini_module==0) {
            news_index=(unsigned char)((news_index+1)%6);
            count_inc(&news_step_count); draw_news();
        }
        dirty_mask=0; draw_stage(DRAW_STAGE_COMPLETE);
    }
}

int main(void)
{
    int key;
    unsigned char old;
    diag_init();
    diag_log(DE_FRAME_REG_BEGIN,DIAG_CTX_NONE,0,0);
    intrinsic_di(); clock_frame_base=clock_frame_counter; intrinsic_ei();
    ncc_install_minimal_vdp_isr();
    diag_log(DE_FRAME_REG_END,DIAG_CTX_NONE,0,0);
    run_startup_splash();
    current_view=VIEW_DASHBOARD; draw_dashboard(); start_cue(1);
    diag_log(DE_BOOT_READY,DIAG_CTX_NONE,selected,current_view);
    for (;;) {
        service_clock();
        service_sound();
        service_scheduler();
        diag_check_guards();
        key=getk();
        if (key==0) { delay_loop(); continue; }
        if ((key==KEY_ESCAPE) || (key=='m') || (key=='M')) { diag_log(DE_VIEW_BACK,DIAG_CTX_UI,(unsigned char)key,current_view); current_view=VIEW_DASHBOARD; diag_log(DE_VIEW_CHANGE,DIAG_CTX_UI,current_view,selected); start_cue(4); redraw_current(); continue; }
        if ((key=='d') || (key=='D')) { current_view=VIEW_DIAGNOSTICS; diag_log(DE_VIEW_CHANGE,DIAG_CTX_UI,current_view,selected); start_cue(3); redraw_current(); continue; }
        if ((key=='h') || (key=='H')) { current_view=VIEW_HELP; diag_log(DE_VIEW_CHANGE,DIAG_CTX_UI,current_view,selected); redraw_current(); continue; }
        if ((key=='q') || (key=='Q')) { current_view=VIEW_IDLE; diag_log(DE_VIEW_CHANGE,DIAG_CTX_UI,current_view,selected); redraw_current(); continue; }
        if ((key=='j') || (key=='J')) { ncc_gate0_endurance_target=100; gate0_endurance(); transient_status="GATE0 QUALIFICATION DONE"; draw_dashboard(); continue; }
        if ((key=='u') || (key=='U')) { ncc_gate0_endurance_target=1000; gate0_endurance(); transient_status="GATE0 ORDINARY DONE"; draw_dashboard(); continue; }
        if ((key=='e') || (key=='E')) { ncc_gate0_endurance_target=5000; gate0_endurance(); transient_status="GATE0 ENDURANCE DONE"; draw_dashboard(); continue; }
        if ((key=='r') || (key=='R')) { diag_log(DE_KEY_R,DIAG_CTX_UI,selected,current_view); request_manual_refresh(); service_global_refresh();
        if(current_status==STATUS_OFFLINE || current_status==STATUS_INVALID) start_cue(8); if(current_view==VIEW_DASHBOARD) draw_dashboard(); else redraw_current(); continue; }
        if ((key=='l') || (key=='L')) { console_global=0; diag_log(DE_MODE_CHANGE,DIAG_CTX_UI,console_global,selected); transient_status="MODE LOCAL"; start_cue(6); redraw_current(); continue; }
        if ((key=='g') || (key=='G')) { console_global=1; diag_log(DE_MODE_CHANGE,DIAG_CTX_UI,console_global,selected); transient_status="MODE GLOBAL"; start_cue(6); redraw_current(); continue; }
        if ((key=='p') || (key=='P')) { scheduler_paused=(unsigned char)!scheduler_paused; transient_status=scheduler_paused?"SCHEDULER PAUSED":"SCHEDULER RUN"; start_cue(6); redraw_current(); continue; }
        if ((key=='x') || (key=='X')) {
            sound_enabled=(unsigned char)!sound_enabled; transient_status=sound_enabled?"SOUND ON":"SOUND OFF";
            if(!sound_enabled) stop_cue(); else start_cue(6); redraw_current(); continue;
        }
        if ((key=='b') || (key=='B')) {
            music_enabled=(unsigned char)!music_enabled; music_wait=0;
            music_frame_mark=clock_frame_counter; music_loop_pending=0; music_started=0;
            if(music_enabled&&!music_valid) refresh_music();
            transient_status=music_enabled?(music_valid?"MUSIC STREAM ON":"MUSIC UNAVAILABLE"):"MUSIC STREAM OFF";
            if(!music_enabled) music_silence();
            draw_music_header();
            continue;
        }
        if (current_view==VIEW_MODULE) {
            unsigned char target_count;
            if(selected==5) {
                if((key==KEY_RIGHT) && (task_page<2)) { ++task_page; start_cue(6); draw_module(); }
                else if((key==KEY_LEFT) && (task_page>0)) { --task_page; start_cue(6); draw_module(); }
                continue;
            }
            target_count=(selected==0)?quake_count():((selected==2)?4:((selected==3)?airspace_count:0));
            if (target_count && ((key==KEY_RIGHT)||(key==KEY_LEFT))) {
                if(key==KEY_RIGHT) selected_target=(unsigned char)((selected_target+1)%target_count);
                else selected_target=(unsigned char)((selected_target+target_count-1)%target_count);
                if(selected==0) format_quake_text(quake_slot()); target_locked=1; transient_status="TARGET SELECTED"; start_cue(6); draw_module();
            } else if(target_count && key==KEY_ENTER) {
                target_locked=(unsigned char)!target_locked; transient_status=target_locked?"TARGET LOCKED":"TARGET UNLOCKED"; start_cue(7); draw_module();
            }
            continue;
        }
        if (current_view!=VIEW_DASHBOARD) continue;
        old=selected;
        if ((key==KEY_RIGHT) && ((selected%3)<2)) ++selected;
        else if ((key==KEY_LEFT) && ((selected%3)>0)) --selected;
        else if ((key==KEY_UP) && (selected>=3)) selected=(unsigned char)(selected-3);
        else if ((key==KEY_DOWN) && (selected<3)) selected=(unsigned char)(selected+3);
        else if (key==KEY_ENTER) { current_view=VIEW_MODULE; diag_log(DE_VIEW_OPEN,DIAG_CTX_UI,selected,current_view); selected_target=0; target_locked=1; if(selected==5) task_page=0; start_cue(3); redraw_current(); continue; }
        else if ((key=='n') || (key=='N')) { news_index=(unsigned char)((news_index+1)%6); count_inc(&news_step_count); count_inc(&dirty_draw_count); dirty_mask|=1; draw_news(); dirty_mask=0; continue; }
        else if ((key=='z') || (key=='Z')) { current_view=VIEW_ZIP; zip_entry(); current_view=VIEW_DASHBOARD; draw_dashboard(); continue; }
        else continue;
        if (old!=selected) { diag_log(DE_SELECTION_CHANGE,DIAG_CTX_UI,old,selected); start_cue(2); update_selection(old); }
    }
}
