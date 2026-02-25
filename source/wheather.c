#include "weather.h"
#include "jsmn.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HTTP_BUF_SIZE  (96 * 1024)
#define MAX_TOKENS     2048

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0)
        return 0;
    return -1;
}

static void tok2str(const char *json, jsmntok_t *tok,
                    char *out, int maxlen) {
    int len = tok->end - tok->start;
    if (len >= maxlen) len = maxlen - 1;
    strncpy(out, json + tok->start, len);
    out[len] = '\0';
}

static int http_get(const char *url, char *buf,
                    u32 bufsize, u32 *bytesRead) {
    httpcContext ctx;
    Result rc;

    rc = httpcOpenContext(&ctx, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(rc)) return -1;

    httpcSetSSLOpt(&ctx, SSLCOPT_DisableVerify);
    httpcSetKeepAlive(&ctx, HTTPC_KEEPALIVE_DISABLED);
    httpcAddRequestHeaderField(&ctx, "User-Agent",
                               "Mozilla/5.0 (Nintendo 3DS)");
    httpcAddRequestHeaderField(&ctx, "Accept",
                               "application/json");
    httpcAddRequestHeaderField(&ctx, "Accept-Encoding", "identity");
    httpcAddRequestHeaderField(&ctx, "Connection",      "close");

    rc = httpcBeginRequest(&ctx);
    if (R_FAILED(rc)) { httpcCloseContext(&ctx); return -2; }

    u32 statuscode = 0;
    httpcGetResponseStatusCode(&ctx, &statuscode);

    if (statuscode == 301 || statuscode == 302) {
        char newurl[512] = "";
        httpcGetResponseHeader(&ctx, "Location",
                               newurl, sizeof(newurl));
        httpcCloseContext(&ctx);
        if (newurl[0])
            return http_get(newurl, buf, bufsize, bytesRead);
        return -3;
    }
    if (statuscode != 200) {
        httpcCloseContext(&ctx);
        return -(int)statuscode;
    }

    *bytesRead = 0;
    u32 readSize = 0;
    u8 *ptr = (u8*)buf;
    u32 remaining = bufsize - 1;
    do {
        rc = httpcReceiveData(&ctx, ptr, remaining);
        httpcGetDownloadSizeState(&ctx, &readSize, NULL);
        ptr       = (u8*)buf + readSize;
        remaining = bufsize - 1 - readSize;
    } while (rc == (Result)HTTPC_RESULTCODE_DOWNLOADPENDING
             && remaining > 0);

    *bytesRead = readSize;
    buf[*bytesRead] = '\0';
    httpcCloseContext(&ctx);
    return (*bytesRead > 0) ? 0 : -4;
}

int weather_geocode(const char *city_name, float *lat, float *lon,
                    char *found_name, char *timezone) {
    char url[256];
    char *buf = (char*)malloc(HTTP_BUF_SIZE);
    if (!buf) return -1;

    char encoded[128] = {0};
    int ei = 0;
    for (int i = 0; city_name[i] && ei < 120; i++) {
        if      (city_name[i] == ' ')
            { encoded[ei++]='%'; encoded[ei++]='2'; encoded[ei++]='0'; }
        else if (city_name[i] == ',')
            { encoded[ei++]='%'; encoded[ei++]='2'; encoded[ei++]='C'; }
        else encoded[ei++] = city_name[i];
    }

    snprintf(url, sizeof(url),
        "http://geocoding-api.open-meteo.com/v1/search"
        "?name=%s&count=1&language=it&format=json", encoded);

    u32 bytesRead = 0;
    int ret = http_get(url, buf, HTTP_BUF_SIZE, &bytesRead);
    if (ret < 0) { free(buf); return ret; }

    jsmn_parser p;
    jsmntok_t *tok = (jsmntok_t*)malloc(MAX_TOKENS * sizeof(jsmntok_t));
    jsmn_init(&p);
    int r = jsmn_parse(&p, buf, bytesRead, tok, MAX_TOKENS);

    int found = 0;
    char val[64];
    for (int i = 0; i < r - 1; i++) {
        if (jsoneq(buf, &tok[i], "latitude") == 0
            && tok[i+1].type == JSMN_PRIMITIVE) {
            tok2str(buf, &tok[i+1], val, sizeof(val));
            *lat = strtof(val, NULL); found++;
        } else if (jsoneq(buf, &tok[i], "longitude") == 0
                   && tok[i+1].type == JSMN_PRIMITIVE) {
            tok2str(buf, &tok[i+1], val, sizeof(val));
            *lon = strtof(val, NULL); found++;
        } else if (jsoneq(buf, &tok[i], "name") == 0 && found_name) {
            tok2str(buf, &tok[i+1], found_name, 48);
        } else if (jsoneq(buf, &tok[i], "timezone") == 0 && timezone) {
            tok2str(buf, &tok[i+1], timezone, 40);
        }
    }
    free(tok); free(buf);
    return (found >= 2) ? 0 : -10;
}

