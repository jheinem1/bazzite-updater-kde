# Verification

Desktop fork `1.2.1-kde.3`, based on upstream `172c28adf55bb71bab0592c3b72e7c555469f6ee` (1.2.1).

## Local checks

- Built the upstream baseline and the redesigned application using the already-installed KDE 6.11 SDK on james-os (Qt 6.11.3).
- Eleven source-contract tests passed.
- The pure QtCore deployment parser passed 458 checks with strict compiler warnings. Cases cover signed/unverified references, registry ports, digests, custom images, missing and malformed data, rpm-ostree boot ordering, bootc queued rollback, and download-only staging.
- The native Qt/Kirigami probe passed 84 checks in Breeze Light and 84 in Breeze Dark. These include screenshot-save assertions, not 168 separate end-user flows.
- Visually inspected the rendered overview, search results/empty state, application menu, Settings, Details, narrow 840×600 window, and larger-text view. The baseline overview is 1080×720.
- Search/menu checks cover Ctrl+F, Enter, Escape, F10, matching feature keywords, no-result feedback, query preservation, controller Menu/A/B, keyboard menu activation, disabled reboot skipping, and reboot confirmation.
- Gamepad signals and connected-controller label strings were injected in the development probe. Physical gamepad hardware was not tested.
- Added guarded-quit cancellation, keyboard Tab traversal, populated update-step rendering, rebase-page opening, and successful-state → failed-retry checks after independent review. A failed retry now clears prior completion feedback.
- Actual mouse press/release tests cover opening the hamburger, closing it with a second click, and clicking outside. Keyboard checks cover F10 opening, Escape dismissal, and menu action activation.
- Deployment UI tests show booted and pending images separately, retain distinct digests, expose missing identity, preserve configured targets in other registry groups, and show custom images in Rebase Helper.
- Details tests cover full-width output, the translated Console Output heading and Copy placement at normal and narrow widths, full copied text, F12 reopen, Escape/controller dismissal without a Close button, and preventing controller A from starting an update while reading logs.
- No real system update, rollback, rebase, or reboot was performed. Native release packaging, installation, and downstream image checks are recorded separately with each release.

The probe has a compile-time command-execution guard. It renders synthetic display data and changes only its isolated settings/output directory. Release builds must have `BAZZITE_UPDATER_UI_PROBE=OFF`.

## Known warnings and limits

The SDK environment lacks the host OS image-info file. It logs that absence. Kirigami also reports that dynamically constructed page objects are initially outside the graphics scene, and upstream's PageNavigation component emits its deprecation warning. The pages subsequently render and the recorded assertions pass; this is not a zero-warning claim.

No full backend suite exists upstream. Source contracts and UI state tests do not establish update correctness. Live desktop/portal theme changes, physical controllers, screen readers, RTL, and Gamescope still need validation. The default CI performs a source build, UI checks, and a separate production build; consult the exact commit's checks before treating remote verification as complete.

## Upstream maintenance: November 24, 2026

Reviewed upstream `2405a6daf170c7d5dd4bdcb2689980d433dcf2b3..1e507795d0a113fb4624988929b8c04302788736` (40 commits), against fork `82aba3f7838c210399b29af4045e5850b3fa149f`. Upstream's latest published release was still 1.2.1; its main branch had bumped the working version to 1.3.0.

- Adapted Robert French's bootc progress handling from [`b7917be`](https://github.com/rfrench3/bazzite-updater/commit/b7917be6e4dea95a6d9b4a7153ccb51f377cb5fc): the updater progress model accepts bounded byte percentages, and the existing Details pane shows bootc's readable descriptions. Legacy text progress and the existing command/status handling remain in place. The probe includes synthetic UUPD JSON, multi-gigabyte counters, malformed input and legacy output; it does not invoke the real updater.
- Kept the fork's existing booted/pending deployment parser and status checks. They already distinguish unknown status from “no restart needed” ([upstream #43](https://github.com/rfrench3/bazzite-updater/issues/43)) and use deployment metadata for the booted build date rather than the base distribution's date.
- Did not import the layered-package remove/reset operations, Gamescope-specific rebase controls, QML restructuring, translation files tied to upstream UI, version bump, or dev-container changes. The fork keeps its existing KDE pages and configuration. Desktop CI already builds with Ninja (upstream #66).

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
