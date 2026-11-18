#include <furi.h>
#include <string.h>
#include <storage/storage.h>
#include <toolbox/saved_struct.h>

#include "loader_guide.h"

#define LOADER_GUIDE_SETTINGS_PATH "/int/loader/guide_settings.bin"
#define LOADER_GUIDE_SETTINGS_MAGIC 0x47
#define LOADER_GUIDE_SETTINGS_VERSION 1

static const char* const guide_apps[LOADER_GUIDE_TOPIC_COUNT] = {
    "subghz", "nfc", "lfrfid", "ibutton", "infrared", "gpio", "bad_usb", "u2f", "archive"};

const char* loader_guide_topic_app(uint32_t topic) {
    return topic < LOADER_GUIDE_TOPIC_COUNT ? guide_apps[topic] : NULL;
}

int32_t loader_guide_topic_for_app(const char* name, const char* appid) {
    for(uint32_t topic = 0; topic < LOADER_GUIDE_TOPIC_COUNT; topic++) {
        if((name && strcmp(name, guide_apps[topic]) == 0) ||
           (appid && strcmp(appid, guide_apps[topic]) == 0)) {
            return (int32_t)topic;
        }
    }
    return -1;
}

void loader_guide_settings_load(LoaderGuideSettings* settings) {
    furi_assert(settings);
    LoaderGuideSettings loaded;
    *settings = (LoaderGuideSettings){.auto_enabled = 1};
    if(saved_struct_load(
           LOADER_GUIDE_SETTINGS_PATH,
           &loaded,
           sizeof(loaded),
           LOADER_GUIDE_SETTINGS_MAGIC,
           LOADER_GUIDE_SETTINGS_VERSION) &&
       loaded.auto_enabled <= 1 && loaded.reserved == 0) {
        *settings = loaded;
    }
}

bool loader_guide_should_show(const LoaderGuideSettings* settings, uint32_t topic) {
    return settings && settings->auto_enabled && topic < LOADER_GUIDE_TOPIC_COUNT &&
           !(settings->seen & (uint16_t)(1U << topic));
}

void loader_guide_mark_seen(LoaderGuideSettings* settings, uint32_t topic) {
    if(settings && topic < LOADER_GUIDE_TOPIC_COUNT) settings->seen |= (uint16_t)(1U << topic);
}

bool loader_guide_settings_save(const LoaderGuideSettings* settings) {
    furi_assert(settings);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, "/int/loader");
    furi_record_close(RECORD_STORAGE);
    return saved_struct_save(
        LOADER_GUIDE_SETTINGS_PATH,
        settings,
        sizeof(*settings),
        LOADER_GUIDE_SETTINGS_MAGIC,
        LOADER_GUIDE_SETTINGS_VERSION);
}
