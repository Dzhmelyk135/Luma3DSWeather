#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <3ds.h>
#include "weather.h"
#include "cities.h"
#include "lang.h"

#define C_RST  "\x1b[0m"
#define C_RED  "\x1b[31m"
#define C_GRN  "\x1b[32m"
#define C_YLW  "\x1b[33m"
#define C_BLU  "\x1b[34m"
#define C_MAG  "\x1b[35m"
#define C_CYN  "\x1b[36m"
#define C_WHT  "\x1b[37m"
#define C_BLD  "\x1b[1m"

#define T(k) lang_get(k)

// ── Forward declarations ──────────────────────────────────────────────────
static void draw_city_list(const City *c, int n, int sel);
static void draw_current(const WeatherData *w, const char *city);
static void draw_hourly(const WeatherData *w, const char *city, int off);
static void draw_daily(const WeatherData *w, const char *city);
static void draw_details(const WeatherData *w, const char *city);
static void draw_legend(void);
static void draw_language(int sel);
static void draw_reorder(const City *c, int n, int sel, int moving);
static void draw_compare(const WeatherData *w1, const WeatherData *w2,
                          const char *c1, const char *c2);
static void draw_credits(void);
static void draw_menu(int sel);

typedef enum {
    SCR_CITY_LIST,
    SCR_CURRENT,
    SCR_HOURLY,
    SCR_DAILY,
    SCR_DETAILS,
    SCR_LEGEND,
    SCR_LANGUAGE,
    SCR_REORDER,
    SCR_COMPARE_SEL,
    SCR_COMPARE,
    SCR_CREDITS,
    SCR_MENU,
} Screen;

typedef enum {
    MENU_LANGUAGE = 0,
    MENU_LEGEND,
    MENU_COMPARE,
    MENU_CREDITS,
    MENU_COUNT
} MenuItem;

static const char *menu_labels[MENU_COUNT] = {
    "Language / Lingua",
    "Symbol legend",
    "Compare cities",
    "Credits",
};

// ── Console handles ───────────────────────────────────────────────────────
static PrintConsole topScreen, botScreen;

// ── Lang persistence ──────────────────────────────────────────────────────
static void lang_save(void) {
    FILE *f = fopen(LANG_FILE, "w");
    if (!f) return;
    fprintf(f, "%d\n", (int)currentLang);
    fclose(f);
}

static void lang_load(void) {
    FILE *f = fopen(LANG_FILE, "r");
    if (!f) {
        // Prima avvio: imposta inglese come default
        lang_set(LANG_EN);
        return;
    }
    int id = 0;
    fscanf(f, "%d", &id);
    fclose(f);
    lang_set((LangID)id);
}

// ── Helpers ───────────────────────────────────────────────────────────────
static const char *wind_dir_str(int deg) {
    const char *d[] = {"N","NE","E","SE","S","SO","O","NO"};
    return d[((deg + 22) % 360) / 45];
}

// ── Draw helpers ──────────────────────────────────────────────────────────
static void draw_header_top(const char *title, const char *sub) {
    consoleSelect(&topScreen);
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_YLW C_BLD " %s\n" C_RST, title);
    if (sub && sub[0])
        printf(C_WHT " %s\n" C_RST, sub);
    printf(C_CYN "================================\n" C_RST);
}

static void draw_header_bot(const char *title) {
    consoleSelect(&botScreen);
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_MAG C_BLD " %s\n" C_RST, title);
    printf(C_CYN "================================\n" C_RST);
}

// ── Menu SELECT ───────────────────────────────────────────────────────────
static void draw_menu(int sel) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top("MENU", NULL);
    printf("\n");
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == sel)
            printf(C_GRN C_BLD " > %s\n" C_RST, menu_labels[i]);
        else
            printf(C_WHT "   %s\n" C_RST, menu_labels[i]);
    }
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT " UP/DOWN: navigate\n" C_RST);
    printf(C_WHT " A: enter  B/SELECT: close\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("3DS WEATHER");
    printf(C_WHT "\n By: " C_GRN C_BLD "Dzhmelyk135\n\n" C_RST);
    printf(C_WHT " github.com/\n" C_RST);
    printf(C_CYN "   Dzhmelyk135/\n" C_RST);
    printf(C_CYN "   Luma3DSWeather\n\n" C_RST);
    printf(C_WHT " Data: " C_CYN "Open-Meteo.com\n" C_RST);
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT " SELECT: open/close menu\n" C_RST);
}

// ── Schermata lista citta' ────────────────────────────────────────────────
static void draw_city_list(const City *c, int n, int sel) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_APP_TITLE), T(STR_CITY_LIST_TITLE));
    printf(C_WHT " %s\n %s\n" C_RST,
           T(STR_NAV_HINT), T(STR_ADD_HINT));
    printf(C_CYN "--------------------------------\n" C_RST);
    if (n == 0) {
        printf(C_RED " %s\n %s\n" C_RST,
               T(STR_NO_CITIES), T(STR_FIRST_CITY));
    } else {
        for (int i = 0; i < n; i++) {
            if (i == sel)
                printf(C_GRN C_BLD " > %s\n" C_RST, c[i].name);
            else
                printf(C_WHT "   %s\n" C_RST, c[i].name);
        }
    }
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " A:meteo  X:add  Y:del  START:exit\n" C_RST);
    printf(C_WHT " L:reorder  R:compare  SEL:menu\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("3DS WEATHER  by Dzhmelyk135");
    printf(C_CYN "\n github.com/Dzhmelyk135/\n" C_RST);
    printf(C_CYN " Luma3DSWeather\n\n" C_RST);
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " A:      " C_CYN "fetch weather\n" C_RST);
    printf(C_WHT " X:      " C_CYN "add city\n" C_RST);
    printf(C_WHT " Y:      " C_CYN "delete city\n" C_RST);
    printf(C_WHT " L:      " C_CYN "reorder cities\n" C_RST);
    printf(C_WHT " R:      " C_CYN "compare cities\n" C_RST);
    printf(C_WHT " SELECT: " C_CYN "menu\n" C_RST);
    printf(C_WHT " START:  " C_CYN "exit\n" C_RST);
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_YLW " %s\n" C_RST, T(STR_POWERED_BY));
}

