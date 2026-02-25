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
typedef enum {
    SCR_CITY_LIST,
    SCR_CURRENT,
    SCR_HOURLY,
    SCR_DAILY,
    SCR_DETAILS,
    SCR_LEGEND,
    SCR_LANGUAGE,
    SCR_REORDER,
} Screen;

static void lang_save(void) {
    FILE *f = fopen(LANG_FILE, "w");
    if (!f) return;
    fprintf(f, "%d\n", (int)currentLang);
    fclose(f);
}

static void lang_load(void) {
    FILE *f = fopen(LANG_FILE, "r");
    if (!f) return;
    int id = 0;
    fscanf(f, "%d", &id);
    fclose(f);
    lang_set((LangID)id);
}

static const char *wind_dir_str(int deg) {
    const char *d[] = {"N","NE","E","SE","S","SO","O","NO"};
    return d[((deg + 22) % 360) / 45];
}

static void draw_header(const char *title, const char *sub) {
    printf(C_CYN C_BLD "================================\n" C_RST);
    printf(C_YLW C_BLD " %s\n" C_RST, title);
    if (sub && sub[0])
        printf(C_WHT " %s\n" C_RST, sub);
    printf(C_CYN "================================\n" C_RST);
}

static void draw_city_list(const City *c, int n, int sel) {
    consoleClear();
    draw_header(T(STR_APP_TITLE), T(STR_CITY_LIST_TITLE));
    printf(C_WHT "%s  %s\n%s  %s\n" C_RST,
           T(STR_NAV_HINT),   T(STR_ADD_HINT),
           T(STR_DEL_HINT),   T(STR_REORDER_HINT));
    printf(C_CYN "--------------------------------\n" C_RST);
    if (n == 0) {
        printf(C_RED "%s\n%s\n" C_RST,
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
    printf(C_WHT " L: lingua  | R: legenda\n" C_RST);
    printf(C_WHT " A: meteo   | START: esci\n" C_RST);
    printf(C_WHT "%s\n" C_RST, T(STR_POWERED_BY));
}

static void draw_current(const WeatherData *w, const char *city) {
    consoleClear();
    draw_header(T(STR_CURRENT_TITLE), city);
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
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_SUNRISE), w->sunrise_hour, w->sunrise_min);
    printf(C_WHT "%s" C_YLW "%02d:%02d\n" C_RST,
           T(STR_SUNSET),  w->sunset_hour,  w->sunset_min);
    printf(C_WHT "%s" C_RED "%.1f\n" C_RST,
           T(STR_UV), w->uv_index);
    printf(C_CYN "\n--------------------------------\n" C_RST);
    printf(C_WHT "%s | %s\n%s | %s\n" C_RST,
           T(STR_NAV_L_HOURLY), T(STR_NAV_R_DAILY),
           T(STR_NAV_X_DETAILS), T(STR_BACK));
}

static void draw_hourly(const WeatherData *w,
                        const char *city, int off) {
    consoleClear();
    draw_header(T(STR_HOURLY_TITLE), city);
    printf(C_CYN "%s\n" C_RST, T(STR_HOUR_HDR));
    printf(C_CYN "--------------------------------\n" C_RST);
    int shown = 0;
    for (int i = off; i < HOURLY_COUNT && shown < 16; i++, shown++) {
        if (i == w->current_hour)
            printf(C_YLW C_BLD " %02d:00 %5.1fC %4.1fmm %3.0f%% %s <\n" C_RST,
                   i,
                   w->hourly_temp[i],
                   w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
        else
            printf(C_WHT " %02d:00" C_YLW " %5.1fC"
                   C_CYN " %4.1fmm" C_BLU " %3.0f%%"
                   C_GRN " %s\n" C_RST,
                   i,
                   w->hourly_temp[i],
                   w->hourly_precip[i],
                   w->hourly_humidity[i],
                   weather_code_icon(w->hourly_code[i]));
    }
    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT "%s | %s\n" C_RST,
           T(STR_NAV_SCROLL), T(STR_NAV_BACK));
}
static void draw_daily(const WeatherData *w, const char *city) {
    consoleClear();
    draw_header(T(STR_DAILY_TITLE), city);
    printf(C_CYN "%s\n" C_RST, T(STR_DAILY_HDR));
    printf(C_CYN "----------------------------------\n" C_RST);
    for (int i = 0; i < FORECAST_DAYS; i++) {
        printf(i==0 ? C_YLW C_BLD : C_WHT);
        printf(" %s %4.0fC %4.0fC %4.1fmm %4.0fkm %s\n" C_RST,
               w->daily_date[i],
               w->daily_max[i], w->daily_min[i],
               w->daily_precip[i], w->daily_wind_max[i],
               weather_code_icon(w->daily_code[i]));
    }
    printf(C_CYN "----------------------------------\n" C_RST);
    printf(C_WHT "%s | %s | %s\n" C_RST,
           T(STR_BACK), T(STR_NAV_L_HOURLY), T(STR_NAV_X_DETAILS));
}

