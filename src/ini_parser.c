#include "ini_parser.h"

int handler(void *user, const char *section, const char *name, const char *value)
{
    Settings *settings = (Settings *)user;

    if (MATCH("settings", "refresh_rate")) {
        settings->refresh_rate = atoi(value);
    } else if (MATCH("settings", "message_spawn_frame_interval")) {
        settings->message_spawn_frame_interval = atoi(value);
    } else {
        return 0;
    }
    return 1;
}