#include "ini_parser.h"


int handler(void *user, const char *section, const char *name, const char *value)
{
    Settings *settings = (Settings *)user;

    if (MATCH("settings", "message")) {
        if (strlen(value) > MESSAGE_MAX_LENGTH) {
            printf("Error: message length exceeds max allowed length\n");
            return 1;
        }
        mbstowcs(settings->message, value, sizeof(settings->message)/sizeof(wchar_t) - 1);
        settings->message[sizeof(settings->message)/sizeof(wchar_t) - 1] = L'\0';
    } else if (MATCH("settings", "refresh_rate")) {
        settings->refresh_rate = atoi(value);
    } else if (MATCH("settings", "message_spawn_frame_interval")) {
        settings->message_spawn_frame_interval = atoi(value);
    } else if (MATCH("settings", "max_trail_length")){
        settings->max_trail_length = atoi(value);
    } else {
        return 0;
    }
    return 1;
}