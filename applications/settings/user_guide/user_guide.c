#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/menu.h>
#include <dialogs/dialogs.h>
#include <assets_icons.h>
#include <locale/locale.h>
#include <loader/loader.h>
#include <loader/loader_guide.h>
#include <string.h>

typedef struct {
    const char* title_en;
    const char* title_ru;
    const char* pages_en[3];
    const char* pages_ru[3];
} GuideTopic;

typedef struct {
    Gui* gui;
    Menu* menu;
    ViewDispatcher* view_dispatcher;
} UserGuide;

enum {
    UserGuideEventAuto = 1,
    UserGuideEventTopic,
};

static const GuideTopic guide_topics[] = {
    {"Sub-GHz", "Sub-GHz", {"Select Read and scan\nyour test device.", "Save only an owned\nsignal. Check region\nand power limits.", "Choose Write or Send\nonly with permission."}, {"Выберите Чтение и\nсканируйте свой тест.", "Сохраняйте только\nсвой сигнал. Проверьте\nрегион и мощность.", "Запись или Передача —\nтолько с разрешения."}},
    {"NFC", "NFC", {"Place the card at the\nback of the Flipper.", "Use Read or Write with\nyour own test tag.", "Emulate only a tag you\nown and may test."}, {"Приложите карту к\nзадней стороне Flipper.", "Выберите Чтение или\nЗапись для своей метки.", "Эмулируйте только\nсвою тестовую метку."}},
    {"RFID 125 kHz", "RFID 125 кГц", {"Open Read and hold\nyour LF test tag near\nthe antenna.", "Save a read only for\nyour own test setup.", "Emulate only your own\ncompatible test tag."}, {"Откройте Чтение и\nподнесите LF-метку\nк антенне.", "Сохраняйте данные\nтолько для своего\nтестового стенда.", "Эмулируйте только\nсвою совместимую\nтестовую метку."}},
    {"iButton", "iButton", {"Open Read and touch\nyour owned key.", "Save the key only with\nowner permission.", "Use Emulate only with\nyour own test lock."}, {"Откройте Чтение и\nкоснитесь своим ключом.", "Сохраняйте ключ только\nс разрешения владельца.", "Эмуляция — только\nсо своим тестовым замком."}},
    {"Infrared", "Инфракрасный", {"Point the IR port at\nyour own receiver.", "Use Learn, then Save\nthe remote button.", "Open Send to test the\nsaved button."}, {"Направьте ИК-порт\nна свой приёмник.", "Выберите Обучение,\nсохраните кнопку.", "Откройте Передача и\nпроверьте свою кнопку."}},
    {"GPIO", "GPIO", {"Check voltage and pins\nbefore connecting.", "Select the needed pin\nmode for your test.", "Read signals only from\nyour safe test circuit."}, {"Проверьте напряжение\nи выводы до подключения.", "Выберите режим вывода\nдля своего теста.", "Считывайте сигналы\nтолько с безопасной\nсхемы."}},
    {"BadUSB", "BadUSB", {"Open a script from SD\nfor your own computer.", "Review every line before\nusing Run.", "Stop it with Back; never\ntest on someone else's data."}, {"Откройте скрипт с SD\nдля своего компьютера.", "Проверьте каждую строку\nперед запуском.", "Остановите кнопкой\nНазад; не трогайте\nчужие данные."}},
    {"U2F", "U2F", {"Open U2F and choose\nRegister on a test site.", "Touch the key when\nthe site requests it.", "Keep a backup sign-in\nmethod for your account."}, {"Откройте U2F и выберите\nРегистрация на тестовом\nсайте.", "Коснитесь ключа по\nзапросу сайта.", "Сохраните запасной\nспособ входа."}},
    {"Archive", "Архив", {"Use Up and Down to\nselect a file or folder.", "Press OK to open; use\nBack to return.", "Copy or delete only\nfiles you own."}, {"Вверх и вниз — выбор\nфайла или папки.", "OK — открыть; Назад —\nвернуться.", "Копируйте и удаляйте\nтолько свои файлы."}},
    {"BT Remote", "BT Remote", {"Pair only with your own\ncompatible device.", "Check the device name\nbefore connecting.", "Use buttons as a remote\nonly with permission."}, {"Сопрягайте только\nсвоё совместимое\nустройство.", "Проверьте имя устройства\nперед подключением.", "Используйте пульт\nтолько с разрешения."}},
    {"Settings", "Настройки", {"Short OK selects an item.\nUp and Down move.", "Back cancels or returns.\nRead the value shown.", "Long OK is not needed\nfor normal controls."}, {"Короткий OK выбирает.\nВверх и вниз — переход.", "Назад отменяет или\nвозвращает. Смотрите\nзначение на экране.", "Долгое OK не нужно\nдля обычных пунктов."}},
    {"Safety", "Важно", {"Features depend on the\nmodel and region.", "Use authorized devices\nand your own test data.", "Follow local law and\nstop if unsure."}, {"Возможности зависят\nот модели и региона.", "Используйте свои\nустройства и тестовые\nданные.", "Соблюдайте закон и\nостановитесь при сомнении."}},
};

static const char* guide_text(const char* english, const char* russian) {
    return locale_translate(english, russian);
}

