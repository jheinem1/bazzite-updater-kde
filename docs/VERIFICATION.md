# Verification

Initial desktop fork, based on upstream `172c28adf55bb71bab0592c3b72e7c555469f6ee` (1.2.1).

## Local checks

- Built the upstream baseline and the redesigned application using the already-installed KDE 6.11 SDK on james-os (Qt 6.11.3).
- Seven source-contract tests passed.
- The native Qt/Kirigami probe passed 42 checks in Breeze Light and 42 in Breeze Dark. These include screenshot-save assertions, not 84 separate end-user flows.
- Visually inspected the rendered overview, Settings, Details, narrow 840×600 window, and larger-text view. The baseline overview is 1080×720.
- Gamepad signals and connected-controller label strings were injected in the development probe. Physical gamepad hardware was not tested.
- Added guarded-quit cancellation, keyboard Tab traversal, populated update-step rendering, rebase-page opening, and successful-state → failed-retry checks after independent review. A failed retry now clears prior completion feedback.
- No real system update, rollback, rebase, reboot, desktop installation, or james-os integration was performed.

The probe has a compile-time command-execution guard. It renders synthetic display data and changes only its isolated settings/output directory. Release builds must have `BAZZITE_UPDATER_UI_PROBE=OFF`.

## Known warnings and limits

The SDK environment lacks the host OS image-info file. It logs that absence. Kirigami also reports that dynamically constructed page objects are initially outside the graphics scene, and upstream's PageNavigation component emits its deprecation warning. The pages subsequently render and the recorded assertions pass; this is not a zero-warning claim.

No full backend suite exists upstream. Source contracts and UI state tests do not establish update correctness. Release package builds, live desktop/portal theme changes, physical controllers, screen readers, RTL, and Gamescope still need validation. The default CI performs a source build, UI checks, and a separate production build; consult the exact commit's checks before treating remote verification as complete.

## Screenshots

These are actual rendered Qt/Kirigami windows with preview fixture data, not installed-app or physical-system-update captures.

- [Breeze Dark overview](screenshots/desktop-dark.png)
- [Breeze Light overview](screenshots/desktop-light.png)
- [Narrow desktop](screenshots/desktop-narrow.png)
- [Larger text](screenshots/desktop-large-text.png)
- [Update details](screenshots/desktop-details.png)
