#include "cities.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void cities_load(City *list, int *count) {
    *count = 0;
    FILE *f = fopen(CITIES_FILE, "r");
    if (!f) return; // nessuna citta' di default
    while (*count < MAX_CITIES) {
        City *c = &list[*count];
        if (fscanf(f, "%47[^|]|%f|%f|%39[^\n]\n",
                   c->name, &c->lat, &c->lon, c->timezone) != 4) break;
        (*count)++;
    }
    fclose(f);
}

void cities_save(const City *list, int count) {
    FILE *f = fopen(CITIES_FILE, "w");
    if (!f) return;
    for (int i = 0; i < count; i++)
        fprintf(f, "%s|%.4f|%.4f|%s\n",
                list[i].name, list[i].lat,
                list[i].lon, list[i].timezone);
    fclose(f);
}

void cities_add(City *list, int *count, const char *name,
                float lat, float lon, const char *tz) {
    if (*count >= MAX_CITIES) return;
    strncpy(list[*count].name,     name, CITY_NAME_LEN - 1);
    list[*count].name[CITY_NAME_LEN - 1] = '\0';
    list[*count].lat = lat;
    list[*count].lon = lon;
    strncpy(list[*count].timezone, tz, 39);
    list[*count].timezone[39] = '\0';
    (*count)++;
}

void cities_remove(City *list, int *count, int index) {
    if (index < 0 || index >= *count) return;
    for (int i = index; i < *count - 1; i++)
        list[i] = list[i+1];
    (*count)--;
}

void cities_swap(City *list, int a, int b) {
    if (a < 0 || b < 0) return;
    City tmp = list[a];
    list[a]  = list[b];
    list[b]  = tmp;
}