/**
 * @file main.c
 * @brief Aplicativo Reprodutor de Musica Desacoplado para Tab5 OS
 */

#include "tab5_sdk.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRACKS 32
#define MAX_NAME 64
#define MAX_PATH 256

typedef struct {
    char name[MAX_NAME];
    char fullpath[MAX_PATH];
    uint32_t size;
    uint8_t is_dir;
    tab5_ui_obj_t btn_handle;
} music_entry_t;

static music_entry_t s_tracks[MAX_TRACKS] = {0};
static uint32_t s_track_count = 0;
static int32_t s_current_index = -1;

static char s_root_dir[MAX_PATH] = "/sdcard/musica";
static char s_current_dir[MAX_PATH] = "/sdcard/musica";

static tab5_ui_obj_t s_lbl_track_title = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_lbl_status = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_play_pause = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_stop = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_prev = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_next = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_vol_slider = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_lbl_vol_val = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_list_music = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_lbl_dir_title = TAB5_UI_INVALID_OBJ;

static bool is_audio_file(const char *name)
{
    return strstr(name, ".mp3") || strstr(name, ".MP3") ||
           strstr(name, ".wav") || strstr(name, ".WAV") ||
           strstr(name, ".flac") || strstr(name, ".FLAC") ||
           strstr(name, ".ogg") || strstr(name, ".OGG") ||
           strstr(name, ".m4a") || strstr(name, ".M4A");
}

static void join_path(char *out, size_t out_size, const char *dir, const char *name)
{
    if (dir[0] != '\0' && dir[strlen(dir) - 1] == '/') {
        snprintf(out, out_size, "%s%s", dir, name);
    } else {
        snprintf(out, out_size, "%s/%s", dir, name);
    }
}

static void go_to_dir(const char *path)
{
    strncpy(s_current_dir, path, sizeof(s_current_dir) - 1);
    s_current_dir[sizeof(s_current_dir) - 1] = '\0';
}

static void go_up(void)
{
    size_t len = strlen(s_current_dir);
    if (len <= strlen(s_root_dir)) {
        go_to_dir(s_root_dir);
        return;
    }
    size_t i = len;
    while (i > 0 && s_current_dir[i - 1] == '/') {
        i--;
    }
    while (i > 0 && s_current_dir[i - 1] != '/') {
        i--;
    }
    if (i == 0) {
        go_to_dir(s_root_dir);
        return;
    }
    char parent[MAX_PATH];
    if (i == 1) {
        snprintf(parent, sizeof(parent), "/");
    } else {
        snprintf(parent, sizeof(parent), "%.*s", (int)(i - 1), s_current_dir);
    }
    go_to_dir(parent);
}

static void update_music_view(void)
{
    bool is_playing = tab5_music_is_playing();

    if (s_lbl_track_title != TAB5_UI_INVALID_OBJ) {
        if (s_current_index >= 0 && (uint32_t)s_current_index < s_track_count && !s_tracks[s_current_index].is_dir) {
            tab5_ui_label_set_text(s_lbl_track_title, s_tracks[s_current_index].name);
        } else {
            tab5_ui_label_set_text(s_lbl_track_title, "Nenhuma faixa selecionada");
        }
    }

    if (s_lbl_status != TAB5_UI_INVALID_OBJ) {
        if (is_playing) {
            tab5_ui_label_set_text(s_lbl_status, "Estado: Reproduzindo");
            tab5_ui_obj_set_style_text_color(s_lbl_status, 0x22C55E, 255);
        } else {
            tab5_ui_label_set_text(s_lbl_status, "Estado: Parado");
            tab5_ui_obj_set_style_text_color(s_lbl_status, 0x94A3B8, 255);
        }
    }

    if (s_btn_play_pause != TAB5_UI_INVALID_OBJ) {
        tab5_ui_label_set_text(s_btn_play_pause, is_playing ? LV_SYMBOL_PAUSE " Pausar" : LV_SYMBOL_PLAY " Reproduzir");
    }
}