static void draw_details(const WeatherData *w, const char *city) {
    consoleClear();
    draw_header(T(STR_DETAILS_TITLE), city);
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
    printf("\n");

    int bars = (int)((w->pressure_now - 970.f) / 60.f * 20.f);
    bars = bars < 0 ? 0 : bars > 20 ? 20 : bars;
    printf(C_CYN " P:[");
    for (int i=0;i<20;i++)
        printf(i<bars ? C_GRN "#" C_RST : C_WHT "-" C_RST);
    printf(C_CYN "]\n" C_RST);

    int wb = (int)(w->wind_now / 120.f * 20.f);
    wb = wb > 20 ? 20 : wb;
    printf(C_MAG " W:[");
    for (int i=0;i<20;i++)
        printf(i<wb ? C_MAG "#" C_RST : C_WHT "-" C_RST);
    printf(C_MAG "]\n" C_RST);

    printf(C_CYN "--------------------------------\n" C_RST);
    printf(C_WHT "%s\n" C_RST, T(STR_BACK));
}

static void draw_legend(void) {
    consoleClear();
    draw_header(T(STR_LEGEND_TITLE), NULL);
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
    printf(C_WHT " %s\n" C_RST, T(STR_BACK));
}

static void draw_language(int sel) {
    consoleClear();
    draw_header(T(STR_LANG_TITLE), NULL);
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
}

static void draw_reorder(const City *c, int n, int sel, int moving) {
    consoleClear();
    draw_header(T(STR_REORDER_TITLE), NULL);
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
    printf(C_WHT " A: %s  B: %s\n" C_RST,
           moving ? "conferma pos." : "inizia spostamento",
           T(STR_NAV_BACK));
}

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
    return (btn == SWKBD_BUTTON_CONFIRM ||
            btn == SWKBD_BUTTON_RIGHT);
}

