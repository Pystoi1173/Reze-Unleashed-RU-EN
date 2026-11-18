#include "../nfc_app_i.h"
#include <locale/locale.h>

void nfc_scene_update_initial_wrong_card_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcApp* instance = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_scene_update_initial_wrong_card_on_enter(void* context) {
    NfcApp* instance = context;
    Widget* widget = instance->widget;

    notification_message(instance->notifications, &sequence_error);

    widget_add_icon_element(widget, 83, 22, &I_WarningDolphinFlip_45x42);
    widget_add_string_element(widget, 3, 4, AlignLeft, AlignTop, FontPrimary, locale_translate("Wrong Card!", "Неверная карта!"));
    widget_add_string_multiline_element(
        widget,
        4,
        17,
        AlignLeft,
        AlignTop,
        FontSecondary,
        locale_translate("Data management\nis only possible\nwith source card", "Управление данными\nвозможно только\nс исходной картой"));
    widget_add_button_element(
        widget,
        GuiButtonTypeLeft,
        locale_translate("Retry", "Повторить"),
        nfc_scene_update_initial_wrong_card_widget_callback,
        instance);

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcViewWidget);
}

bool nfc_scene_update_initial_wrong_card_on_event(void* context, SceneManagerEvent event) {
    NfcApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(instance->scene_manager);
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcSceneSavedMenu);
    }
    return consumed;
}

void nfc_scene_update_initial_wrong_card_on_exit(void* context) {
    NfcApp* instance = context;

    widget_reset(instance->widget);
}