static void play_track_entry(uint32_t index)
{
    if (index >= s_track_count || s_tracks[index].is_dir) {
        return;
    }
    s_current_index = (int32_t)index;
    tab5_music_play(s_tracks[index].fullpath);
    tab5_sound_play_beep(1200, 30);
    tab5_ui_show_toast("Iniciando reproducao", 1000);
    update_music_view();
}

static void on_play_pause_clicked(void)
{
    if (tab5_music_is_playing()) {
        tab5_music_pause();
        tab5_sound_play_beep(800, 30);
        tab5_ui_show_toast("Musica pausada", 1000);
    } else {
        if (s_current_index >= 0 && (uint32_t)s_current_index < s_track_count) {
            tab5_music_resume();
            tab5_sound_play_beep(1200, 30);
            tab5_ui_show_toast("Retomando reproducao", 1000);
        } else if (s_track_count > 0) {
            uint32_t first = 0;
            while (first < s_track_count && s_tracks[first].is_dir) {
                first++;
            }
            if (first < s_track_count) {
                play_track_entry(first);
            } else {
                tab5_ui_show_toast("Nenhuma musica encontrada", 1500);
            }
        } else {
            tab5_ui_show_toast("Nenhuma musica encontrada", 1500);
        }
    }
    update_music_view();
}

static void on_stop_clicked(void)
{
    tab5_music_stop();
    tab5_sound_play_beep(600, 40);
    tab5_ui_show_toast("Reproducao finalizada", 1000);
    update_music_view();
}

static void on_prev_clicked(void)
{
    if (s_track_count == 0) {
        return;
    }
    int32_t prev = s_current_index - 1;
    while (prev >= 0 && s_tracks[prev].is_dir) {
        prev--;
    }
    if (prev < 0) {
        prev = (int32_t)s_track_count - 1;
        while (prev >= 0 && s_tracks[prev].is_dir) {
            prev--;
        }
    }
    if (prev >= 0) {
        play_track_entry((uint32_t)prev);
    }
}

static void on_next_clicked(void)
{
    if (s_track_count == 0) {
        return;
    }
    int32_t next = s_current_index + 1;
    while (next >= 0 && (uint32_t)next < s_track_count && s_tracks[next].is_dir) {
        next++;
    }
    if ((uint32_t)next >= s_track_count) {
        next = 0;
        while (next >= 0 && (uint32_t)next < s_track_count && s_tracks[next].is_dir) {
            next++;
        }
    }
    if (next >= 0 && (uint32_t)next < s_track_count) {
        play_track_entry((uint32_t)next);
    }
}