static bool guide_show_topic(uint32_t topic) {
    if(topic >= COUNT_OF(guide_topics)) return false;
    const GuideTopic* guide = &guide_topics[topic];
    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* message = dialog_message_alloc();
    bool completed = true;

    for(size_t page = 0; page < 3; page++) {
        dialog_message_set_header(
            message,
            guide_text(guide->title_en, guide->title_ru),
            2,
            0,
            AlignLeft,
            AlignTop);
        dialog_message_set_text(
            message,
            guide_text(guide->pages_en[page], guide->pages_ru[page]),
            2,
            14,
            AlignLeft,
            AlignTop);
        dialog_message_set_buttons(
            message,
            NULL,
            NULL,
            guide_text(page == 2 ? "Done" : "Next", page == 2 ? "Готово" : "Далее"));
        if(dialog_message_show(dialogs, message) == DialogMessageButtonBack) {
            completed = false;
            break;
        }
    }

    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);
    return completed;
}

static void guide_auto_settings(void) {
    LoaderGuideSettings settings;
    loader_guide_settings_load(&settings);
    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(
        message, guide_text("First-launch guides", "Подсказки запуска"), 2, 0, AlignLeft, AlignTop);
    dialog_message_set_text(
        message,
        settings.auto_enabled ?
            guide_text("Auto guides: On\nBuilt-in apps from\nLoader main menu only.",
                       "Подсказки: вкл\nВстроенные приложения\nглавного меню Loader.") :
            guide_text("Auto guides: Off\nBuilt-in apps from\nLoader main menu only.",
                       "Подсказки: выкл\nВстроенные приложения\nглавного меню Loader."),
        2, 14, AlignLeft, AlignTop);
    dialog_message_set_buttons(
        message, guide_text("Off", "Выкл"), NULL, guide_text("On", "Вкл"));
    DialogMessageButton result = dialog_message_show(dialogs, message);
    if(result == DialogMessageButtonLeft || result == DialogMessageButtonRight) {
        settings.auto_enabled = result == DialogMessageButtonRight;
        if(!loader_guide_settings_save(&settings)) {
            dialog_message_set_text(
                message, guide_text("Could not save setting.", "Не удалось сохранить."),
                2, 14, AlignLeft, AlignTop);
            dialog_message_set_buttons(message, NULL, NULL, guide_text("Done", "Готово"));
            dialog_message_show(dialogs, message);
        }
    }
    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);
}

static void guide_menu_callback(void* context, uint32_t index) {
    UserGuide* guide = context;
    view_dispatcher_send_custom_event(
        guide->view_dispatcher,
        index == COUNT_OF(guide_topics) ? UserGuideEventAuto : UserGuideEventTopic + index);
}

static bool guide_custom_event_callback(void* context, uint32_t event) {
    UNUSED(context);
    /* Блокирующий диалог выполняется только в потоке приложения. */
    if(event == UserGuideEventAuto) {
        guide_auto_settings();
        return true;
    }
    if(event < UserGuideEventTopic) return false;
    const uint32_t topic = event - UserGuideEventTopic;
    if(topic < COUNT_OF(guide_topics) && guide_show_topic(topic) &&
       topic < LOADER_GUIDE_TOPIC_COUNT) {
        LoaderGuideSettings settings;
        loader_guide_settings_load(&settings);
        loader_guide_mark_seen(&settings, topic);
        if(!loader_guide_settings_save(&settings)) FURI_LOG_W("UserGuide", "Save seen failed");
    }
    return true;
}

static uint32_t guide_exit_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static bool guide_parse_topic(const char* args, uint32_t* topic) {
    if(!args || strlen(args) != 7 || strncmp(args, "guide:", 6) != 0 || args[6] < '0' ||
       args[6] > '8') {
        return false;
    }
    *topic = (uint32_t)(args[6] - '0');
    return true;
}

static void guide_finish_auto(uint32_t topic) {
    LoaderGuideSettings settings;
    loader_guide_settings_load(&settings);
    loader_guide_mark_seen(&settings, topic);
    if(!loader_guide_settings_save(&settings)) FURI_LOG_W("UserGuide", "Save seen failed");
    const char* target = loader_guide_topic_app(topic);
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_enqueue_launch(loader, target, NULL, LoaderDeferredLaunchFlagGui);
    furi_record_close(RECORD_LOADER);
}

int32_t user_guide_app(void* p) {
    const char* args = p;
    uint32_t auto_topic;
    const bool auto_mode = guide_parse_topic(args, &auto_topic);
    if(auto_mode) {
        if(guide_show_topic(auto_topic)) guide_finish_auto(auto_topic);
        return 0;
    }

    UserGuide guide = {0};
    guide.gui = furi_record_open(RECORD_GUI);
    guide.menu = menu_alloc();
    guide.view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(guide.view_dispatcher, &guide);
    view_dispatcher_set_custom_event_callback(guide.view_dispatcher, guide_custom_event_callback);

    for(size_t i = 0; i < COUNT_OF(guide_topics); i++) {
        menu_add_item(
            guide.menu,
            guide_text(guide_topics[i].title_en, guide_topics[i].title_ru),
            &A_Settings_14,
            i,
            guide_menu_callback,
            &guide);
    }

    menu_add_item(
        guide.menu,
        guide_text("First-launch guides", "Подсказки запуска"),
        &A_Settings_14,
        COUNT_OF(guide_topics),
        guide_menu_callback,
        &guide);

    view_dispatcher_add_view(guide.view_dispatcher, 0, menu_get_view(guide.menu));
    view_set_previous_callback(menu_get_view(guide.menu), guide_exit_callback);
    view_dispatcher_attach_to_gui(guide.view_dispatcher, guide.gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(guide.view_dispatcher, 0);
    view_dispatcher_run(guide.view_dispatcher);
    view_dispatcher_remove_view(guide.view_dispatcher, 0);
    view_dispatcher_free(guide.view_dispatcher);
    menu_free(guide.menu);
    furi_record_close(RECORD_GUI);
    return 0;
}
