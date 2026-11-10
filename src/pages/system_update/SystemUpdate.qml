// SPDX-FileCopyrightText: 2025-2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FC

import io.github.rfrench3.bazzite_updater
import io.github.rfrench3.controllable as GP

AppPage {
    id: page

    title: i18n("System Update")
    objectName: "systemUpdatePage"
    drawer: consoleDrawer
    actionToggleDrawer: toggleConsole

    function handleInput(buttonId: int, button_down: bool): void {
        if (button_down == false)
            return;

        switch (buttonId) {
        case 0: // A
            updateButton.animateClick();
            return;
        }

        if (consoleDrawer.drawerOpen) {
            consoleDrawer.handleInput(buttonId, button_down);
            return;
        }

        switch (buttonId) {
        case 3: // Y
            toggleConsole.trigger();
            break;
        }
    }

    Kirigami.Action {
            id: updateAction
            objectName: "updateAction"
            text: i18n("Update System and Software")
            icon.name: "system-software-update"
            enabled: AppState.allowCommands && !SystemUpdateBackend.blockUpdate && (AppConfig.ini.Commands?.systemUpdateCommand || "")

            onTriggered: {
                sessionStorage.updateCompleted = false;
                sessionStorage.updateFailed = false;
                sessionStorage.mainText = i18n("Installing updates…");

                showPassiveNotification(i18n("Update Started"), Kirigami.short);
                SystemUpdateBackend.runUpdate(callback => {
                    if (callback != 0) {
                        if (consoleDrawer.drawerOpen) {
                            showPassiveNotification(i18n("Update Failed."), Kirigami.short);
                        } else {
                            showPassiveNotification(i18n("Update Failed. Check console for more details."), Kirigami.long, i18n("Open console"), () => {
                                toggleConsole.trigger();
                            });
                        }
                        sessionStorage.updateFailed = true;
                        sessionStorage.mainText = "";
                        return;
                    }

                    showPassiveNotification(i18n("Update Succeeded!"), Kirigami.short);
                    sessionStorage.updateCompleted = true;
                    sessionStorage.mainText = i18n("System Updated!");

                    AppData.reloadOstree(() => {
                        if (!AppData.ostree.onLatestDeployment())
                            showPassiveNotification(i18n("You must reboot to apply changes."), Kirigami.short);
                        else
                            showPassiveNotification(i18n("No reboot necessary."), Kirigami.short);
                    });
                }, errorMessage => {
                    errorDialog.subtitle = errorMessage;
                    AppDialogManager.show(errorDialog);
                });
            }
    }

    actions: [
        Kirigami.Action {
            id: toggleConsole
            objectName: "toggleConsoleAction"
            text: consoleDrawer.drawerOpen ? i18n("Hide Details") : i18n("Show Details")
            icon.name: "utilities-terminal"
            checkable: true
            checked: consoleDrawer.drawerOpen
            shortcut: "F12"
            onTriggered: {
                // hide notifications when opening the drawer
                if (!consoleDrawer.drawerOpen) {
                    for (let i = 0; i < 5; ++i) {
                        hidePassiveNotification();
                    }
                }

                consoleDrawer.drawerOpen = !consoleDrawer.drawerOpen;
            }
        }
    ]

    AppDialog {
        id: errorDialog
        titleStart: i18n("Update Error")
        standardButtons: Kirigami.Dialog.NoButton

        subtitle: ""

        customFooterActions: [
            Kirigami.Action {
                id: errorDialogConfirm
                text: i18n("OK")

                onTriggered: errorDialog.accept()
            }
        ]

        function handleInput(buttonId, button_down) {
            if (!button_down)
                return;
            switch (buttonId) {
            case 0: // A
                errorDialogConfirm.trigger();
                break;
            case 1: // B
                errorDialogConfirm.trigger();
                break;
            }
        }
    }

    FC.FormCard {
        objectName: "updateStatusCard"
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        maximumWidth: Kirigami.Units.gridUnit * 38

        FC.FormTextDelegate {
            objectName: "updateStatus"
            text: AppState.updateRunning ? i18n("Installing updates…")
                : sessionStorage.updateCompleted ? i18n("Updates installed")
                : sessionStorage.updateFailed ? i18n("Update failed")
                : i18n("Ready to update")
            description: AppState.updateRunning ? i18n("You can view the output in Details. Keep this window open until the update finishes.")
                : sessionStorage.updateFailed ? i18n("Open Details to review the error, then try again.")
                : sessionStorage.updateCompleted ? i18n("Restart if required to use the updated system image.")
                : i18n("Install updates for your system image and software using your distribution’s configured updater.")
            icon.name: sessionStorage.updateFailed ? "dialog-error" : "system-software-update"
        }

        FormDelegateSeparatorFixed {}

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: sessionStorage.updateCompleted ? i18n("Completed in this session")
                    : AppData.osRelease["PRETTY_NAME"] || AppData.aboutDataOs.displayName
                color: Kirigami.Theme.disabledTextColor
            }

            QQC2.Button {
                id: updateButton
                objectName: "updateButton"
                action: updateAction
            }
        }

        FormDelegateSeparatorFixed {
            visible: errorUpdateNotFound.visible
        }

        FC.FormTextDelegate {
            id: errorUpdateNotFound
            visible: !(AppConfig.ini.Commands?.systemUpdateCommand || "")
            text: i18n("No update command is configured")
            description: i18n("Check your distribution’s /etc/bazzite-updater/config.ini file.")
            icon.name: "dialog-warning"
        }
    }

    UpdateSteps {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 38
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
    }

    FC.FormCard {
        maximumWidth: Kirigami.Units.gridUnit * 38
        visible: AppData.storageSpace.total > 0

        FC.FormTextDelegate {
            text: i18n("System Drive")
            description: i18nc("percentage of disk space in use", "%1% used", Math.floor(AppData.storageSpace.used * 100 / Math.max(1, AppData.storageSpace.total)))
            icon.name: "drive-harddisk"
        }

        QQC2.ProgressBar {
            objectName: "storageProgress"
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            Layout.minimumHeight: Kirigami.Units.gridUnit
            from: 0
            to: Math.max(1, AppData.storageSpace.total)
            value: AppData.storageSpace.used
            Accessible.name: i18n("System drive space used")
        }
    }

    ConsoleDrawer {
        id: consoleDrawer
        objectName: "updateConsole"
        model: SystemUpdateBackend.consoleModel

        readonly property var __img: AppData.ostree.deployments.find(img => img.isCurrent)
        readonly property int __layeredCount: __img?.layeredPackages?.length || 0

        includeAtStart: {
            const list = ["Bazzite Updater %1".arg(ApplicationVersion)];

            if (RebaseModel.currentImage.name && RebaseModel.currentImage.currentTag)
                list.push(`${RebaseModel.currentImage.name}:${RebaseModel.currentImage.currentTag}`);

            if (OtherUtilsBackend.currentImage.version)
                list.push(OtherUtilsBackend.currentImage.version);

            if (__layeredCount !== 0)
                list.push(i18np("%1 Layered package", "%1 Layered packages", __layeredCount));

            return list.join('\n');
        }

        Item {
            Layout.fillHeight: true
        }

        // displays useful information in a page users usually screenshot when something goes wrong
        QQC2.Label {
            text: consoleDrawer.includeAtStart
            color: systemPalette.placeholderText
        }

        SystemPalette {
            id: systemPalette
            colorGroup: SystemPalette.Active
        }
    }
}
