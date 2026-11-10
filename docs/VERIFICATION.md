# Verification

Desktop fork `1.2.1-kde.2`, based on upstream `172c28adf55bb71bab0592c3b72e7c555469f6ee` (1.2.1).

## Local checks

- Built the upstream baseline and the redesigned application using the already-installed KDE 6.11 SDK on james-os (Qt 6.11.3).
- Eleven source-contract tests passed.
- The pure QtCore deployment parser passed 458 checks with strict compiler warnings. Cases cover signed/unverified references, registry ports, digests, custom images, missing and malformed data, rpm-ostree boot ordering, bootc queued rollback, and download-only staging.
- The native Qt/Kirigami probe passed 80 checks in Breeze Light and 80 in Breeze Dark. These include screenshot-save assertions, not 160 separate end-user flows.
- Visually inspected the rendered overview, search results/empty state, application menu, Settings, Details, narrow 840×600 window, and larger-text view. The baseline overview is 1080×720.
- Search/menu checks cover Ctrl+F, Enter, Escape, F10, matching feature keywords, no-result feedback, query preservation, controller Menu/A/B, keyboard menu activation, disabled reboot skipping, and reboot confirmation.
- Gamepad signals and connected-controller label strings were injected in the development probe. Physical gamepad hardware was not tested.
- Added guarded-quit cancellation, keyboard Tab traversal, populated update-step rendering, rebase-page opening, and successful-state → failed-retry checks after independent review. A failed retry now clears prior completion feedback.
- Actual mouse press/release tests cover opening the hamburger, closing it with a second click, and clicking outside. Keyboard checks cover F10 opening, Escape dismissal, and menu action activation.
- Deployment UI tests show booted and pending images separately, retain distinct digests, expose missing identity, preserve configured targets in other registry groups, and show custom images in Rebase Helper.
- Details tests cover full-width output, copy action placement and full copied text, F12 reopen, Escape/controller dismissal without a Close button, and preventing controller A from starting an update while reading logs.
- No real system update, rollback, rebase, or reboot was performed. Native release packaging, installation, and downstream image checks are recorded separately with each release.

The probe has a compile-time command-execution guard. It renders synthetic display data and changes only its isolated settings/output directory. Release builds must have `BAZZITE_UPDATER_UI_PROBE=OFF`.

## Known warnings and limits

The SDK environment lacks the host OS image-info file. It logs that absence. Kirigami also reports that dynamically constructed page objects are initially outside the graphics scene, and upstream's PageNavigation component emits its deprecation warning. The pages subsequently render and the recorded assertions pass; this is not a zero-warning claim.

No full backend suite exists upstream. Source contracts and UI state tests do not establish update correctness. Live desktop/portal theme changes, physical controllers, screen readers, RTL, and Gamescope still need validation. The default CI performs a source build, UI checks, and a separate production build; consult the exact commit's checks before treating remote verification as complete.

## Screenshots

These are actual rendered Qt/Kirigami windows with preview fixture data, not installed-app or physical-system-update captures.

- [Breeze Dark overview](screenshots/desktop-dark.png)
- [Breeze Light overview](screenshots/desktop-light.png)
- [Narrow desktop](screenshots/desktop-narrow.png)
- [Larger text](screenshots/desktop-large-text.png)
- [Update details](screenshots/desktop-details.png)

- [Application menu](screenshots/application-menu.png)
- [Search results](screenshots/search-results.png)
- [No matching pages](screenshots/search-empty.png)

- [Booted and next-boot identity](screenshots/deployment-identity.png)
- [Unavailable deployment metadata](screenshots/deployment-unavailable.png)
