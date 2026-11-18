#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/menu.h>
#include <gui/modules/submenu.h>
#include <assets_icons.h>
#include <applications.h>
#include <archive/helpers/archive_favorites.h>
#include "loader.h"
#include "loader_menu.h"
#include "loader_guide.h"
#include <locale/locale.h>
#include <string.h>

#define TAG "LoaderMenu"

struct LoaderMenu {
    FuriThread* thread;
    void (*closed_cb)(void*);
    void* context;
    Menu* primary_menu;
    Submenu* settings_menu;
    LocaleLanguage language;
    ViewDispatcher* view_dispatcher;
};

typedef enum {
    LoaderMenuViewPrimary,
    LoaderMenuViewSettings,
} LoaderMenuView;

typedef enum {
    LoaderMenuEventStart = 1,
} LoaderMenuEvent;

static const char* loader_menu_label(const char* name) {
    static const struct {
        const char* english;
        const char* russian;
    } labels[] = {
        {"Apps", "Приложения"},
        {"Settings", "Настройки"},
        {"System", "Система"},
        {"Desktop", "Рабочий стол"},
        {"Clock & Alarm", "Часы и будильник"},
        {"Clock", "Часы"},
        {"User Guide", "Руководство"},
        {"Power", "Питание"},
        {"Storage", "Хранилище"},
        {"About", "О системе"},
        {"Input", "Кнопки"},
        {"LCD and Notifications", "Экран и уведомления"},
        {"Expansion Modules", "Модули расширения"},
        {"Passport", "Паспорт"},
        {"Infrared", "ИК-пульты"},
        {"125 kHz RFID", "RFID 125 кГц"},
        {"Sub-GHz Remote", "Пульт Sub-GHz"},
    };
    for(size_t i = 0; i < COUNT_OF(labels); i++) {
        if(strcmp(name, labels[i].english) == 0 || strcmp(name, labels[i].russian) == 0) {
            return locale_translate(labels[i].english, labels[i].russian);
        }
    }
    return name;
}

static void loader_menu_refresh_language(void* context) {
    LoaderMenu* loader_menu = context;
    LocaleLanguage language = locale_get_language();
    if(loader_menu->language == language) return;
    loader_menu->language = language;
    with_view_model(
        menu_get_view(loader_menu->primary_menu),
        MenuModel * model,
        {
            for(size_t i = 0; i < model->count; i++) {
                model->items[i].label = loader_menu_label(model->items[i].label);
            }
        },
        true);
    for(size_t i = 0; i < FLIPPER_EXTSETTINGS_APPS_COUNT; i++) {
        submenu_change_item_label(
            loader_menu->settings_menu,
            (uint32_t)FLIPPER_EXTSETTINGS_APPS[i].name,
            loader_menu_label(FLIPPER_EXTSETTINGS_APPS[i].name));
    }
    for(size_t i = 0; i < FLIPPER_SETTINGS_APPS_COUNT; i++) {
        submenu_change_item_label(
            loader_menu->settings_menu,
            (uint32_t)FLIPPER_SETTINGS_APPS[i].name,
            loader_menu_label(FLIPPER_SETTINGS_APPS[i].name));
    }
}

static void loader_menu_start(const char* name) {
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_start_with_gui_error(loader, name, NULL);
    furi_record_close(RECORD_LOADER);
}

static void loader_menu_apps_callback(void* context, uint32_t index) {
    LoaderMenu* loader_menu = context;
    view_dispatcher_send_custom_event(
        loader_menu->view_dispatcher, LoaderMenuEventStart + (uint32_t)index);
}

static bool loader_menu_custom_event_callback(void* context, uint32_t event) {
    UNUSED(context);
    if(event < LoaderMenuEventStart || event >= LoaderMenuEventStart + FLIPPER_APPS_COUNT) {
        return false;
    }

    const uint32_t index = event - LoaderMenuEventStart;
    const FlipperInternalApplication* app = &FLIPPER_APPS[index];
    const int32_t topic = loader_guide_topic_for_app(app->name, app->appid);
    if(topic < 0) {
        loader_menu_start(app->name);
        return true;
    }

    LoaderGuideSettings settings;
    loader_guide_settings_load(&settings);
    Loader* loader = furi_record_open(RECORD_LOADER);
    if(loader_guide_should_show(&settings, (uint32_t)topic)) {
        static const char* const guide_args[LOADER_GUIDE_TOPIC_COUNT] = {
            "guide:0",
            "guide:1",
            "guide:2",
            "guide:3",
            "guide:4",
            "guide:5",
            "guide:6",
            "guide:7",
            "guide:8"};
        loader_start_with_gui_error(loader, "User Guide", guide_args[topic]);
    } else {
        loader_start_with_gui_error(loader, app->name, NULL);
    }
    furi_record_close(RECORD_LOADER);
    return true;
}

static void loader_menu_external_apps_callback(void* context, uint32_t index) {
    UNUSED(context);
    const char* path = FLIPPER_EXTERNAL_APPS[index].name;
    loader_menu_start(path);
}

static void loader_menu_applications_callback(void* context, uint32_t index) {
    UNUSED(index);
    UNUSED(context);
    const char* name = LOADER_APPLICATIONS_NAME;
    loader_menu_start(name);
}

static void
    loader_menu_settings_menu_callback(void* context, InputType input_type, uint32_t index) {
    UNUSED(context);
    if(input_type == InputTypeShort) {
        loader_menu_start((const char*)index);
    } else if(input_type == InputTypeLong) {
        archive_favorites_handle_setting_pin_unpin((const char*)index, NULL);
    }
}

