# First-launch guide integration

Automatic help is implemented only for built-in applications selected from the Loader main menu. In the current build, only Sub-GHz reaches this hook: generated `build/f7-firmware-C/applications/applications.c` lists only `subghz` in `FLIPPER_APPS`. NFC, RFID, iButton, Infrared, GPIO, Bad USB and U2F are `MENUEXTERNAL` apps and use the external-app callback without the guide policy check. Archive has a separate entry path. Desktop shortcuts, Archive, RPC, file launches, settings and external apps remain unchanged.

`loader_guide.h` defines a stable nine-topic order matching the first nine User Guide topics: subghz, nfc, lfrfid, ibutton, infrared, gpio, bad_usb, u2f, archive. This mapping does not mean all nine have an intercepted launch path in the current build. The remaining three manual topics (Settings, BT Remote and Safety) have no automatic application mapping or persisted seen marks. Settings use saved_struct version 1 at `/int/loader/guide_settings.bin`, with a per-topic seen bitmask and a global auto-enabled preference (default on). Completing any of the first nine topics manually marks it seen; Back does not. The guide has 12 topics with three pages each. Its UI strings use `locale_translate` through `guide_text`.

`Settings → User Guide → First-launch guides` (`Настройки → Руководство → Подсказки запуска`) offers Off on the left and On on the right; Back leaves the setting unchanged. Both choices preserve the seen bitmask. There is no dedicated UI reset for seen topics. Do not use Storage Factory Reset merely to retest guides: it erases internal storage and resets RTC settings. Safely revisit topics manually and record unseen-state tests as blocked until an isolated test state or a dedicated reset feature is available.

The Loader main-menu built-in-app callback only queues a custom event. The Loader menu dispatcher performs the policy check and loads settings in its own thread. If needed, it starts `User Guide` with the exact `guide:0` through `guide:8` argument. The guide topic renderer also validates its numeric topic before indexing the topic table. The User Guide validates that argument, displays the topic in its ordinary loader-managed application thread, and on completion marks it seen and enqueues the matching stable built-in app ID with `LoaderDeferredLaunchFlagGui`. Back cancels without marking or enqueueing.

There is no general loader interception, RPC/file hook, or dependency on the user_guide implementation from the loader service.

## Build and hardware verification status

As of 18 November 2026, two builds have been built and installed via qFlipper; the second installation completed with `Full Update SUCCESS`. Earlier statements that no build/install had been run described the initial implementation/audit stage only and are not the current project status.

- First build: all 12 English guide topics, three pages each, completed on hardware.
- Second build: English persisted across the update reboot; immediate System row updates during RU → EN → RU and Launcher Settings label refresh confirmed.
- All 12 Russian topics, three pages each, completed.
- A separate normal reboot via Power preserved Russian in System.
- Archive opened using desktop Down; an existing saved IR file launched directly via Archive Run without a guide. The remote screen was visible; no transmission was performed.
- The external NFC FAP opened its Russian start menu without a guide. Sub-GHz opened without a repeat guide after manual completion and reboot.

Automatic-guide cancellation and completion remain untested on hardware: the manual pass marked all nine mapped topics seen, and there is no UI reset. Sub-GHz-only automatic coverage is established by source and the generated application table, not by a successful automatic-guide hardware test. See [RELEASE_NOTES_RU.md](../../../RELEASE_NOTES_RU.md) and [RU_TEST_PLAN.md](../../../RU_TEST_PLAN.md) for results and remaining tests.

## Localization dependency audit

The localization changes outside `applications/main` are covered by explicit `locale` dependencies in their `application.fam` files, including the GUI, Desktop and Loader services, System/Input/Power settings, Clock, and User Guide. The Loader service now calls `locale_translate` for display labels in `loader_menu.c` and explicitly requires `locale`; launch identifiers are unchanged. System refreshes all 12 rows when its language selection changes, and Loader refreshes menu labels after a language change. Standard Canvas primary, secondary and keyboard fonts support Cyrillic in both languages; shared `elements` helpers also handle UTF-8 width trimming and scrolling. Keep this rule when adding translated text to another application: add `locale` to that application's `requires` list rather than relying on an indirect service dependency.

Format strings must remain format strings: do not pass a translated string directly as the format argument to `snprintf` when it can contain user-controlled or localized `%` characters. Build the translated text with a bounded string API or use a fixed format string and pass the translated text as a `%s` argument. The initial audit was source-only; its historical “no build was run” note does not describe the later builds and installations summarized above. Current localization changes also include services and settings outside `applications/main`.