static void scan_music_files(void)
{
    if (s_list_music == TAB5_UI_INVALID_OBJ) {
        return;
    }
    tab5_ui_obj_clean(s_list_music);
    s_track_count = 0;
    s_current_index = -1;

    if (s_lbl_dir_title != TAB5_UI_INVALID_OBJ) {
        tab5_ui_label_set_text(s_lbl_dir_title, s_current_dir);
    }

    tab5_dir_entry_t entries[32];
    uint32_t count = 0;
    tab5_err_t err = tab5_storage_scandir(s_current_dir, entries, 32, &count);
    if (err != TAB5_OK || count == 0) {
        if (strcmp(s_current_dir, s_root_dir) == 0) {
            go_to_dir("/sdcard");
            tab5_storage_scandir(s_current_dir, entries, 32, &count);
            if (s_lbl_dir_title != TAB5_UI_INVALID_OBJ) {
                tab5_ui_label_set_text(s_lbl_dir_title, s_current_dir);
            }
        }
    }

    if (strcmp(s_current_dir, s_root_dir) != 0 && s_track_count < MAX_TRACKS) {
        tab5_ui_obj_t btn = tab5_ui_list_add_btn(s_list_music, LV_SYMBOL_LEFT, "..");
        if (btn != TAB5_UI_INVALID_OBJ) {
            s_tracks[s_track_count].name[0] = '\0';
            s_tracks[s_track_count].fullpath[0] = '\0';
            s_tracks[s_track_count].is_dir = 2;
            s_tracks[s_track_count].btn_handle = btn;
            s_track_count++;
        }
    }

    if (err == TAB5_OK && count > 0) {
        for (uint32_t pass = 0; pass < 2; pass++) {
            for (uint32_t i = 0; i < count && s_track_count < MAX_TRACKS; i++) {
                if (entries[i].name[0] == '.') {
                    continue;
                }
                bool is_dir = entries[i].is_dir != 0;
                if (pass == 0 && !is_dir) {
                    continue;
                }
                if (pass == 1 && (is_dir || !is_audio_file(entries[i].name))) {
                    continue;
                }
                char label_buf[160];
                if (is_dir) {
                    snprintf(label_buf, sizeof(label_buf), "%s/", entries[i].name);
                } else if (entries[i].size >= 1024 * 1024) {
                    snprintf(label_buf, sizeof(label_buf), "%s (%.1f MB)", entries[i].name,
                             (float)entries[i].size / (1024.0f * 1024.0f));
                } else if (entries[i].size >= 1024) {
                    snprintf(label_buf, sizeof(label_buf), "%s (%u KB)", entries[i].name, entries[i].size / 1024);
                } else {
                    snprintf(label_buf, sizeof(label_buf), "%s (%u B)", entries[i].name, entries[i].size);
                }

                tab5_ui_obj_t btn = tab5_ui_list_add_btn(s_list_music, is_dir ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_AUDIO,
                                                         label_buf);
                if (btn != TAB5_UI_INVALID_OBJ) {
                    strncpy(s_tracks[s_track_count].name, entries[i].name, sizeof(s_tracks[s_track_count].name) - 1);
                    join_path(s_tracks[s_track_count].fullpath, sizeof(s_tracks[s_track_count].fullpath),
                              s_current_dir, entries[i].name);
                    s_tracks[s_track_count].size = entries[i].size;
                    s_tracks[s_track_count].is_dir = is_dir ? 1 : 0;
                    s_tracks[s_track_count].btn_handle = btn;
                    s_track_count++;
                }
            }
        }
    }

    if (s_track_count == 0) {
        tab5_ui_list_add_btn(s_list_music, LV_SYMBOL_CLOSE, "Nenhuma musica ou pasta encontrada");
    }
    update_music_view();
}

static void handle_list_click(tab5_ui_obj_t btn)
{
    for (uint32_t i = 0; i < s_track_count; i++) {
        if (s_tracks[i].btn_handle == btn) {
            if (s_tracks[i].is_dir == 2) {
                go_up();
                scan_music_files();
                return;
            }
            if (s_tracks[i].is_dir == 1) {
                go_to_dir(s_tracks[i].fullpath);
                scan_music_files();
                return;
            }
            play_track_entry(i);
            return;
        }
    }
}

