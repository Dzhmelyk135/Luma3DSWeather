#ifndef WEATHER_H
#define WEATHER_H

#define FORECAST_DAYS  7
#define HOURLY_COUNT   24

typedef struct {
    float temp_now;
    float wind_now;
    int   wind_dir_now;
    float pressure_now;
    float humidity_now;
    float feels_like_now;
    int   weather_code_now;

    float hourly_temp[HOURLY_COUNT];
    float hourly_precip[HOURLY_COUNT];
    float hourly_humidity[HOURLY_COUNT];
    int   hourly_code[HOURLY_COUNT];

    char  daily_date[FORECAST_DAYS][12];
    float daily_max[FORECAST_DAYS];
    float daily_min[FORECAST_DAYS];
    float daily_precip[FORECAST_DAYS];
    float daily_wind_max[FORECAST_DAYS];
    int   daily_code[FORECAST_DAYS];

    float uv_index;
    int   sunrise_hour, sunrise_min;
    int   sunset_hour,  sunset_min;
    int   valid;
} WeatherData;

int         weather_fetch(float lat, float lon,
                          const char *timezone, WeatherData *out);
int         weather_geocode(const char *city_name, float *lat, float *lon,
                            char *found_name, char *timezone);
const char *weather_code_desc(int code);
const char *weather_code_icon(int code);

#endif