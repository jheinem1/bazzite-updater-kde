// SPDX-License-Identifier: GPL-2.0-or-later
// Compiled only by the opt-in development probe, never in normal builds.
#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII
#include "ui_probe.h"
#include "data.h"
#include "system_update.h"
#include "utils.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QTimer>

void startUiProbe(QQmlApplicationEngine &engine, QApplication &app)
{
    const auto output = qEnvironmentVariable("UPDATER_UI_PROBE_DIR");
    if (output.isEmpty())
        return;
    QTimer::singleShot(1200, &app, [&engine, &app, output]() {
        QJsonArray checks;
        bool success = true;
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        auto check = [&](const QString &name, bool passed) {
            checks.append(QJsonObject{{"name", name}, {"passed", passed}});
            success &= passed;
            qInfo().noquote() << (passed ? "PASS" : "FAIL") << name;
        };
        auto evaluate = [&](const QString &expression) {
            QQmlExpression expr(QQmlEngine::contextForObject(window), window, expression);
            QVariant result = expr.evaluate();
            if (expr.hasError()) {
                qWarning() << expr.error();
                success = false;
            }
            return result;
        };
        auto named = [&](const char *name) {
            return window->findChild<QObject *>(QString::fromUtf8(name));
        };
        auto screenshot = [&](const QString &name) {
            QTest::qWait(350);
            check("render " + name, window->grabWindow().save(output + '/' + name + ".png"));
        };
        QDir().mkpath(output);
        // Deterministic display data only. No OS operation is requested by this probe.
        auto *data = AppData::instance();
        data->osRelease = {{"PRETTY_NAME", "Bazzite (preview fixture)"}, {"VERSION_ID", "20261110"}};
        data->storageSpace = {1000, 430};
        Q_EMIT data->osReleaseChanged();
        Q_EMIT data->storageSpaceChanged();
        qInfo() << "Preview palette window" << app.palette().color(QPalette::Window).name();
        window->resize(1080, 720);
        window->requestActivate();
        QTest::qWait(350);
        auto *sidebar = named("navigationSidebar");
        check("sidebar exists", sidebar);
        check("persistent nonmodal sidebar",
              sidebar && sidebar->property("drawerOpen").toBool() && !sidebar->property("modal").toBool() && !sidebar->property("collapsible").toBool());
        check("initial page title", evaluate("root.pageStack.currentItem.title").toString() == "System Update");
        check("clean update action", named("updateAction") && named("updateAction")->property("text").toString() == "Update System and Software");
        screenshot("desktop-overview");
        check("application commands absent from navigation",
              evaluate("globalDrawer.actions.every(action => action !== actionReboot && action !== actionQuit)").toBool());
        QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
        check("Ctrl+F focuses search", named("navigationSearch") && named("navigationSearch")->property("activeFocus").toBool());
        for (const char c : QByteArray("rollback"))
            QTest::keyClick(window, Qt::Key(c - 'a' + Qt::Key_A));
        QTest::qWait(150);
        check("feature search finds utilities", evaluate("globalDrawer.actions[1].visible && !globalDrawer.actions[0].visible").toBool());
        QTest::keyClick(window, Qt::Key_Return);
        QTest::qWait(200);
        check("search result activates with Enter", evaluate("root.pageStack.currentItem.title").toString() == "Other Utilities");
        check("search query remains after activation", named("navigationSearch")->property("text").toString() == "rollback");
        screenshot("search-results");
        QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
        for (int i = 0; i < 8; ++i)
            QTest::keyClick(window, Qt::Key_Z);
        QTest::qWait(150);
        check("empty search result is explained", named("noSearchResults") && named("noSearchResults")->property("visible").toBool());
        screenshot("search-empty");
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(150);
        check("Escape clears search and restores pages",
              named("navigationSearch")->property("text").toString().isEmpty() && evaluate("globalDrawer.actions[0].visible").toBool());
        evaluate("globalDrawer.actions[0].trigger()");
        QTest::keyClick(window, Qt::Key_F10);
        QTest::qWait(250);
        check("F10 opens application menu", named("applicationMenu") && named("applicationMenu")->property("visible").toBool());
        check("menu anchored below hamburger",
              evaluate("applicationMenu.y === applicationMenuButton.height && applicationMenu.x + applicationMenu.width === applicationMenuButton.width")
                  .toBool());
        screenshot("application-menu");
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(250);
        check("Escape dismisses application menu", !named("applicationMenu")->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_F10);
        QTest::keyClick(window, Qt::Key_Down);
        QTest::keyClick(window, Qt::Key_Return);
        QTest::qWait(250);
        check("keyboard menu reboot opens confirmation", evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(250);
        evaluate("GP.Gamepad.buttonEvent(6, true)");
        QTest::qWait(250);
        check("controller Menu opens application menu", named("applicationMenu")->property("visible").toBool());
        evaluate("GP.Gamepad.buttonEvent(0, true)");
        QTest::qWait(250);
        check("controller menu reboot still requires confirmation", evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        evaluate("GP.Gamepad.buttonEvent(1, true)");
        QTest::qWait(250);
        check("controller B cancels reboot confirmation", !evaluate("Boolean(AppDialogManager.activeDialog)").toBool());

        // Merely connecting a controller must not pollute text or change the layout.
        evaluate("GP.Labels.south = '(A)'; GP.Labels.east = '(B)'; GP.Labels.west = '(X)'; GP.Labels.north = '(Y)'");
        check("controller labels do not alter action text", named("updateAction")->property("text").toString() == "Update System and Software");
        evaluate("GP.Gamepad.buttonEvent(1, true)");
        check("B enters controller navigation", sidebar->property("controllerNavigationActive").toBool());
        evaluate("GP.Gamepad.buttonEvent(12, true)");
        QTest::qWait(350);
        check("D-pad changes selected page", evaluate("root.pageStack.currentItem.title").toString() == "Other Utilities");
        evaluate("GP.Gamepad.buttonEvent(0, true)");
        check("A returns controller focus to page", !sidebar->property("controllerNavigationActive").toBool());
        check("controller navigation never hides sidebar", sidebar->property("drawerOpen").toBool());
        screenshot("utilities");
        evaluate("globalDrawer.actions[2].trigger()");
        QTest::qWait(400);
        check("rebase page opens with persistent sidebar",
              evaluate("root.pageStack.currentItem.title").toString() == "Rebase Helper" && sidebar->property("drawerOpen").toBool());
        screenshot("rebase");

        // Walk every configured page in both directions, including boundaries.
        evaluate("globalDrawer.actions[0].trigger()");
        evaluate("globalDrawer.navigatePages(-1)");
        check("top boundary stays on first page", evaluate("root.pageStack.currentItem.title").toString() == "System Update");
        evaluate("globalDrawer.actions[4].trigger()");
        QTest::qWait(400);
        // Index 4 is a separator; it must not replace the page.
        check("separator cannot navigate", evaluate("root.pageStack.currentItem.title").toString() == "System Update");
        evaluate("globalDrawer.actions[5].trigger()");
        QTest::qWait(400);
        check("settings page opens", evaluate("root.pageStack.currentItem.title").toString() == "Settings");
        screenshot("settings");
        evaluate("globalDrawer.actions[7].trigger()");
        QTest::qWait(400);
        const auto aboutTitle = evaluate("root.pageStack.currentItem.title").toString();
        evaluate("globalDrawer.navigatePages(1)");
        check("last page boundary skips reboot and quit", evaluate("root.pageStack.currentItem.title").toString() == aboutTitle);
        screenshot("about");

        evaluate("globalDrawer.actions[0].trigger()");
        QTest::qWait(350);
        QTest::keyClick(window, Qt::Key_F12);
        QTest::qWait(350);
        check("F12 opens details", named("updateConsole") && named("updateConsole")->property("drawerOpen").toBool());
        screenshot("details");
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(350);
        check("Escape closes details", !named("updateConsole")->property("drawerOpen").toBool());
        check("Escape keeps sidebar visible", sidebar->property("drawerOpen").toBool());
        QTest::keyClick(window, Qt::Key_M, Qt::ControlModifier);
        QTest::keyClick(window, Qt::Key_Return);
        check("Return leaves navigation without starting update", !sidebar->property("controllerNavigationActive").toBool() && !appState.commandRunning());

        for (int i = 0; i < 20; ++i)
            QTest::keyClick(window, Qt::Key_Tab);
        check("Tab traversal retains a focus target", window->activeFocusItem() != nullptr);
        auto *backend = engine.singletonInstance<SystemUpdateBackend *>("io.github.rfrench3.bazzite_updater", "SystemUpdateBackend");
        backend->m_updateStepsModel->updateData("system image", 2, 5, UpdateStepsNS::RUNNING);
        backend->m_updateStepsModel->updateData("applications", 3, 3, UpdateStepsNS::SUCCESS);
        appState.setUpdateRunning(true);
        QTest::qWait(200);
        check("update action disabled while command runs", !named("updateAction")->property("enabled").toBool());
        evaluate("actionReboot.trigger()");
        check("disabled reboot action cannot open confirmation", !evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        screenshot("running-state");
        evaluate("GP.Gamepad.buttonEvent(6, true)");
        QTest::qWait(250);
        check("controller menu skips disabled reboot", evaluate("applicationMenu.currentIndex === 1 && !applicationMenu.itemAt(0).enabled").toBool());
        evaluate("GP.Gamepad.buttonEvent(11, true)");
        check("controller cannot select disabled reboot", evaluate("applicationMenu.currentIndex === 1").toBool());
        evaluate("GP.Gamepad.buttonEvent(0, true)");
        QTest::qWait(200);
        check("quit while running opens warning", evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        check("early exit confirmation disabled", !evaluate("AppDialogManager.activeDialog.customFooterActions[0].enabled").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(300);
        check("cancel quit preserves running state", appState.commandRunning() && !evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        backend->m_updateStepsModel->clearData();
        appState.setUpdateRunning(false);
        evaluate("sessionStorage.updateFailed = true");
        QTest::qWait(150);
        check("failure stays visible", named("updateStatus")->property("text").toString() == "Update failed");
        screenshot("failure-state");
        evaluate("sessionStorage.updateFailed = false");
        evaluate("actionReboot.trigger()");
        QTest::qWait(250);
        check("reboot opens confirmation only", evaluate("Boolean(AppDialogManager.activeDialog)").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTest::qWait(300);
        check("Escape cancels confirmation", !evaluate("Boolean(AppDialogManager.activeDialog)").toBool());

        evaluate("sessionStorage.updateCompleted = true; sessionStorage.updateFailed = false");
        appState.setCommandSucceeded(false); // Allow a synthetic retry after a prior successful session state.
        auto *retryButton = qobject_cast<QQuickItem *>(named("updateButton"));
        check("retry button exists", retryButton != nullptr);
        if (retryButton) {
            QTest::mouseClick(window,
                              Qt::LeftButton,
                              Qt::NoModifier,
                              retryButton->mapToScene(QPointF(retryButton->width() / 2, retryButton->height() / 2)).toPoint());
            QTest::qWait(500);
        }
        check("retry clears previous success", !evaluate("Boolean(sessionStorage.updateCompleted)").toBool());
        check("failed retry displays latest outcome",
              evaluate("Boolean(sessionStorage.updateFailed)").toBool() && named("updateStatus")->property("text").toString() == "Update failed");
        evaluate("sessionStorage.updateFailed = false");
        // Dismiss passive feedback before the clean responsive previews.
        evaluate("root.hidePassiveNotification()");
        QTest::qWait(400);
        data->storageSpace = {1000, 430};
        Q_EMIT data->storageSpaceChanged();
        window->resize(840, 600);
        QTest::qWait(200);
        auto *button = qobject_cast<QQuickItem *>(named("updateButton"));
        check("narrow update control fits page", button && button->width() > 100 && button->mapToScene(QPointF(button->width(), 0)).x() <= window->width() + 1);
        screenshot("narrow-desktop");
        QFont font = app.font();
        font.setPointSize(14);
        app.setFont(font);
        window->resize(qMax(960, window->minimumWidth()), qMax(700, window->minimumHeight()));
        QTest::qWait(400);
        check("large text sidebar remains visible", sidebar->property("drawerOpen").toBool());
        screenshot("large-text");
        QFile report(output + "/ui-report.json");
        if (!report.open(QIODevice::WriteOnly)) {
            app.exit(2);
            return;
        }
        report.write(QJsonDocument(QJsonObject{{"passed", success},
                                               {"checks", checks},
                                               {"scope", "Offscreen Qt/Kirigami render and synthetic input/state regression; no real update/rebase/reboot."}})
                         .toJson());
        report.close();
        app.exit(success ? 0 : 1);
    });
}