static void loader_menu_switch_to_settings(void* context, uint32_t index) {
    UNUSED(index);
    LoaderMenu* loader_menu = context;
    view_dispatcher_switch_to_view(loader_menu->view_dispatcher, LoaderMenuViewSettings);
}

static uint32_t loader_menu_switch_to_primary(void* context) {
    UNUSED(context);
    return LoaderMenuViewPrimary;
}

static uint32_t loader_menu_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void loader_menu_build_menu(LoaderMenu* loader_menu) {
    Menu* menu = loader_menu->primary_menu;
    size_t i = 0;

    menu_add_item(
        menu,
        loader_menu_label(LOADER_APPLICATIONS_NAME),
        &A_Plugins_14,
        i++,
        loader_menu_applications_callback,
        loader_menu);

    for(i = 0; i < FLIPPER_APPS_COUNT; i++) {
        menu_add_item(
            menu,
            loader_menu_label(FLIPPER_APPS[i].name),
            FLIPPER_APPS[i].icon,
            i,
            loader_menu_apps_callback,
            loader_menu);
    }

    for(i = 0; i < FLIPPER_EXTERNAL_APPS_COUNT; i++) {
        menu_add_item(
            menu,
            loader_menu_label(FLIPPER_EXTERNAL_APPS[i].name),
            FLIPPER_EXTERNAL_APPS[i].icon,
            i,
            loader_menu_external_apps_callback,
            loader_menu);
    }

    menu_add_item(
        menu,
        loader_menu_label("Settings"),
        &A_Settings_14,
        i++,
        loader_menu_switch_to_settings,
        loader_menu);
}

static void loader_menu_build_submenu(Submenu* submenu, LoaderMenu* loader_menu) {
    for(size_t i = 0; i < FLIPPER_EXTSETTINGS_APPS_COUNT; i++) {
        submenu_add_item_ex(
            submenu,
            loader_menu_label(FLIPPER_EXTSETTINGS_APPS[i].name),
            (uint32_t)FLIPPER_EXTSETTINGS_APPS[i].name,
            loader_menu_settings_menu_callback,
            loader_menu);
    }
    for(size_t i = 0; i < FLIPPER_SETTINGS_APPS_COUNT; i++) {
        submenu_add_item_ex(
            submenu,
            loader_menu_label(FLIPPER_SETTINGS_APPS[i].name),
            (uint32_t)FLIPPER_SETTINGS_APPS[i].name,
            loader_menu_settings_menu_callback,
            loader_menu);
    }
}

static int32_t loader_menu_thread(void* p) {
    LoaderMenu* loader_menu = p;
    furi_assert(loader_menu);

    Gui* gui = furi_record_open(RECORD_GUI);
    Submenu* settings_menu = submenu_alloc();
    loader_menu->settings_menu = settings_menu;
    loader_menu_build_submenu(settings_menu, loader_menu);

    loader_menu->view_dispatcher = view_dispatcher_alloc();

    View* primary_view = menu_get_view(loader_menu->primary_menu);
    view_set_previous_callback(primary_view, loader_menu_exit);
    view_dispatcher_add_view(loader_menu->view_dispatcher, LoaderMenuViewPrimary, primary_view);

    View* settings_view = submenu_get_view(settings_menu);
    view_set_previous_callback(settings_view, loader_menu_switch_to_primary);
    view_dispatcher_add_view(loader_menu->view_dispatcher, LoaderMenuViewSettings, settings_view);
    view_dispatcher_switch_to_view(loader_menu->view_dispatcher, LoaderMenuViewPrimary);

    view_dispatcher_set_event_callback_context(loader_menu->view_dispatcher, loader_menu);
    view_dispatcher_set_tick_event_callback(
        loader_menu->view_dispatcher, loader_menu_refresh_language, furi_ms_to_ticks(100));
    view_dispatcher_set_custom_event_callback(
        loader_menu->view_dispatcher, loader_menu_custom_event_callback);
    view_dispatcher_attach_to_gui(loader_menu->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_run(loader_menu->view_dispatcher);

    if(loader_menu->closed_cb) {
        loader_menu->closed_cb(loader_menu->context);
    }

    view_dispatcher_remove_view(loader_menu->view_dispatcher, LoaderMenuViewPrimary);
    view_dispatcher_remove_view(loader_menu->view_dispatcher, LoaderMenuViewSettings);
    view_dispatcher_free(loader_menu->view_dispatcher);
    submenu_free(settings_menu);
    furi_record_close(RECORD_GUI);

    return 0;
}

LoaderMenu* loader_menu_alloc(void (*closed_cb)(void*), void* context, const MenuStyle* style) {
    LoaderMenu* loader_menu = malloc(sizeof(LoaderMenu));
    loader_menu->closed_cb = closed_cb;
    loader_menu->context = context;
    loader_menu->primary_menu = menu_alloc();
    loader_menu->language = locale_get_language();
    menu_set_style(loader_menu->primary_menu, style);
    loader_menu_build_menu(loader_menu);
    loader_menu->thread = furi_thread_alloc_ex(TAG, 1024, loader_menu_thread, loader_menu);
    furi_thread_start(loader_menu->thread);
    return loader_menu;
}

void loader_menu_free(LoaderMenu* loader_menu) {
    furi_assert(loader_menu);
    furi_thread_join(loader_menu->thread);
    furi_thread_free(loader_menu->thread);
    menu_free(loader_menu->primary_menu);
    free(loader_menu);
}

void loader_menu_set_style(LoaderMenu* loader_menu, const MenuStyle* style) {
    furi_assert(loader_menu);
    menu_set_style(loader_menu->primary_menu, style);
}
