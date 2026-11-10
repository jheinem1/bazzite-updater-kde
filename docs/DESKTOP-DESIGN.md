# Desktop design review

This is a focused first pass, not certification of complete KDE HIG compliance.

## Applied patterns

- Persistent nonmodal sidebar with no collapse action or redundant Navigation label. Its native application header aligns with the page toolbar and pairs a Search field with an application-menu button, following Info Center. Navigation remains visible after switching pages, opening details, dismissing dialogs, and resizing.
- Search filters page titles and feature keywords, including rollback under Other Utilities. Ctrl+F focuses and selects the query, Enter activates the first match, and an empty result is explained. Clearing search restores the full navigation list.
- Reboot System and Quit live in the sidebar hamburger menu rather than among pages. F10 or the controller Menu button opens it. Reboot confirmation and guarded quit behavior remain intact.
- Plain action and page names, including with a controller connected. Controller glyphs no longer become part of translated labels or accessible names.
- Controller navigation has its own focus state. B or View switches between page and sidebar navigation; D-pad selects pages and A returns to the page. The existing controller input backend remains in place.
- One page heading. The update page has a native status card and explicit update button, rather than another large logo/title and a controller-oriented action tile.
- Persistent running/failure/success feedback, a named Show Details / Hide Details action, and no meaningless idle update progress bar. The separate system-drive bar is labelled and shown only when capacity is known.
- No global Return shortcut that starts an update. Return/Space activate focused controls normally. Ctrl+F focuses search, F10 opens the application menu, F12 toggles details, Escape dismisses the menu, search, details, or dialogs, Ctrl+M selects navigation, and Ctrl+Q retains the guarded quit flow.
- Disabled actions use `trigger()` rather than emitting their signal directly, preserving their enabled guard for controller invocation.
- Normal startup retains KDE's font, palette, style, and accent. Light/dark preview overrides exist only in the opt-in regression build.
- Context properties needed by QML are initialized before loading the UI.

These follow the [KDE HIG](https://develop.kde.org/hig/), particularly [layout and navigation](https://develop.kde.org/hig/layout_and_nav/), [text and labels](https://develop.kde.org/hig/text_and_labels/), [accessibility](https://develop.kde.org/hig/accessibility/), and [status changes](https://develop.kde.org/hig/status_changes/). They also follow lessons from the Piper Kirigami desktop work: public native components, aligned headers, platform styling, larger-text checks, and scoped actions.

## Boundaries

The backend, system configuration lookup, and upstream update/rebase/rollback implementation are retained. No real system update, rebase, rollback, or reboot is performed by the UI tests. Physical-controller behavior, screen readers, right-to-left languages, Gamescope, live desktop theme changes, release packages, installation, and downstream OS integration require additional verification. Existing controller components emit a deprecation warning; migration to their newer API is separate work.

The desktop minimum is 44 × 28 Kirigami grid units. Navigation stays expanded at that minimum; this deliberately prioritizes desktop use over narrow phone layouts.
