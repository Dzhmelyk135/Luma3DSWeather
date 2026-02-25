#ifndef CITIES_H
#define CITIES_H

#define MAX_CITIES     20
#define CITY_NAME_LEN  48
#define CITIES_FILE    "/3ds/3ds-weather/cities.txt"
#define LANG_FILE      "/3ds/3ds-weather/lang.txt"

typedef struct {
    char  name[CITY_NAME_LEN];
    float lat;
    float lon;
    char  timezone[40];
} City;

void cities_load(City *list, int *count);
void cities_save(const City *list, int count);
void cities_add(City *list, int *count, const char *name,
                float lat, float lon, const char *tz);
void cities_remove(City *list, int *count, int index);
void cities_swap(City *list, int a, int b);

#endif