#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ini.h>
#include <wchar.h>

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

#define MESSAGE_MAX_LENGTH 2048

typedef struct
{
    wchar_t message[MESSAGE_MAX_LENGTH];
    int refresh_rate;
    int message_spawn_frame_interval;
    int max_trail_length;
} Settings;

int handler(void *user, const char *section, const char *name, const char *value);

#endif // !INI_PARSER_H