int weather_fetch(float lat, float lon,
                  const char *timezone, WeatherData *out) {
    char url[512];
    char *buf = (char*)malloc(HTTP_BUF_SIZE);
    if (!buf) return -1;
    memset(out, 0, sizeof(WeatherData));

    char tz_enc[64] = {0};
    int ti = 0;
    for (int i = 0; timezone[i] && ti < 60; i++) {
        if (timezone[i] == '/')
            { tz_enc[ti++]='%'; tz_enc[ti++]='2'; tz_enc[ti++]='F'; }
        else tz_enc[ti++] = timezone[i];
    }

    snprintf(url, sizeof(url),
        "http://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,relative_humidity_2m,"
        "apparent_temperature,weather_code,wind_speed_10m,"
        "wind_direction_10m,surface_pressure,precipitation"
        "&hourly=temperature_2m,precipitation,"
        "relative_humidity_2m,weather_code"
        "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
        "precipitation_sum,wind_speed_10m_max,uv_index_max,"
        "sunrise,sunset"
        "&forecast_days=7&forecast_hours=24&timezone=%s",
        lat, lon, tz_enc);

    u32 bytesRead = 0;
    int ret = http_get(url, buf, HTTP_BUF_SIZE, &bytesRead);
    if (ret < 0) { free(buf); return ret; }

    jsmn_parser p;
    jsmntok_t *tok =
        (jsmntok_t*)malloc(MAX_TOKENS * sizeof(jsmntok_t));
    jsmn_init(&p);
    int r = jsmn_parse(&p, buf, bytesRead, tok, MAX_TOKENS);
    if (r < 0) {
        free(tok); free(buf); return -20;
    }

    char val[32];
    for (int i = 0; i < r - 1; i++) {
        if (tok[i].type != JSMN_STRING) continue;
        if (jsoneq(buf,&tok[i],"temperature_2m")==0
            && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->temp_now=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"relative_humidity_2m")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->humidity_now=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"apparent_temperature")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->feels_like_now=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"weather_code")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->weather_code_now=atoi(val);
        } else if (jsoneq(buf,&tok[i],"wind_speed_10m")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->wind_now=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"wind_direction_10m")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->wind_dir_now=atoi(val);
        } else if (jsoneq(buf,&tok[i],"surface_pressure")==0
                   && tok[i+1].type==JSMN_PRIMITIVE) {
            tok2str(buf,&tok[i+1],val,sizeof(val));
            out->pressure_now=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"uv_index_max")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            tok2str(buf,&tok[i+2],val,sizeof(val));
            out->uv_index=strtof(val,NULL);
        } else if (jsoneq(buf,&tok[i],"temperature_2m")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<HOURLY_COUNT&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->hourly_temp[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"precipitation")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<HOURLY_COUNT&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->hourly_precip[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"relative_humidity_2m")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<HOURLY_COUNT&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->hourly_humidity[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"weather_code")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<HOURLY_COUNT&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->hourly_code[j]=atoi(val);
            }
        } else if (jsoneq(buf,&tok[i],"time")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<FORECAST_DAYS&&j<tok[i+1].size;j++)
                tok2str(buf,&tok[i+2+j],out->daily_date[j],12);
        } else if (jsoneq(buf,&tok[i],"temperature_2m_max")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<FORECAST_DAYS&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->daily_max[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"temperature_2m_min")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<FORECAST_DAYS&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->daily_min[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"precipitation_sum")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<FORECAST_DAYS&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->daily_precip[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"wind_speed_10m_max")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            for (int j=0;j<FORECAST_DAYS&&j<tok[i+1].size;j++) {
                tok2str(buf,&tok[i+2+j],val,sizeof(val));
                out->daily_wind_max[j]=strtof(val,NULL);
            }
        } else if (jsoneq(buf,&tok[i],"sunrise")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            char sr[20];
            tok2str(buf,&tok[i+2],sr,sizeof(sr));
            if (strlen(sr)>=16) {
                out->sunrise_hour=atoi(sr+11);
                out->sunrise_min =atoi(sr+14);
            }
        } else if (jsoneq(buf,&tok[i],"sunset")==0
                   && tok[i+1].type==JSMN_ARRAY) {
            char ss[20];
            tok2str(buf,&tok[i+2],ss,sizeof(ss));
            if (strlen(ss)>=16) {
                out->sunset_hour=atoi(ss+11);
                out->sunset_min =atoi(ss+14);
            }
        }
    }
    free(tok); free(buf);
    out->valid = 1;
    return 0;
}

const char *weather_code_desc(int code) {
    switch(code) {
        case 0:  return "Sereno";
        case 1:  return "Quasi sereno";
        case 2:  return "Parz. nuvoloso";
        case 3:  return "Nuvoloso";
        case 45: return "Nebbia";
        case 48: return "Nebbia gelata";
        case 51: return "Pioggerella lieve";
        case 53: return "Pioggerella";
        case 55: return "Pioggerella forte";
        case 61: return "Pioggia lieve";
        case 63: return "Pioggia";
        case 65: return "Pioggia forte";
        case 71: return "Neve lieve";
        case 73: return "Neve";
        case 75: return "Neve intensa";
        case 77: return "Granelli neve";
        case 80: return "Rovescio lieve";
        case 81: return "Rovescio";
        case 82: return "Rovescio forte";
        case 85: return "Nevicata lieve";
        case 86: return "Nevicata forte";
        case 95: return "Temporale";
        case 96: return "Temp.+grandine";
        case 99: return "Temp.+grandine f.";
        default: return "Sconosciuto";
    }
}

const char *weather_code_icon(int code) {
    if (code == 0)               return "(*)";
    if (code <= 2)               return "(^)";
    if (code == 3)               return "(n)";
    if (code==45 || code==48)    return "~~~";
    if (code <= 55)              return "._.";
    if (code <= 65)              return ".|.";
    if (code <= 77)              return "***";
    if (code <= 82)              return ".|.";
    if (code <= 86)              return "***";
    if (code >= 95)              return "/!/";
    return "???";
}