// ── Schermata meteo attuale ───────────────────────────────────────────────
static void draw_current(const WeatherData *w, const char *city) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_CURRENT_TITLE), city);
    printf(C_YLW "\n %s  %s\n\n" C_RST,
           weather_code_icon(w->weather_code_now),
           weather_code_desc(w->weather_code_now));
    printf(C_WHT "%s" C_YLW "%.1fC\n" C_RST,
           T(STR_TEMP), w->temp_now);
    printf(C_WHT "%s" C_YLW "%.1fC\n" C_RST,
           T(STR_FEELS), w->feels_like_now);
    printf(C_WHT "%s" C_CYN "%.0f%%\n" C_RST,
           T(STR_HUMIDITY), w->humidity_now);
    printf(C_WHT "%s" C_GRN "%.0f hPa\n" C_RST,
           T(STR_PRESSURE), w->pressure_now);
    printf(C_WHT "%s" C_MAG "%.1f km/h %s\n" C_RST,
           T(STR_WIND), w->wind_now,
           wind_dir_str(w->wind_dir_now));
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " L:hourly  R:7days  X:details  B:back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("DETAILS + NEXT HOURS");
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_SUNRISE), w->sunrise_hour, w->sunrise_min);
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_SUNSET),  w->sunset_hour,  w->sunset_min);
    printf(C_WHT "%s" C_RED "%.1f\n" C_RST,
           T(STR_UV), w->uv_index);
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_CYN " Hour Temp  Rain Weather\n" C_RST);
    int cur = w->current_hour;
    int shown = 0;
    for (int i = cur; i < HOURLY_COUNT && shown < 7; i++, shown++) {
        printf(i == cur ? C_YLW C_BLD : C_WHT);
        printf(" %02d:00 %4.1fC %3.1fmm %s\n" C_RST,
               i, w->hourly_temp[i],
               w->hourly_precip[i],
               weather_code_icon(w->hourly_code[i]));
    }
}

