// SPDX-FileCopyrightText: 2025 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.statefulapp as StatefulApp
// import org.kde.kirigamiaddons.formcard as FormCard

import io.github.rfrench3.bazzite_updater
import io.github.rfrench3.controllable as GP

// Controller focus is independent of sidebar visibility. Desktop action labels remain plain.

StatefulApp.StatefulWindow {
    id: root

    title: i18nc("@title:window", "Bazzite Updater KDE")

    windowName: "Bazzite Updater KDE"
    objectName: "updaterWindow"
    width: Kirigami.Units.gridUnit * 60
    height: Kirigami.Units.gridUnit * 40

    minimumWidth: Kirigami.Units.gridUnit * 44
    minimumHeight: Kirigami.Units.gridUnit * 28

    visibility: (UseFullscreen || UserSettings.preferFullscreen) ? Window.FullScreen : Window.Windowed

    onClosing: close => {
        close.accepted = false;
        actionQuit.trigger();
    }

    Shortcut {
        sequences: ["F11"]
        context: Qt.ApplicationShortcut
        enabled: !UseFullscreen

        onActivated: {
            if (root.visibility === Window.Windowed)
                root.visibility = Window.FullScreen;
            else if (root.visibility === Window.FullScreen)
                root.visibility = Window.Windowed;
        }
    }

    // Handle global drawer navigation for controllers

    // TODO: gamepad button handling needs a major overhaul, this should be removed when it is ready
    Connections {
        target: GP.Gamepad

        function onButtonEvent(buttonId, button_down) {
            if (AppDialogManager.activeDialog) {
                AppDialogManager.activeDialog.handleInput(buttonId, button_down);
                return;
            }

            // this is a bad system that must be fully reworked
            if (typeof root.pageStack.currentItem.moreHandleInput === "function")
                if (root.pageStack.currentItem.moreHandleInput(buttonId, button_down))
                    return;

            switch (buttonId) {
            case 1: // B
            case 4: // view, minus
            case 6: // pause, plus
                if (button_down)
                    globalDrawer.controllerNavigationActive = !globalDrawer.controllerNavigationActive;
                return;
            }

            if (globalDrawer.controllerNavigationActive) {
                globalDrawer.handleInput(buttonId, button_down);
                return;
            }

            if (typeof root.pageStack.currentItem.handleInput === "function") {
                root.pageStack.currentItem.handleInput(buttonId, button_down);
                return;
            }
        }
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        objectName: "navigationSidebar"
        header: Kirigami.AbstractApplicationHeader {
            objectName: "navigationHeader"
            minimumHeight: preferredHeight
            maximumHeight: preferredHeight
            Kirigami.Heading {
                anchors.fill: parent
                leftPadding: Kirigami.Units.largeSpacing
                verticalAlignment: Text.AlignVCenter
                level: 2
                text: i18n("Navigation")
            }
        }
        modal: false
        drawerOpen: true
        collapsible: false
        collapsed: false
        handleVisible: false
        interactive: false
        closePolicy: QQC2.Popup.NoAutoClose
        resetMenuOnTriggered: false

        // B / View / Menu changes controller focus without hiding navigation.
        property bool controllerNavigationActive: false

        Behavior on width {
            NumberAnimation {
                duration: Kirigami.Units.shortDuration
                easing.type: Easing.InOutQuad
            }
        }

        // In some versions of Kirigami, the drawer width does not function properly. Therefore it must be manually set
        FontMetrics {
            id: actionFontMetrics
            font: Kirigami.Theme.defaultFont
        }
        function getMaxActionTextWidth() {
            // Reading font explicitly makes this recompute after platform font changes.
            const font = actionFontMetrics.font;
            let maxWidth = 0;
            for (let i = 0; i < actions.length; i++) {
                if (actions[i].text) {
                    // Measure the actual pixel width of this specific string
                    let currentWidth = actionFontMetrics.advanceWidth(actions[i].text);
                    if (currentWidth > maxWidth) {
                        maxWidth = currentWidth;
                    }
                }
            }
            return maxWidth;
        }
        width: Math.max(Kirigami.Units.gridUnit * 14, getMaxActionTextWidth() + Kirigami.Units.iconSizes.medium + (Kirigami.Units.largeSpacing * 4))

        Shortcut {
            sequences: ["Ctrl+M"]
            context: Qt.ApplicationShortcut
            enabled: !AppDialogManager.activeDialog
            onActivated: globalDrawer.controllerNavigationActive = !globalDrawer.controllerNavigationActive
        }

        Shortcut {
            sequence: "Escape"
            context: Qt.ApplicationShortcut
            onActivated: {
                if (AppDialogManager.activeDialog)
                    AppDialogManager.activeDialog.reject();
                else if (root.pageStack.currentItem.drawer?.drawerOpen)
                    root.pageStack.currentItem.drawer.close();
                else
                    globalDrawer.controllerNavigationActive = false;
            }
        }

        Shortcut {
            sequences: ["Return", "Enter"]
            context: Qt.ApplicationShortcut
            enabled: globalDrawer.controllerNavigationActive && !AppDialogManager.activeDialog
            onActivated: {
                globalDrawer.controllerNavigationActive = false;
                root.pageStack.currentItem.forceActiveFocus(Qt.TabFocusReason);
            }
        }

        Shortcut {
            sequence: "Up"
            context: Qt.ApplicationShortcut
            enabled: globalDrawer.controllerNavigationActive && !AppDialogManager.activeDialog
            onActivated: globalDrawer.navigatePages(-1)
        }

        Shortcut {
            sequence: "Down"
            context: Qt.ApplicationShortcut
            enabled: globalDrawer.controllerNavigationActive && !AppDialogManager.activeDialog
            onActivated: globalDrawer.navigatePages(1)
        }

        QQC2.ActionGroup {
            id: pageSelector
        }

        actions: [
            Kirigami.Action {

                text: i18n("System Update")
                icon.name: "system-software-update-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                checked: true

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("SystemUpdate.qml")
            },
            Kirigami.Action {

                text: i18n("Other Utilities")
                icon.name: "applications-accessories-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("OtherUtils.qml")
            },
            Kirigami.Action {
                enabled: AppConfig.ini.Rebase?.enabled === "true"
                visible: enabled

                text: i18n("Rebase Helper")
                icon.name: "system-reboot-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("RebasePage.qml")
            },
            Kirigami.Action {
                enabled: (AppConfig.ini.General?.rssFeed ?? "") !== ""
                visible: enabled

                text: i18n("Announcements")
                icon.name: "feed-subscribe-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("RssPage.qml")
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "settings-configure-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: pageStack.initialPage = Qt.resolvedUrl("Settings.qml")
            },
            Kirigami.Action {
                text: i18nc("About (user's OS)", "About %1", AppData.aboutDataOs.displayName)
                icon.name: "help-about-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("AboutDataOS.qml")
            },
            Kirigami.Action {
                text: i18n("About Application")
                icon.name: "help-about-symbolic"

                checkable: true
                QQC2.ActionGroup.group: pageSelector

                onTriggered: root.pageStack.initialPage = Qt.resolvedUrl("AboutDataApp.qml")
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                id: actionReboot
                text: i18n("Reboot System")
                icon.name: AppState.commandSucceeded ? "system-shutdown-update-symbolic" : "system-shutdown-symbolic"

                enabled: !AppState.commandRunning

                onTriggered: {
                    AppDialogManager.show(rebootDialog);
                }
            },
            Kirigami.Action {
                id: actionQuit
                text: i18n("Quit")
                icon.name: "application-exit-symbolic"
                shortcut: StandardKey.Quit
                onTriggered: {
                    // A running command is always worth warning about, only the reminder is optional.
                    // TODO: improve logic for non-atomic systems
                    if (AppState.commandRunning || (!AppData.ostree.onLatestDeployment() && UserSettings.showRebootReminder)) {
                        AppDialogManager.show(exitDialog);
                    } else
                        Qt.quit();
                }
            }
        ]

        function navigatePages(direction) {
            // direction = +1 or -1, used to navigate with a controller or arrow keys

            // Find the current page
            let currentIndex = -1;
            for (let i = 0; i < globalDrawer.actions.length; i++) {
                if (globalDrawer.actions[i].checked) {
                    currentIndex = i;
                    break;
                }
            }

            let newIndex = currentIndex;

            // Find the next page (skip non-page elements of the list)
            for (let j = 0; j < globalDrawer.actions.length; j++) {
                newIndex += direction;

                // Do not wrap around
                if (newIndex < 0 || newIndex >= globalDrawer.actions.length)
                    return;

                let item = globalDrawer.actions[newIndex];
                if (item.checkable && item.visible && item.enabled)
                    break;
            }

            if (newIndex !== currentIndex) {
                const nextAction = globalDrawer.actions[newIndex];
                if (!nextAction?.checkable || !nextAction.visible || !nextAction.enabled)
                    return;
                nextAction.trigger();
            }
        }

        function handleInput(buttonId, button_down) {
            if (!button_down)
                return;

            switch (buttonId) {
            case 0: // A
                controllerNavigationActive = false;
                break;
            case 2: // X
                actionQuit.trigger();
                break;
            case 3: // Y
                actionReboot.trigger();
                break;
            case 11: // Dpad Up
                navigatePages(-1);
                break;
            case 12: // Dpad Down
                navigatePages(1);
                break;
            }
        }
    }

    AppDialog {
        id: rebootDialog
        titleStart: i18nc("@title:window", "Reboot System")
        standardButtons: Kirigami.Dialog.NoButton

        enabled: !AppState.commandRunning

        subtitle: i18n("This will reboot the system.")

        customFooterActions: [
            Kirigami.Action {
                id: confirmReboot
                text: i18n("Reboot")

                onTriggered: rebootDialog.accept()
            },
            Kirigami.Action {
                id: cancelReboot
                text: i18n("Cancel")
                onTriggered: rebootDialog.reject()
            }
        ]

        onAccepted: {
            AppState.rebootSystem(function (callback) {
                rebootTimer.start();
                console.log("Reboot callback: " + callback);
            });
        }

        // Wait a little while to ensure "your reboot has failed" isn't ever visible momentarily before rebooting succeeds
        Timer {
            id: rebootTimer
            interval: 5000
            repeat: false
            onTriggered: root.showPassiveNotification(i18n("The app was unable to reboot. Reboot through your system menu to apply changes."))
        }

        function handleInput(buttonId, button_down) {
            if (!button_down)
                return;
            switch (buttonId) {
            case 0: // A
                confirmReboot.trigger();
                break;
            case 1: // B
                cancelReboot.trigger();
                break;
            }
        }
    }

    AppDialog {
        id: exitDialog
        titleStart: i18n("Exit Application")
        standardButtons: Kirigami.Dialog.NoButton

        subtitle: {
            if (!AppState.commandRunning)
                return !AppData.ostree.onLatestDeployment() ? i18n("You must reboot to apply system updates.") : i18n("No command is running, you may exit safely.");

            if (AppConfig.ini.Commands?.allowEarlyExit !== "true")
                return i18n("You should not exit the application until the running command completes. If you exit early, it may break your system.");
            else
                return i18n("If you exit the application before the running command completes, it may not apply its changes.");
        }

        QQC2.CheckBox {
            id: skipRebootReminder

            // Only the reminder can be silenced, not the warning about a running command.
            visible: !AppState.commandRunning
            enabled: visible

            text: i18n("Do not show this again")

            checked: !UserSettings.showRebootReminder

            onToggled: {
                UserSettings.showRebootReminder = !checked;

                // Assigning to checked drops the binding, put it back
                checked = Qt.binding(() => !UserSettings.showRebootReminder);
            }

            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
        }

        customFooterActions: [
            Kirigami.Action {
                id: confirmExit
                text: i18nc("dialog to exit the application", "Exit")
                enabled: AppConfig.ini.Commands?.allowEarlyExit === "true" || !AppState.commandRunning

                onTriggered: exitDialog.accept()
            },
            Kirigami.Action {
                id: cancelExit
                text: i18n("Cancel")
                onTriggered: exitDialog.reject()
            }
        ]

        onAccepted: Qt.quit()

        function handleInput(buttonId, button_down) {
            if (!button_down)
                return;

            switch (buttonId) {
            case 0: // A
                confirmExit.trigger();
                break;
            case 1: // B
                cancelExit.trigger();
                break;
            case 3: // Y
                if (skipRebootReminder.visible)
                    skipRebootReminder.animateClick();
                break;
            }
        }
    }

    pageStack.initialPage: Qt.resolvedUrl("SystemUpdate.qml")
}
