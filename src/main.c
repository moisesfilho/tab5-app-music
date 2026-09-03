/**
 * @file main.c
 * @brief Aplicativo Reprodutor de Musica para Tab5 OS
 */

#include "tab5_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool s_playing = false;
static char s_current_track[128] = "Sem faixa selecionada";

static void update_player_view(void)
{
    char buf[512];
    snprintf(buf, sizeof(buf),
             "====================================\n"
             "        REPRODUTOR DE MUSICA        \n"
             "====================================\n\n"
             " Faixa:    %s\n"
             " Status:   %s\n"
             " Volume:   80%%\n"
             " Saida:    Codec ES8388 / Fone 3.5mm\n\n"
             " Lista de Reproducao:\n"
             "  1. [MP3] Synthwave_01.mp3\n"
             "  2. [WAV] Lo-Fi_Study.wav\n"
             "  3. [MP3] Retro_Cyber_Theme.mp3\n\n"
             " Pressione Play/Pause na barra superior.\n",
             s_current_track,
             s_playing ? "[ TOCANDO > ]" : "[ PAUSADO || ]");

    tab5_ui_obj_t ta = tab5_ui_get_main_textarea();
    if (ta != NULL) {
        tab5_ui_textarea_set_text(ta, buf);
    }
}

static void app_open_file(const char *filepath)
{
    if (filepath != NULL && filepath[0] != '\0') {
        const char *last_slash = strrchr(filepath, '/');
        const char *filename = (last_slash != NULL) ? last_slash + 1 : filepath;
        strncpy(s_current_track, filename, sizeof(s_current_track) - 1);
        s_playing = true;
        update_player_view();
        tab5_ui_show_toast("Abrindo faixa de audio", 2000);
    }
}

static void app_init(void)
{
    tab5_system_log(2, "tab5_music", "Aplicativo Musica iniciado");
    tab5_ui_app_bar_set_title("Musica");
    update_player_view();
}

static void app_resume(void)
{
    tab5_system_log(2, "tab5_music", "Musica retomado");
    update_player_view();
}

static void app_pause(void)
{
    tab5_system_log(2, "tab5_music", "Musica pausado");
}

static void app_destroy(void)
{
    tab5_system_log(2, "tab5_music", "Musica finalizado");
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
