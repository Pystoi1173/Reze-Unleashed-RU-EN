#pragma once

#include "loader.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOADER_GUIDE_TOPIC_COUNT 9
typedef struct {
    uint16_t seen;
    uint8_t auto_enabled;
    uint8_t reserved;
} LoaderGuideSettings;

/* These helpers perform storage I/O: use only from an application/dispatcher thread. */
void loader_guide_settings_load(LoaderGuideSettings* settings);
bool loader_guide_settings_save(const LoaderGuideSettings* settings);
const char* loader_guide_topic_app(uint32_t topic);
int32_t loader_guide_topic_for_app(const char* name, const char* appid);

/* Pure policy check; callers must separately establish a local normal launch.
 * This API deliberately does not launch apps or display dialogs. */
bool loader_guide_should_show(const LoaderGuideSettings* settings, uint32_t topic);
void loader_guide_mark_seen(LoaderGuideSettings* settings, uint32_t topic);

#ifdef __cplusplus
}
#endif