int main(int argc, char *argv[]) {
    gfxInitDefault();
    httpcInit(0);

    u32 *socBuf = (u32*)memalign(0x1000, 0x100000);
    socInit(socBuf, 0x100000);

    PrintConsole top;
    consoleInit(GFX_TOP, &top);

    mkdir("/3ds/3ds-weather", 0777);

    lang_load();

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
    WeatherData wdata;
    memset(&wdata, 0, sizeof(wdata));

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        switch (screen) {

        case SCR_CITY_LIST:
            if (kDown & KEY_DOWN) {
                if (cityCount > 0)
                    selCity = (selCity+1) % cityCount;
                redraw = true;
            }
            if (kDown & KEY_UP) {
                if (cityCount > 0)
                    selCity = (selCity-1+cityCount) % cityCount;
                redraw = true;
            }
            if (kDown & KEY_A && cityCount > 0) {
                consoleClear();
                printf(C_YLW "\n\n %s\n " C_BLD "%s" C_RST
                       C_YLW "...\n" C_RST,
                       T(STR_DOWNLOADING), cities[selCity].name);
                gfxFlushBuffers(); gfxSwapBuffers();
                gspWaitForVBlank();

                int ret = weather_fetch(cities[selCity].lat,
                                        cities[selCity].lon,
                                        cities[selCity].timezone,
                                        &wdata);
                if (ret == 0) {
                    screen = SCR_CURRENT;
                    draw_current(&wdata, cities[selCity].name);
                    redraw = false;
                } else {
                    printf(C_RED "\n %s (cod:%d)\n" C_RST,
                           T(STR_WIFI_ERR), ret);
                    printf(C_WHT " %s\n" C_RST, T(STR_PRESS_B));
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
                                 T(STR_CANCEL),
                                 T(STR_SEARCH));
                if (ok && strlen(inp) >= 2) {
                    consoleClear();
                    printf(C_YLW "\n %s " C_BLD "%s" C_RST "...\n",
                           T(STR_SEARCHING), inp);
                    gfxFlushBuffers(); gfxSwapBuffers();
                    gspWaitForVBlank();

                    int ret = weather_geocode(inp,&la,&lo,found,tz);
                    if (ret == 0) {
                        cities_add(cities,&cityCount,found,la,lo,tz);
                        cities_save(cities, cityCount);
                        printf(C_GRN "\n %s: %s\n" C_RST,
                               T(STR_CITY_ADDED), found);
                        printf(C_WHT " %.4f, %.4f\n" C_RST, la, lo);
                    } else {
                        printf(C_RED "\n %s\n%s\n" C_RST,
                               T(STR_CITY_NOT_FOUND),
                               T(STR_TRY_EN));
                    }
                    for (int f=0;f<150;f++) {
                        gfxFlushBuffers(); gfxSwapBuffers();
                        gspWaitForVBlank();
                    }
                }
                redraw = true;
            }
            if (kDown & KEY_Y && cityCount > 1) {
                cities_remove(cities, &cityCount, selCity);
                cities_save(cities, cityCount);
                if (selCity >= cityCount) selCity = cityCount-1;
                redraw = true;
            }
            if (kDown & KEY_SELECT && cityCount > 1) {
                reorderSel    = selCity;
                reorderMoving = false;
                screen = SCR_REORDER;
                draw_reorder(cities, cityCount,
                             reorderSel, reorderMoving);
                redraw = false;
            }
            if (kDown & KEY_L) {
                selLang = (int)currentLang;
                screen = SCR_LANGUAGE;
                draw_language(selLang);
                redraw = false;
            }
            if (kDown & KEY_R) {
                screen = SCR_LEGEND;
                draw_legend();
                redraw = false;
            }
            if (redraw) {
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            }
            break;

        case SCR_CURRENT:
            if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_L) {
                hourOff = 0;
                screen = SCR_HOURLY;
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

        case SCR_HOURLY:
            if (kDown & KEY_B) {
                screen = SCR_CURRENT;
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            } else if (kDown & KEY_R) {
                screen = SCR_DAILY;
                draw_daily(&wdata, cities[selCity].name);
                redraw = false;
            } else if ((kDown & KEY_DOWN) && hourOff < HOURLY_COUNT-16) {
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

        case SCR_DAILY:
            if (kDown & KEY_B) {
                screen = SCR_CURRENT;
                draw_current(&wdata, cities[selCity].name);
                redraw = false;
            } else if (kDown & KEY_L) {
                hourOff = 0;
                screen = SCR_HOURLY;
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

        case SCR_LEGEND:
            if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (redraw) {
                draw_legend();
                redraw = false;
            }
            break;

        case SCR_LANGUAGE:
            if (kDown & KEY_DOWN) {
                selLang = (selLang+1) % LANG_COUNT;
                draw_language(selLang);
            } else if (kDown & KEY_UP) {
                selLang = (selLang-1+LANG_COUNT) % LANG_COUNT;
                draw_language(selLang);
            } else if (kDown & KEY_A) {
                lang_set((LangID)selLang);
                lang_save();
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_B) {
                screen = SCR_CITY_LIST;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (redraw) {
                draw_language(selLang);
                redraw = false;
            }
            break;

        case SCR_REORDER:
            if (kDown & KEY_B && !reorderMoving) {
                cities_save(cities, cityCount);
                screen = SCR_CITY_LIST;
                selCity = reorderSel;
                draw_city_list(cities, cityCount, selCity);
                redraw = false;
            } else if (kDown & KEY_A) {
                reorderMoving = !reorderMoving;
                draw_reorder(cities, cityCount,
                             reorderSel, reorderMoving);
            } else if (kDown & KEY_DOWN) {
                if (reorderMoving && reorderSel < cityCount-1) {
                    cities_swap(cities, reorderSel, reorderSel+1);
                    reorderSel++;
                } else if (!reorderMoving && reorderSel < cityCount-1) {
                    reorderSel++;
                }
                draw_reorder(cities, cityCount,
                             reorderSel, reorderMoving);
            } else if (kDown & KEY_UP) {
                if (reorderMoving && reorderSel > 0) {
                    cities_swap(cities, reorderSel, reorderSel-1);
                    reorderSel--;
                } else if (!reorderMoving && reorderSel > 0) {
                    reorderSel--;
                }
                draw_reorder(cities, cityCount,
                             reorderSel, reorderMoving);
            } else if (redraw) {
                draw_reorder(cities, cityCount,
                             reorderSel, reorderMoving);
                redraw = false;
            }
            break;

        default: break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    socExit();
    httpcExit();
    free(socBuf);
    gfxExit();
    return 0;
}