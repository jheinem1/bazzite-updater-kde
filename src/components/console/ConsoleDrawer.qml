// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import io.github.rfrench3.bazzite_updater
import io.github.rfrench3.controllable as GP

Kirigami.OverlayDrawer {
    id: root

    required property var model
    property alias buttonClipboard: copy
    property string includeAtStart: ""

    edge: Qt.BottomEdge
    modal: false
    drawerOpen: UserSettings.preferConsole
    interactiveResizeEnabled: true
    maximumSize: parent.height - Kirigami.Units.gridUnit * 2.5
    preferredSize: parent.height * 0.5

    function handleInput(buttonId, button_down) {
        if (!button_down)
            return;
        switch (buttonId) {
        case 2: // X
            copy.animateClick();
            break;
        case 3: // Y
            root.close();
            break;
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Item { Layout.fillWidth: true }
                ToolButton {
                    id: copy
                    objectName: "detailsCopyButton"
                    text: i18n("Copy to Clipboard")
                    icon.name: "edit-copy"
                    Accessible.name: text
                    onClicked: {
                        root.model.copyToClipboard(root.includeAtStart + "\n");
                        showPassiveNotification(i18n("Text Copied"), Kirigami.short);
                    }
                }
            }
        }

        GP.ScrollHandler {
            target: consoleView.scrollBar
        }

        ConsoleView {
            id: consoleView
            objectName: "detailsLogView"
            model: root.model
            Layout.fillWidth: true
            Layout.fillHeight: true
            Accessible.name: i18n("Update details log")
        }
    }
}
