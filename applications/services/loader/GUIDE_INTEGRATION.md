# First-launch guide integration

Automatic help is implemented only for built-in applications selected from the Loader main menu. Other entry paths (desktop shortcuts, Archive, RPC, file launches, settings and external apps) remain unchanged.

`loader_guide.h` defines a stable nine-topic order matching the first nine User Guide pages: subghz, nfc, lfrfid, ibutton, infrared, gpio, bad_usb, u2f, archive. The remaining three manual topics (Settings, BT Remote and Safety) have no automatic application mapping. Settings use saved_struct version 1 at `/int/loader/guide_settings.bin`, with a per-topic seen bitmask and a global auto-enabled preference (default on). A completed manual topic marks it seen; Back does not. All UI strings use `locale_translate` through `guide_text`.

The Loader main-menu built-in-app callback only queues a custom event. The Loader menu dispatcher performs the policy check and loads settings in its own thread. If needed, it starts `User Guide` with the exact `guide:0` through `guide:8` argument. The guide topic renderer also validates its numeric topic before indexing the topic table. The User Guide validates that argument, displays the topic in its ordinary loader-managed application thread, and on completion marks it seen and enqueues the matching stable built-in app ID with `LoaderDeferredLaunchFlagGui`. Back cancels without marking or enqueueing.

There is no general loader interception, RPC/file hook, or dependency on the user_guide implementation from the loader service. The current implementation was built, packaged, installed and exercised on a Flipper Zero. Automatic-guide cancellation and completion remain a separate test item because the device state already marked the mapped topics as seen.

## Localization dependency audit

The localization changes outside `applications/main` are covered by explicit `locale` dependencies in their `application.fam` files: the GUI and Desktop services, Loader, Locale, System/Input/Power settings, Clock, and User Guide. The Loader uses the locale service for its visible menu labels while keeping stable English launch identifiers. Keep this rule when adding translated text to another application: add `locale` to that application's `requires` list rather than relying on an indirect service dependency.

Format strings must remain format strings: do not pass a translated string directly as the format argument to `snprintf` when it can contain user-controlled or localized `%` characters. Build the translated text with a bounded string API or use a fixed format string and pass the translated text as a `%s` argument. The localization dependency audit was followed by a compact build, package validation and device checks described in the release notes.