// ── Schermata oraria ──────────────────────────────────────────────────────
static void draw_hourly(const WeatherData *w,
                         const char *city, int off) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_HOURLY_TITLE), city);
    printf(C_CYN " Hour Temp   Rain  Hum  Weather\n" C_RST);
    printf(C_CYN "--------------------------------\n" C_RST);
    int shown = 0;
    for (int i = off; i < HOURLY_COUNT && shown < 12; i++, shown++) {
        if (i == w->current_hour)
            printf(C_YLW C_BLD " %02d:00 %4.1fC %3.1fmm %3.0f%% %s<\n" C_RST,
                   i, w->hourly_temp[i], w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
        else
            printf(C_WHT " %02d:00" C_YLW " %4.1fC"
                   C_CYN " %3.1fmm" C_BLU " %3.0f%%"
                   C_GRN " %s\n" C_RST,
                   i, w->hourly_temp[i], w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
    }
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " UP/DOWN: scroll  B: back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("NEXT HOURS");
    printf(C_CYN " Hour Temp   Rain  Hum  Weather\n" C_RST);
    printf(C_CYN "--------------------------------\n" C_RST);
    int start2 = off + 12;
    for (int i = start2; i < HOURLY_COUNT; i++) {
        if (i == w->current_hour)
            printf(C_YLW C_BLD " %02d:00 %4.1fC %3.1fmm %3.0f%% %s<\n" C_RST,
                   i, w->hourly_temp[i], w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
        else
            printf(C_WHT " %02d:00" C_YLW " %4.1fC"
                   C_CYN " %3.1fmm" C_BLU " %3.0f%%"
                   C_GRN " %s\n" C_RST,
                   i, w->hourly_temp[i], w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
    }
}

// ── Schermata giornaliera ─────────────────────────────────────────────────
static void draw_daily(const WeatherData *w, const char *city) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_DAILY_TITLE), city);
    printf(C_CYN " Date      Max  Min  Rain Wind\n" C_RST);
    printf(C_CYN "----------------------------------\n" C_RST);
    for (int i = 0; i < 4 && i < FORECAST_DAYS; i++) {
        printf(i==0 ? C_YLW C_BLD : C_WHT);
        printf(" %s %3.0fC %3.0fC %3.1fmm %3.0fkm %s\n" C_RST,
               w->daily_date[i],
               w->daily_max[i], w->daily_min[i],
               w->daily_precip[i], w->daily_wind_max[i],
               weather_code_icon(w->daily_code[i]));
    }
    printf(C_CYN "----------------------------------\n" C_RST);
    printf(C_WHT " B:back  L:hourly  X:details\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("NEXT DAYS");
    printf(C_CYN " Date      Max  Min  Rain Wind\n" C_RST);
    printf(C_CYN "----------------------------------\n" C_RST);
    for (int i = 4; i < FORECAST_DAYS; i++) {
        printf(C_WHT " %s %3.0fC %3.0fC %3.1fmm %3.0fkm %s\n" C_RST,
               w->daily_date[i],
               w->daily_max[i], w->daily_min[i],
               w->daily_precip[i], w->daily_wind_max[i],
               weather_code_icon(w->daily_code[i]));
    }
}

// ── Schermata dettagli ────────────────────────────────────────────────────
static void draw_details(const WeatherData *w, const char *city) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_DETAILS_TITLE), city);
    printf("\n");
    printf(C_WHT "%s" C_GRN "%.1f hPa\n" C_RST,
           T(STR_PRESSURE),  w->pressure_now);
    printf(C_WHT "%s" C_MAG "%.1f km/h\n" C_RST,
           T(STR_WIND),      w->wind_now);
    printf(C_WHT "%s" C_MAG "%d deg (%s)\n" C_RST,
           T(STR_WIND_DIR),  w->wind_dir_now,
           wind_dir_str(w->wind_dir_now));
    printf(C_WHT "%s" C_CYN "%.0f%%\n" C_RST,
           T(STR_HUMIDITY),  w->humidity_now);
    printf(C_WHT "%s" C_YLW "%.1fC\n" C_RST,
           T(STR_FEELS_LIKE),w->feels_like_now);
    printf(C_WHT "%s" C_RED "%.1f\n"  C_RST,
           T(STR_UV),        w->uv_index);
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_DAWN),  w->sunrise_hour, w->sunrise_min);
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_DUSK),  w->sunset_hour,  w->sunset_min);
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " B: back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("CHARTS");

    int bars = (int)((w->pressure_now - 970.f) / 60.f * 28.f);
    bars = bars < 0 ? 0 : bars > 28 ? 28 : bars;
    printf(C_GRN " Pressure:\n [");
    for (int i=0;i<28;i++)
        printf(i<bars ? C_GRN "#" C_RST : C_WHT "-" C_RST);
    printf(C_CYN "] %.0fhPa\n\n" C_RST, w->pressure_now);

    int wb = (int)(w->wind_now / 120.f * 28.f);
    wb = wb > 28 ? 28 : wb;
    printf(C_MAG " Wind:\n [");
    for (int i=0;i<28;i++)
        printf(i<wb ? C_MAG "#" C_RST : C_WHT "-" C_RST);
    printf(C_MAG "] %.1fkm/h\n\n" C_RST, w->wind_now);

    int ub = (int)(w->uv_index / 11.f * 28.f);
    ub = ub > 28 ? 28 : ub;
    printf(C_RED " UV:\n [");
    for (int i=0;i<28;i++)
        printf(i<ub ? C_RED "#" C_RST : C_WHT "-" C_RST);
    printf(C_RED "] %.1f\n\n" C_RST, w->uv_index);

    int hb = (int)(w->humidity_now / 100.f * 28.f);
    hb = hb > 28 ? 28 : hb;
    printf(C_CYN " Humidity:\n [");
    for (int i=0;i<28;i++)
        printf(i<hb ? C_CYN "#" C_RST : C_WHT "-" C_RST);
    printf(C_CYN "] %.0f%%\n" C_RST, w->humidity_now);
}

// ── Schermata legenda ─────────────────────────────────────────────────────
static void draw_legend(void) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_LEGEND_TITLE), NULL);
    printf("\n");
    printf(C_YLW " %s\n" C_RST, T(STR_LEG_SUNNY));
    printf(C_YLW " %s\n" C_RST, T(STR_LEG_PCLOUDY));
    printf(C_WHT " %s\n" C_RST, T(STR_LEG_CLOUDY));
    printf(C_WHT " %s\n" C_RST, T(STR_LEG_FOG));
    printf(C_CYN " %s\n" C_RST, T(STR_LEG_DRIZZLE));
    printf(C_BLU " %s\n" C_RST, T(STR_LEG_RAIN));
    printf(C_WHT " %s\n" C_RST, T(STR_LEG_SNOW));
    printf(C_RED " %s\n" C_RST, T(STR_LEG_STORM));
    printf(C_WHT " %s\n" C_RST, T(STR_LEG_UNKNOWN));
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT " B: back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("WMO CODES");
    printf(C_WHT " 0      " C_YLW "Clear sky\n" C_RST);
    printf(C_WHT " 1-2    " C_YLW "Partly cloudy\n" C_RST);
    printf(C_WHT " 3      " C_WHT "Overcast\n" C_RST);
    printf(C_WHT " 45,48  " C_WHT "Fog\n" C_RST);
    printf(C_WHT " 51-55  " C_CYN "Drizzle\n" C_RST);
    printf(C_WHT " 61-65  " C_BLU "Rain\n" C_RST);
    printf(C_WHT " 71-77  " C_WHT "Snow\n" C_RST);
    printf(C_WHT " 80-82  " C_BLU "Showers\n" C_RST);
    printf(C_WHT " 85-86  " C_WHT "Snow showers\n" C_RST);
    printf(C_WHT " 95     " C_RED "Thunderstorm\n" C_RST);
    printf(C_WHT " 96,99  " C_RED "Storm+hail\n" C_RST);
}

