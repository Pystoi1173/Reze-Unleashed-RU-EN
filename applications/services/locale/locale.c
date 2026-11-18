#include "locale.h"

#include <toolbox/saved_struct.h>
#include <storage/storage.h>

#define TAG "LocaleSrv"
#define LOCALE_SETTINGS_PATH INT_PATH(".locale.settings")
#define LOCALE_SETTINGS_MAGIC 0x4C
#define LOCALE_SETTINGS_VERSION 1

static LocaleLanguage locale_language = LocaleLanguageRussian;

LocaleLanguage locale_get_language(void) {
    return locale_language;
}

void locale_set_language(LocaleLanguage language) {
    if(language != LocaleLanguageRussian && language != LocaleLanguageEnglish) return;

    locale_language = language;
    const uint8_t value = language;
    if(!saved_struct_save(
           LOCALE_SETTINGS_PATH,
           &value,
           sizeof(value),
           LOCALE_SETTINGS_MAGIC,
           LOCALE_SETTINGS_VERSION)) {
        FURI_LOG_W(TAG, "Language setting was not saved");
    }
}

const char* locale_translate(const char* english, const char* russian) {
    return (locale_get_language() == LocaleLanguageRussian && russian) ? russian : english;
}


LocaleMeasurementUnits locale_get_measurement_unit(void) {
    return (LocaleMeasurementUnits)furi_hal_rtc_get_locale_units();
}

void locale_set_measurement_unit(LocaleMeasurementUnits format) {
    furi_hal_rtc_set_locale_units((FuriHalRtcLocaleUnits)format);
}

LocaleTimeFormat locale_get_time_format(void) {
    return (LocaleTimeFormat)furi_hal_rtc_get_locale_timeformat();
}

void locale_set_time_format(LocaleTimeFormat format) {
    furi_hal_rtc_set_locale_timeformat((FuriHalRtcLocaleTimeFormat)format);
}

LocaleDateFormat locale_get_date_format(void) {
    return (LocaleDateFormat)furi_hal_rtc_get_locale_dateformat();
}

void locale_set_date_format(LocaleDateFormat format) {
    furi_hal_rtc_set_locale_dateformat((FuriHalRtcLocaleDateFormat)format);
}

float locale_fahrenheit_to_celsius(float temp_f) {
    return (temp_f - 32.f) / 1.8f;
}

float locale_celsius_to_fahrenheit(float temp_c) {
    return temp_c * 1.8f + 32.f;
}

void locale_format_time(
    FuriString* out_str,
    const DateTime* datetime,
    const LocaleTimeFormat format,
    const bool show_seconds) {
    furi_check(out_str);
    furi_check(datetime);

    uint8_t hours = datetime->hour;
    uint8_t am_pm = 0;
    if(format == LocaleTimeFormat12h) {
        if(hours >= 12) {
            if(hours > 12) hours -= 12;
            am_pm = 2;
        } else {
            am_pm = 1;
        }
        if(hours == 0) {
            hours = 12;
        }
    }

    if(show_seconds) {
        furi_string_printf(out_str, "%02u:%02u:%02u", hours, datetime->minute, datetime->second);
    } else {
        furi_string_printf(out_str, "%02u:%02u", hours, datetime->minute);
    }

    if(am_pm > 0) {
        furi_string_cat_printf(out_str, " %s", (am_pm == 1) ? ("AM") : ("PM"));
    }
}

void locale_format_date(
    FuriString* out_str,
    const DateTime* datetime,
    const LocaleDateFormat format,
    const char* separator) {
    furi_check(out_str);
    furi_check(datetime);
    furi_check(separator);

    if(format == LocaleDateFormatDMY) {
        furi_string_printf(
            out_str,
            "%02u%s%02u%s%04u",
            datetime->day,
            separator,
            datetime->month,
            separator,
            datetime->year);
    } else if(format == LocaleDateFormatMDY) {
        furi_string_printf(
            out_str,
            "%02u%s%02u%s%04u",
            datetime->month,
            separator,
            datetime->day,
            separator,
            datetime->year);
    } else {
        furi_string_printf(
            out_str,
            "%04u%s%02u%s%02u",
            datetime->year,
            separator,
            datetime->month,
            separator,
            datetime->day);
    }
}

int32_t locale_on_system_start(void* p) {
    UNUSED(p);
    // Хук вызывается загрузчиком после запуска служб; открытие записи ожидает хранилище.
    uint8_t value = LocaleLanguageRussian;
    if(saved_struct_load(
           LOCALE_SETTINGS_PATH,
           &value,
           sizeof(value),
           LOCALE_SETTINGS_MAGIC,
           LOCALE_SETTINGS_VERSION) &&
       value <= LocaleLanguageEnglish) {
        locale_language = (LocaleLanguage)value;
    }
    return 0;
}