static void build_music_ui(void)
{
    uint32_t pal_surface = tab5_ui_theme_get_color(TAB5_UI_COLOR_SURFACE);
    uint32_t pal_surface_alt = tab5_ui_theme_get_color(TAB5_UI_COLOR_SURFACE_ALT);
    uint32_t pal_border = tab5_ui_theme_get_color(TAB5_UI_COLOR_BORDER);
    uint32_t pal_text = tab5_ui_theme_get_color(TAB5_UI_COLOR_TEXT);
    uint32_t pal_text_muted = tab5_ui_theme_get_color(TAB5_UI_COLOR_TEXT_MUTED);
    uint32_t pal_accent = tab5_ui_theme_get_color(TAB5_UI_COLOR_ACCENT);

    tab5_ui_obj_t scr = tab5_ui_get_screen();

    tab5_ui_obj_t main_cont = tab5_ui_container_create(scr);
    tab5_ui_obj_set_size(main_cont, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_align(main_cont, TAB5_UI_ALIGN_TOP_MID, 0, 104);
    tab5_ui_obj_set_flex_flow(main_cont, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(main_cont, 0, 0);
    tab5_ui_obj_set_style_border(main_cont, 0, 0);
    tab5_ui_obj_set_pad(main_cont, 14);
    tab5_ui_obj_set_gap(main_cont, 14);

    // 1. Card Player Tocando Agora
    tab5_ui_obj_t card_player = tab5_ui_container_create(main_cont);
    tab5_ui_obj_set_size(card_player, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(card_player, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(card_player, pal_surface, 255);
    tab5_ui_obj_set_style_border(card_player, pal_border, 1);
    tab5_ui_obj_set_style_radius(card_player, 12);
    tab5_ui_obj_set_pad(card_player, 16);
    tab5_ui_obj_set_gap(card_player, 12);

    tab5_ui_obj_t lbl_p_title = tab5_ui_label_create(card_player, LV_SYMBOL_AUDIO "  REPRODUZINDO AGORA");
    tab5_ui_obj_set_style_text_color(lbl_p_title, pal_accent, 255);

    s_lbl_track_title = tab5_ui_label_create(card_player, "Nenhuma faixa selecionada");
    tab5_ui_obj_set_style_text_color(s_lbl_track_title, pal_text, 255);

    s_lbl_status = tab5_ui_label_create(card_player, "Estado: Parado");
    tab5_ui_obj_set_style_text_color(s_lbl_status, pal_text_muted, 255);

    // Controles de Playback
    tab5_ui_obj_t row_ctrl = tab5_ui_container_create(card_player);
    tab5_ui_obj_set_size(row_ctrl, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(row_ctrl, TAB5_UI_FLEX_FLOW_ROW);
    tab5_ui_obj_set_style_bg(row_ctrl, 0, 0);
    tab5_ui_obj_set_style_border(row_ctrl, 0, 0);
    tab5_ui_obj_set_gap(row_ctrl, 10);

    s_btn_prev = tab5_ui_btn_create(row_ctrl, LV_SYMBOL_PREV " Anterior");
    tab5_ui_obj_set_style_bg(s_btn_prev, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_prev, pal_text, 255);
    tab5_ui_obj_set_flex_grow(s_btn_prev, 1);

    s_btn_play_pause = tab5_ui_btn_create(row_ctrl, LV_SYMBOL_PLAY " Reproduzir");
    tab5_ui_obj_set_style_bg(s_btn_play_pause, pal_accent, 255);
    tab5_ui_obj_set_style_text_color(s_btn_play_pause, 0xFFFFFF, 255);
    tab5_ui_obj_set_flex_grow(s_btn_play_pause, 2);

    s_btn_stop = tab5_ui_btn_create(row_ctrl, LV_SYMBOL_STOP " Parar");
    tab5_ui_obj_set_style_bg(s_btn_stop, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_stop, pal_text, 255);
    tab5_ui_obj_set_flex_grow(s_btn_stop, 1);

    s_btn_next = tab5_ui_btn_create(row_ctrl, LV_SYMBOL_NEXT " Proxima");
    tab5_ui_obj_set_style_bg(s_btn_next, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_next, pal_text, 255);
    tab5_ui_obj_set_flex_grow(s_btn_next, 1);

    // Controle de Volume
    tab5_ui_obj_t row_vol = tab5_ui_container_create(card_player);
    tab5_ui_obj_set_size(row_vol, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(row_vol, TAB5_UI_FLEX_FLOW_ROW);
    tab5_ui_obj_set_style_bg(row_vol, 0, 0);
    tab5_ui_obj_set_style_border(row_vol, 0, 0);
    tab5_ui_obj_set_gap(row_vol, 12);

    tab5_ui_obj_t lbl_vol = tab5_ui_label_create(row_vol, LV_SYMBOL_VOLUME_MAX " Volume");
    tab5_ui_obj_set_style_text_color(lbl_vol, pal_text, 255);

    int cur_vol = tab5_music_get_volume();
    s_vol_slider = tab5_ui_slider_create(row_vol, 0, 100);
    tab5_ui_slider_set_value(s_vol_slider, cur_vol);
    tab5_ui_obj_set_flex_grow(s_vol_slider, 1);

    char vol_txt[16];
    snprintf(vol_txt, sizeof(vol_txt), "%d%%", cur_vol);
    s_lbl_vol_val = tab5_ui_label_create(row_vol, vol_txt);
    tab5_ui_obj_set_style_text_color(s_lbl_vol_val, pal_text_muted, 255);

    // 2. Card Lista de Músicas
    tab5_ui_obj_t card_list = tab5_ui_container_create(main_cont);
    tab5_ui_obj_set_size(card_list, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(card_list, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(card_list, pal_surface, 255);
    tab5_ui_obj_set_style_border(card_list, pal_border, 1);
    tab5_ui_obj_set_style_radius(card_list, 12);
    tab5_ui_obj_set_pad(card_list, 16);
    tab5_ui_obj_set_gap(card_list, 10);

    tab5_ui_obj_t lbl_list_title = tab5_ui_label_create(card_list, LV_SYMBOL_DIRECTORY "  BIBLIOTECA DE MUSICAS");
    tab5_ui_obj_set_style_text_color(lbl_list_title, pal_accent, 255);

    s_lbl_dir_title = tab5_ui_label_create(card_list, s_current_dir);
    tab5_ui_obj_set_style_text_color(s_lbl_dir_title, pal_text_muted, 255);

    s_list_music = tab5_ui_list_create(card_list);
    tab5_ui_obj_set_size(s_list_music, TAB5_UI_PCT(100), 240);

    scan_music_files();
    update_music_view();
}

static void app_open_file(const char *path)
{
    if (path != NULL && path[0] != '\0') {
        tab5_music_play(path);
        const char *last_slash = strrchr(path, '/');
        const char *fn = (last_slash != NULL) ? last_slash + 1 : path;
        if (s_lbl_track_title != TAB5_UI_INVALID_OBJ) {
            tab5_ui_label_set_text(s_lbl_track_title, fn);
        }
        update_music_view();
        tab5_ui_show_toast("Reproduzindo arquivo aberto", 1500);
    }
}

TAB5_APP_EXPORT void tab5_app_on_open_file(const char *path)
{
    app_open_file(path);
}

static void app_init(void)
{
    tab5_system_log(2, "tab5_music", "Aplicativo Musica iniciado");
    tab5_ui_app_bar_set_title("Musica");
    build_music_ui();
}

static void app_resume(void)
{
    tab5_system_log(2, "tab5_music", "Musica retomado");
    scan_music_files();
    update_music_view();
}

static void app_pause(void)
{
    tab5_system_log(2, "tab5_music", "Musica pausado");
}

static void app_destroy(void)
{
    tab5_system_log(2, "tab5_music", "Musica finalizado");
}

TAB5_APP_EXPORT void tab5_app_on_theme_changed(bool dark)
{
    (void)dark;
    tab5_ui_clear_content();
    build_music_ui();
}

TAB5_APP_EXPORT void tab5_app_on_ui_event(tab5_ui_obj_t obj, uint32_t event_type, int32_t event_val)
{
    if (event_type == TAB5_UI_EVENT_VALUE_CHANGED && obj == s_vol_slider) {
        int32_t vol = tab5_ui_slider_get_value(s_vol_slider);
        tab5_music_set_volume(vol);
        char vol_txt[16];
        snprintf(vol_txt, sizeof(vol_txt), "%d%%", (int)vol);
        if (s_lbl_vol_val != TAB5_UI_INVALID_OBJ) {
            tab5_ui_label_set_text(s_lbl_vol_val, vol_txt);
        }
        return;
    }

    if (event_type != TAB5_UI_EVENT_CLICKED) {
        return;
    }

    if (obj == s_btn_play_pause) {
        on_play_pause_clicked();
        return;
    } else if (obj == s_btn_stop) {
        on_stop_clicked();
        return;
    } else if (obj == s_btn_prev) {
        on_prev_clicked();
        return;
    } else if (obj == s_btn_next) {
        on_next_clicked();
        return;
    }

    for (uint32_t i = 0; i < s_track_count; i++) {
        if (obj == s_tracks[i].btn_handle) {
            handle_list_click(obj);
            return;
        }
    }
}

TAB5_APP_EXPORT int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    tab5_lifecycle_callbacks_t cbs = {
        .on_init = app_init,
        .on_resume = app_resume,
        .on_pause = app_pause,
        .on_destroy = app_destroy,
        .on_open_file = app_open_file,
    };

    tab5_lifecycle_register(&cbs);
    app_init();
    return 0;
}