// ── Schermata lingua ──────────────────────────────────────────────────────
static void draw_language(int sel) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_LANG_TITLE), NULL);
    printf("\n");
    for (int i = 0; i < LANG_COUNT; i++) {
        if (i == sel)
            printf(C_GRN C_BLD " > %s%s\n" C_RST,
                   lang_name(i),
                   (i == (int)currentLang) ? " (*)" : "");
        else
            printf(C_WHT "   %s%s\n" C_RST,
                   lang_name(i),
                   (i == (int)currentLang) ? " (*)" : "");
    }
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT "%s\n" C_RST, T(STR_SELECT_LANG));

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("LANGUAGE / LINGUA");
    printf(C_WHT "\n UP/DOWN: navigate\n" C_RST);
    printf(C_WHT " A:       select\n" C_RST);
    printf(C_WHT " B:       back\n" C_RST);
    printf(C_CYN "\n (*) = current language\n" C_RST);
}

// ── Schermata riordina ────────────────────────────────────────────────────
static void draw_reorder(const City *c, int n, int sel, int moving) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top(T(STR_REORDER_TITLE), NULL);
    printf(C_WHT "%s\n" C_RST, T(STR_MOVE_HINT));
    printf(C_CYN "--------------------------------\n" C_RST);
    for (int i = 0; i < n; i++) {
        if (i == sel && moving)
            printf(C_YLW C_BLD " >> %s\n" C_RST, c[i].name);
        else if (i == sel)
            printf(C_GRN C_BLD " >  %s\n" C_RST, c[i].name);
        else
            printf(C_WHT "    %s\n" C_RST, c[i].name);
    }
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " A: %s  B: save & exit\n" C_RST,
           moving ? "confirm pos." : "start move");

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("REORDER CITIES");
    printf(C_WHT "\n A:       start / confirm move\n" C_RST);
    printf(C_WHT " UP/DOWN: move city\n" C_RST);
    printf(C_WHT " B:       save and exit\n" C_RST);
}

// ── Schermata selezione confronto ─────────────────────────────────────────
static void draw_compare_sel(const City *c, int n,
                              int sel1, int sel2, int step) {
    consoleSelect(&topScreen);
    consoleClear();
    draw_header_top("COMPARE CITIES", NULL);
    printf(C_WHT "\n Step %d/2: select city %d\n\n" C_RST, step, step);
    for (int i = 0; i < n; i++) {
        bool is_cursor = (step == 1) ? (i == sel1) : (i == sel2);
        bool is_locked = (step == 2 && i == sel1);
        if (is_cursor)
            printf(C_GRN C_BLD " > %s\n" C_RST, c[i].name);
        else if (is_locked)
            printf(C_CYN "   %s [1]\n" C_RST, c[i].name);
        else
            printf(C_WHT "   %s\n" C_RST, c[i].name);
    }
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT " A: confirm  B: cancel\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    draw_header_bot("COMPARE");
    printf(C_WHT "\n Select two cities\n" C_RST);
    printf(C_WHT " to compare.\n\n" C_RST);
    if (step == 2)
        printf(C_CYN " City 1: %s\n\n" C_RST, c[sel1].name);
    printf(C_WHT " UP/DOWN: navigate\n" C_RST);
    printf(C_WHT " A:       select\n" C_RST);
    printf(C_WHT " B:       cancel\n" C_RST);
}

// ── Schermata confronto ───────────────────────────────────────────────────
static void draw_compare(const WeatherData *w1, const WeatherData *w2,
                          const char *c1, const char *c2) {
    consoleSelect(&topScreen);
    consoleClear();
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_GRN C_BLD " [1] %s\n" C_RST, c1);
    printf(C_CYN "================================\n" C_RST);
    printf(C_YLW "\n %s %s\n\n" C_RST,
           weather_code_icon(w1->weather_code_now),
           weather_code_desc(w1->weather_code_now));
    printf(C_WHT " Temp:    " C_YLW "%.1fC\n" C_RST, w1->temp_now);
    printf(C_WHT " Feels:   " C_YLW "%.1fC\n" C_RST, w1->feels_like_now);
    printf(C_WHT " Humidity:" C_CYN "%.0f%%\n" C_RST, w1->humidity_now);
    printf(C_WHT " Pressure:" C_GRN "%.0fhPa\n" C_RST, w1->pressure_now);
    printf(C_WHT " Wind:    " C_MAG "%.1fkm/h %s\n" C_RST,
           w1->wind_now, wind_dir_str(w1->wind_dir_now));
    printf(C_WHT " UV:      " C_RED "%.1f\n" C_RST, w1->uv_index);
    printf(C_WHT " Sunrise: " C_YLW "%02d:%02d\n" C_RST,
           w1->sunrise_hour, w1->sunrise_min);
    printf(C_WHT " Sunset:  " C_YLW "%02d:%02d\n" C_RST,
           w1->sunset_hour,  w1->sunset_min);
    printf(C_CYN "--------------------------------\n" C_RST);
    float dt = w1->temp_now - w2->temp_now;
    if      (dt >  0.05f) printf(C_RED " +%.1fC vs %s\n" C_RST, dt,  c2);
    else if (dt < -0.05f) printf(C_BLU " %.1fC vs %s\n"  C_RST, dt,  c2);
    else                   printf(C_GRN " Same temp as %s\n" C_RST,   c2);
    printf(C_WHT " B: back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_YLW C_BLD " [2] %s\n" C_RST, c2);
    printf(C_CYN "================================\n" C_RST);
    printf(C_YLW "\n %s %s\n\n" C_RST,
           weather_code_icon(w2->weather_code_now),
           weather_code_desc(w2->weather_code_now));
    printf(C_WHT " Temp:    " C_YLW "%.1fC\n" C_RST, w2->temp_now);
    printf(C_WHT " Feels:   " C_YLW "%.1fC\n" C_RST, w2->feels_like_now);
    printf(C_WHT " Humidity:" C_CYN "%.0f%%\n" C_RST, w2->humidity_now);
    printf(C_WHT " Pressure:" C_GRN "%.0fhPa\n" C_RST, w2->pressure_now);
    printf(C_WHT " Wind:    " C_MAG "%.1fkm/h %s\n" C_RST,
           w2->wind_now, wind_dir_str(w2->wind_dir_now));
    printf(C_WHT " UV:      " C_RED "%.1f\n" C_RST, w2->uv_index);
    printf(C_WHT " Sunrise: " C_YLW "%02d:%02d\n" C_RST,
           w2->sunrise_hour, w2->sunrise_min);
    printf(C_WHT " Sunset:  " C_YLW "%02d:%02d\n" C_RST,
           w2->sunset_hour,  w2->sunset_min);
    printf(C_CYN "--------------------------------\n" C_RST);
    float dt2 = w2->temp_now - w1->temp_now;
    if      (dt2 >  0.05f) printf(C_RED " +%.1fC vs %s\n" C_RST, dt2, c1);
    else if (dt2 < -0.05f) printf(C_BLU " %.1fC vs %s\n"  C_RST, dt2, c1);
    else                    printf(C_GRN " Same temp as %s\n" C_RST,   c1);
}

// ── Schermata crediti ─────────────────────────────────────────────────────
static void draw_credits(void) {
    consoleSelect(&topScreen);
    consoleClear();
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_YLW C_BLD "       3DS WEATHER\n" C_RST);
    printf(C_CYN "================================\n" C_RST);
    printf(C_WHT "\n Developer:\n\n" C_RST);
    printf(C_GRN C_BLD "   Dzhmelyk135\n\n" C_RST);
    printf(C_WHT " Repository:\n" C_RST);
    printf(C_CYN "   github.com/\n" C_RST);
    printf(C_CYN "   Dzhmelyk135/\n" C_RST);
    printf(C_CYN "   Luma3DSWeather\n\n" C_RST);
    printf(C_WHT " Weather data:\n" C_RST);
    printf(C_CYN "   Open-Meteo.com\n\n" C_RST);
    printf(C_WHT " Geocoding:\n" C_RST);
    printf(C_CYN "   geocoding-api.\n" C_RST);
    printf(C_CYN "   open-meteo.com\n" C_RST);
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT " B: back\n" C_RST);

    consoleSelect(&botScreen);
    consoleClear();
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_MAG C_BLD "       THANKS TO\n" C_RST);
    printf(C_CYN "================================\n" C_RST);
    printf(C_WHT "\n Libraries:\n\n" C_RST);
    printf(C_YLW "   libctru " C_WHT "(devkitPro)\n\n" C_RST);
    printf(C_YLW "   jsmn    " C_WHT "(Serge Zaitsev)\n\n" C_RST);
    printf(C_WHT " Tools:\n" C_RST);
    printf(C_YLW "   devkitARM\n" C_RST);
    printf(C_YLW "   bannertool\n" C_RST);
    printf(C_YLW "   makerom\n\n" C_RST);
    printf(C_CYN " Open-Meteo is free\n" C_RST);
    printf(C_CYN " and open source!\n" C_RST);
}

// ── Tastiera software ─────────────────────────────────────────────────────
static bool get_kb(char *out, int maxlen,
                   const char *hint,
                   const char *btnCancel,
                   const char *btnOk) {
    SwkbdState kb;
    swkbdInit(&kb, SWKBD_TYPE_NORMAL, 2, maxlen - 1);
    swkbdSetHintText(&kb, hint);
    swkbdSetButton(&kb, SWKBD_BUTTON_LEFT,  btnCancel, false);
    swkbdSetButton(&kb, SWKBD_BUTTON_RIGHT, btnOk,     true);
    SwkbdButton btn = swkbdInputText(&kb, out, maxlen);
    return (btn == SWKBD_BUTTON_CONFIRM || btn == SWKBD_BUTTON_RIGHT);
}

// ── Avviso WiFi ───────────────────────────────────────────────────────────
static void show_wifi_error(int code) {
    consoleSelect(&topScreen);
    consoleClear();
    consoleSelect(&botScreen);
    consoleClear();
    consoleSelect(&topScreen);
    printf(C_CYN C_BLD "\n================================\n" C_RST);
    printf(C_RED C_BLD "      WiFi ERROR\n" C_RST);
    printf(C_CYN "================================\n\n" C_RST);
    printf(C_RED " Cannot connect to internet.\n\n" C_RST);
    printf(C_WHT " Error code: " C_YLW "%d\n\n" C_RST, code);
    printf(C_WHT " Please check:\n" C_RST);
    printf(C_WHT "  - WiFi is enabled\n" C_RST);
    printf(C_WHT "  - You are in range\n" C_RST);
    printf(C_WHT "  - Internet connection\n" C_RST);
    printf(C_WHT "    works on 3DS\n\n" C_RST);
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT " B: back\n" C_RST);

    consoleSelect(&botScreen);
    printf(C_CYN C_BLD "\n================================\n" C_RST);
    printf(C_YLW C_BLD "       TIPS\n" C_RST);
    printf(C_CYN "================================\n\n" C_RST);
    printf(C_WHT " 1. Go to HOME > System\n" C_RST);
    printf(C_WHT "    Settings > Internet\n" C_RST);
    printf(C_WHT "    Settings and connect.\n\n" C_RST);
    printf(C_WHT " 2. Make sure the WiFi\n" C_RST);
    printf(C_WHT "    switch is ON.\n\n" C_RST);
    printf(C_WHT " 3. Try again after\n" C_RST);
    printf(C_WHT "    connecting.\n" C_RST);
}

// ── Main ──────────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP,    true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    consoleInit(GFX_TOP,    &topScreen);
    consoleInit(GFX_BOTTOM, &botScreen);

    acInit();
    cfguInit();

    u32 *socBuf = (u32*)memalign(0x1000, 0x100000);
    socInit(socBuf, 0x100000);
    httpcInit(0x100000);

    mkdir("/3ds/3ds-weather", 0777);
    lang_load();  // imposta EN se primo avvio

    City cities[MAX_CITIES];
    int  cityCount = 0;
    cities_load(cities, &cityCount);

    Screen      screen        = SCR_CITY_LIST;
    int         selCity       = 0;
    int         selLang       = (int)currentLang;
    int         hourOff       = 0;
    int         reorderSel    = 0;
    bool        reorderMoving = false;
    bool        redraw        = true;
    int         menuSel       = 0;
    WeatherData wdata, wdata2;
    memset(&wdata,  0, sizeof(wdata));
    memset(&wdata2, 0, sizeof(wdata2));

    int  cmpStep = 1;
    int  cmpSel1 = 0;
    int  cmpSel2 = 1;
    int  cmpNav  = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        switch (screen) {

        // ── Lista citta' ───────────────────────────────────────────────
        case SCR_CITY_LIST:
            if (kDown & KEY_DOWN) {
                if (cityCount > 0) selCity = (selCity+1) % cityCount;
                redraw = true;
            }
            if (kDown & KEY_UP) {
                if (cityCount > 0)
                    selCity = (selCity-1+cityCount) % cityCount;
                redraw = true;
            }
            if ((kDown & KEY_A) && cityCount > 0) {
                consoleSelect(&topScreen); consoleClear();
                consoleSelect(&botScreen); consoleClear();
                consoleSelect(&topScreen);
                printf(C_YLW "\n\n %s\n " C_BLD "%s" C_RST
                       C_YLW "...\n" C_RST,
                       T(STR_DOWNLOADING), cities[selCity].name);
                gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();

                int ret = weather_fetch(
                              cities[selCity].lat,
                              cities[selCity].lon,
                              cities[selCity].timezone,
                              &wdata);
                if (ret == 0) {
                    screen = SCR_CURRENT;
                    draw_current(&wdata, cities[selCity].name);
                    redraw = false;
                } else {
                    show_wifi_error(ret);
                    gfxFlushBuffers(); gfxSwapBuffers();
                    while (aptMainLoop()) {
                        hidScanInput();
                        if (hidKeysDown() & KEY_B) break;
                        gfxFlushBuffers(); gfxSwapBuffers();
                        gspWaitForVBlank();
                    }
                    redraw = true;
                }
            }
            if (kDown & KEY_X) {
                char inp[48]="", found[48]="", tz[40]="Europe/Rome";
                float la=0, lo=0;
                bool ok = get_kb(inp, sizeof(inp),
                                 T(STR_SEARCH_HINT),
                                 T(STR_CANCEL), T(STR_SEARCH));
                if (ok && strlen(inp) >= 2) {
                    consoleSelect(&topScreen); consoleClear();
                    consoleSelect(&botScreen); consoleClear();
                    consoleSelect(&topScreen);
                    printf(C_YLW "\n %s " C_BLD "%s" C_RST "...\n",
                           T(STR_SEARCHING), inp);
                    gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
                    int ret = weather_geocode(inp,&la,&lo,found,tz);
                    if (ret == 0) {
                        cities_add(cities,&cityCount,found,la,lo,tz);
                        cities_save(cities, cityCount);
                        printf(C_GRN "\n %s: %s\n" C_RST,
                               T(STR_CITY_ADDED), found);
                        printf(C_WHT " %.4f, %.4f\n" C_RST, la, lo);
                    } else {
                        show_wifi_error(ret);
                        gfxFlushBuffers(); gfxSwapBuffers();
                        while (aptMainLoop()) {
                            hidScanInput();
                            if (hidKeysDown() & KEY_B) break;
                            gfxFlushBuffers(); gfxSwapBuffers();
                            gspWaitForVBlank();
                        }
                    }
                    for (int f=0;f<60;f++) {
                        gfxFlushBuffers(); gfxSwapBuffers();
                        gspWaitForVBlank();
                    }
                }
                redraw = true;
            }
            if ((kDown & KEY_Y) && cityCount > 1) {
                cities_remove(cities, &cityCount, selCity);
                cities_save(cities, cityCount);
                if (selCity >= cityCount) selCity = cityCount-1;
                redraw = true;
            }
            // L = riordina
            if ((kDown & KEY_L) && cityCount > 1) {
                reorderSel = selCity;
                reorderMoving = false;
                screen = SCR_REORDER;
                draw_reorder(cities, cityCount, reorderSel, reorderMoving);
                redraw = false;
            }
            // R = confronta
            if ((kDown & KEY_R) && cityCount >= 2) {
                cmpStep = 1; cmpSel1 = 0; cmpSel2 = 1; cmpNav = 0;
                screen = SCR_COMPARE_SEL;
                draw_compare_sel(cities, cityCount,
                                 cmpSel1, cmpSel2, cmpStep);
                redraw = false;
            }
            if (kDown & KEY_SELECT) {
                menuSel = 0;
                screen = SCR_MENU;
                draw_menu(menuSel);
                redraw = false;
            }
            if (redraw) {
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            }
            break;

        // ── Menu ──────────────────────────────────────────────────────
        case SCR_MENU:
            if (kDown & KEY_DOWN) {
                menuSel = (menuSel+1) % MENU_COUNT;
                draw_menu(menuSel);
            } else if (kDown & KEY_UP) {
                menuSel = (menuSel-1+MENU_COUNT) % MENU_COUNT;
                draw_menu(menuSel);
            } else if (kDown & KEY_A) {
                switch (menuSel) {
                case MENU_LANGUAGE:
                    selLang = (int)currentLang;
                    screen = SCR_LANGUAGE;
                    draw_language(selLang);
                    break;
                case MENU_LEGEND:
                    screen = SCR_LEGEND;
                    draw_legend();
                    break;
                case MENU_COMPARE:
                    if (cityCount >= 2) {
                        cmpStep = 1; cmpSel1 = 0;
                        cmpSel2 = 1; cmpNav  = 0;
                        screen = SCR_COMPARE_SEL;
                        draw_compare_sel(cities, cityCount,
                                         cmpSel1, cmpSel2, cmpStep);
                    } else {
                        consoleSelect(&topScreen); consoleClear();
                        printf(C_RED "\n Need at least 2 cities!\n" C_RST);
                        printf(C_WHT " B: back\n" C_RST);
                        gfxFlushBuffers(); gfxSwapBuffers();
                        while (aptMainLoop()) {
                            hidScanInput();
                            if (hidKeysDown() & KEY_B) break;
                            gfxFlushBuffers(); gfxSwapBuffers();
                            gspWaitForVBlank();
                        }
                        draw_menu(menuSel);
                    }
                    break;
                case MENU_CREDITS:
                    screen = SCR_CREDITS;
                    draw_credits();
                    break;
                }
                redraw = false;
            } else if ((kDown & KEY_B) || (kDown & KEY_SELECT)) {
                screen = SCR_CITY_LIST;
                redraw = true;
            } else if (redraw) {
                draw_menu(menuSel);
                redraw = false;
            }
            break;

        // ── Meteo attuale ──────────────────────────────────────────────
        case SCR_CURRENT:
            if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_L) {
                hourOff = 0; screen = SCR_HOURLY;
                draw_hourly(&wdata, cities[selCity].name, hourOff);
                redraw = false;
            } else if (kDown & KEY_R) {
                screen = SCR_DAILY;
                draw_daily(&wdata, cities[selCity].name);
                redraw = false;
            } else if (kDown & KEY_X) {
                screen = SCR_DETAILS;
                draw_details(&wdata, cities[selCity].name);
                redraw = false;
            } else if (redraw) {
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            }
            break;

        // ── Oraria ────────────────────────────────────────────────────
        case SCR_HOURLY:
            if (kDown & KEY_B) {
                screen = SCR_CURRENT;
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            } else if (kDown & KEY_R) {
                screen = SCR_DAILY;
                draw_daily(&wdata, cities[selCity].name);
                redraw = false;
            } else if ((kDown & KEY_DOWN) && hourOff < HOURLY_COUNT-12) {
                hourOff++;
                draw_hourly(&wdata, cities[selCity].name, hourOff);
            } else if ((kDown & KEY_UP) && hourOff > 0) {
                hourOff--;
                draw_hourly(&wdata, cities[selCity].name, hourOff);
            } else if (redraw) {
                draw_hourly(&wdata, cities[selCity].name, hourOff);
                redraw = false;
            }
            break;

        // ── Giornaliera ───────────────────────────────────────────────
        case SCR_DAILY:
            if (kDown & KEY_B) {
                screen = SCR_CURRENT;
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            } else if (kDown & KEY_L) {
                hourOff = 0; screen = SCR_HOURLY;
                draw_hourly(&wdata, cities[selCity].name, hourOff);
                redraw = false;
            } else if (kDown & KEY_X) {
                screen = SCR_DETAILS;
                draw_details(&wdata, cities[selCity].name);
                redraw = false;
            } else if (redraw) {
                draw_daily(&wdata, cities[selCity].name);
                redraw = false;
            }
            break;

        // ── Dettagli ──────────────────────────────────────────────────
        case SCR_DETAILS:
            if (kDown & KEY_B) {
                screen = SCR_CURRENT;
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            } else if (redraw) {
                draw_details(&wdata, cities[selCity].name);
                redraw = false;
            }
            break;

        // ── Legenda ───────────────────────────────────────────────────
        case SCR_LEGEND:
            if (kDown & KEY_B) {
                screen = SCR_MENU;
                draw_menu(menuSel);
                redraw = false;
            } else if (redraw) {
                draw_legend();
                redraw = false;
            }
            break;

        // ── Lingua ────────────────────────────────────────────────────
        case SCR_LANGUAGE:
            if (kDown & KEY_DOWN) {
                selLang = (selLang+1) % LANG_COUNT;
                draw_language(selLang);
            } else if (kDown & KEY_UP) {
                selLang = (selLang-1+LANG_COUNT) % LANG_COUNT;
                draw_language(selLang);
            } else if (kDown & KEY_A) {
                lang_set((LangID)selLang); lang_save();
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_B) {
                screen = SCR_MENU;
                draw_menu(menuSel);
                redraw = false;
            } else if (redraw) {
                draw_language(selLang);
                redraw = false;
            }
            break;

        // ── Riordina ──────────────────────────────────────────────────
        case SCR_REORDER:
            if ((kDown & KEY_B) && !reorderMoving) {
                cities_save(cities, cityCount);
                screen = SCR_CITY_LIST;
                selCity = reorderSel;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_A) {
                reorderMoving = !reorderMoving;
                draw_reorder(cities, cityCount, reorderSel, reorderMoving);
            } else if (kDown & KEY_DOWN) {
                if (reorderMoving && reorderSel < cityCount-1) {
                    cities_swap(cities, reorderSel, reorderSel+1);
                    reorderSel++;
                } else if (!reorderMoving && reorderSel < cityCount-1) {
                    reorderSel++;
                }
                draw_reorder(cities, cityCount, reorderSel, reorderMoving);
            } else if (kDown & KEY_UP) {
                if (reorderMoving && reorderSel > 0) {
                    cities_swap(cities, reorderSel, reorderSel-1);
                    reorderSel--;
                } else if (!reorderMoving && reorderSel > 0) {
                    reorderSel--;
                }
                draw_reorder(cities, cityCount, reorderSel, reorderMoving);
            } else if (redraw) {
                draw_reorder(cities, cityCount, reorderSel, reorderMoving);
                redraw = false;
            }
            break;

        // ── Selezione confronto ───────────────────────────────────────
        case SCR_COMPARE_SEL:
            if (kDown & KEY_DOWN) {
                cmpNav = (cmpNav+1) % cityCount;
                draw_compare_sel(cities, cityCount,
                    cmpStep==1 ? cmpNav : cmpSel1,
                    cmpStep==2 ? cmpNav : cmpSel2,
                    cmpStep);
            } else if (kDown & KEY_UP) {
                cmpNav = (cmpNav-1+cityCount) % cityCount;
                draw_compare_sel(cities, cityCount,
                    cmpStep==1 ? cmpNav : cmpSel1,
                    cmpStep==2 ? cmpNav : cmpSel2,
                    cmpStep);
            } else if (kDown & KEY_A) {
                if (cmpStep == 1) {
                    cmpSel1 = cmpNav;
                    cmpStep = 2;
                    cmpNav  = (cmpSel1 == 0) ? 1 : 0;
                    draw_compare_sel(cities, cityCount,
                                     cmpSel1, cmpNav, cmpStep);
                } else {
                    if (cmpNav == cmpSel1)
                        cmpNav = (cmpNav+1) % cityCount;
                    cmpSel2 = cmpNav;

                    consoleSelect(&topScreen); consoleClear();
                    consoleSelect(&botScreen); consoleClear();
                    consoleSelect(&topScreen);
                    printf(C_YLW "\n Downloading %s...\n" C_RST,
                           cities[cmpSel1].name);
                    gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();

                    int r1 = weather_fetch(
                                 cities[cmpSel1].lat,
                                 cities[cmpSel1].lon,
                                 cities[cmpSel1].timezone,
                                 &wdata);

                    if (r1 != 0) {
                        show_wifi_error(r1);
                        gfxFlushBuffers(); gfxSwapBuffers();
                        while (aptMainLoop()) {
                            hidScanInput();
                            if (hidKeysDown() & KEY_B) break;
                            gfxFlushBuffers(); gfxSwapBuffers();
                            gspWaitForVBlank();
                        }
                        screen = SCR_CITY_LIST;
                        redraw = true;
                        break;
                    }

                    printf(C_YLW " Downloading %s...\n" C_RST,
                           cities[cmpSel2].name);
                    gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();

                    int r2 = weather_fetch(
                                 cities[cmpSel2].lat,
                                 cities[cmpSel2].lon,
                                 cities[cmpSel2].timezone,
                                 &wdata2);

                    if (r2 != 0) {
                        show_wifi_error(r2);
                        gfxFlushBuffers(); gfxSwapBuffers();
                        while (aptMainLoop()) {
                            hidScanInput();
                            if (hidKeysDown() & KEY_B) break;
                            gfxFlushBuffers(); gfxSwapBuffers();
                            gspWaitForVBlank();
                        }
                        screen = SCR_CITY_LIST;
                        redraw = true;
                        break;
                    }

                    screen = SCR_COMPARE;
                    draw_compare(&wdata, &wdata2,
                                 cities[cmpSel1].name,
                                 cities[cmpSel2].name);
                    redraw = false;
                }
            } else if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (redraw) {
                draw_compare_sel(cities, cityCount,
                    cmpStep==1 ? cmpNav : cmpSel1,
                    cmpStep==2 ? cmpNav : cmpSel2,
                    cmpStep);
                redraw = false;
            }
            break;

        // ── Confronto ─────────────────────────────────────────────────
        case SCR_COMPARE:
            if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (redraw) {
                draw_compare(&wdata, &wdata2,
                             cities[cmpSel1].name,
                             cities[cmpSel2].name);
                redraw = false;
            }
            break;

        // ── Crediti ───────────────────────────────────────────────────
        case SCR_CREDITS:
            if (kDown & KEY_B) {
                screen = SCR_MENU;
                draw_menu(menuSel);
                redraw = false;
            } else if (redraw) {
                draw_credits();
                redraw = false;
            }
            break;

        default: break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    httpcExit();
    socExit();
    free(socBuf);
    cfguExit();
    acExit();
    gfxExit();
    return 0;
}