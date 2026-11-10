// SPDX-FileCopyrightText: 2025-2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <KirigamiAddons/App/kirigamiappdefaults.h>
#include <QApplication>
#include <QtGlobal>

#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QScreen>
#include <QStyleHints>
#include <QUrl>

#include "data.h"
#include "version-bazzite-updater.h"
#include <KAboutData>
#include <KColorSchemeManager>
#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KirigamiAddons/App/KirigamiAppDefaults>
#include <cstdlib>
#include <iostream>
#include <kaboutdata.h>
#include <kcolorschememanager.h>
#include <ostream>
#include <qapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqml.h>

#include <qqmlpropertymap.h>
#include <qquickstyle.h>
#include <qstylehints.h>
#include <qtpreprocessorsupport.h>
#include <unistd.h>

#include "k_config.h"
#include "utils.h"
#ifdef BAZZITE_UPDATER_UI_PROBE
#include "../tests/ui_probe.h"
#include <KColorScheme>
#include <KSharedConfig>
#endif

inline constexpr float BASE_HEIGHT = 1080.0;

using namespace Qt::Literals::StringLiterals;

// Handle non-gui functionality: Replace the bazzite-updater process with the selected process defined by the config.ini
void commandLine(char *argv[], QCoreApplication &app);

void setup_theming();

int main(int argc, char *argv[])
{
    bool UseFullscreen = false;

    QApplication app(argc, argv);

    if (Utils::GAMESCOPE_SESSION) {
        /*
            Gamescope-session does not apply any app scaling.
            This ensures it is scaled well on high-DPI, while respecting regular desktop settings.

            This may behave strangely in multi-monitor environments, but gamescope should keep those hidden from the app.
            Example: kde window rules to move the app to a certain display seem to break this scaling check
        */
        if (qEnvironmentVariableIsEmpty("QT_SCALE_FACTOR")) {
            QScreen *screen = app.primaryScreen();
            const float height = screen->geometry().height();
            const float scale_factor = height / BASE_HEIGHT;

            // scale by a minimum of 2X for mobile and TV usage
            float final_scale = 2;
            if (scale_factor > 1.0)
                final_scale *= scale_factor;
            qputenv("QT_SCALE_FACTOR", QByteArray::number(final_scale));
            execv("/proc/self/exe", argv);
        }

        UseFullscreen = true;
    }

    KirigamiAppDefaults::apply(&app);

    // exits early if the command line options are used
    commandLine(argv, app);

    setup_theming();
#ifdef BAZZITE_UPDATER_UI_PROBE
    const QString previewScheme = qEnvironmentVariable("UPDATER_UI_PROBE_SCHEME");
    if (!previewScheme.isEmpty()) {
        app.setProperty("KDE_COLOR_SCHEME_PATH", previewScheme);
        app.setPalette(KColorScheme::createApplicationPalette(KSharedConfig::openConfig(previewScheme)));
    }
#endif

    const QString FULL_APP_DOMAIN = u"io.github.rfrench3.bazzite-updater"_s;
    KLocalizedString::setApplicationDomain("bazzite-updater");
    QCoreApplication::setOrganizationName(u"UniversalBlue"_s);

    KAboutData aboutData(u"bazzite-updater"_s,
                         i18n("Bazzite Updater KDE"),
                         QStringLiteral(BAZZITE_UPDATER_VERSION_STRING),
                         i18n("Desktop-focused updating and rebasing utility for Bazzite"),
                         KAboutLicense::Unknown, // Can't directly set v2-or-later here
                         i18n("© 2025-2026 Robert French"),
                         i18n("An independent desktop-focused fork. Contributions are currently closed; free-software license rights are unchanged."),
                         u"https://github.com/jheinem1/bazzite-updater-kde"_s,
                         QString());
    aboutData.setLicense(KAboutLicense::GPL_V2, KAboutLicense::OrLaterVersions);
    aboutData.addAuthor(u"James Heinemann"_s, i18n("Desktop-focused fork"));
    aboutData.addAuthor(u"Robert French"_s, i18n("Original Bazzite Updater"));
    aboutData.addCredit(u"Gareth Widlansky"_s, u"Uupd integration"_s, u""_s, u"https://github.com/gerblesh"_s);
    aboutData.setDesktopFileName(FULL_APP_DOMAIN);
    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setWindowIcon(QIcon::fromTheme(FULL_APP_DOMAIN));
    QGuiApplication::setDesktopFileName(FULL_APP_DOMAIN);

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    // For non-critical data storage
    auto sessionStorage = QQmlPropertyMap::create(&app);
    engine.rootContext()->setContextProperty(u"sessionStorage"_s, sessionStorage);

    engine.rootContext()->setContextProperty(u"UseFullscreen"_s, UseFullscreen);

    engine.rootContext()->setContextProperty(u"ApplicationVersion"_s, QStringLiteral(BAZZITE_UPDATER_VERSION_STRING));

    engine.loadFromModule("io.github.rfrench3.bazzite_updater", u"Main"_s);

    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    // load AppData on startup instead of on first access
    AppData::instance();

#ifdef BAZZITE_UPDATER_UI_PROBE
    startUiProbe(engine, app);
#endif

    return app.exec();
}

void commandLine(char *argv[], QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption update(u"update"_s, u"Performs the configured system update command."_s);
    QCommandLineOption rollback(u"rollback"_s, u"Performs the configured system rollback command."_s);

    parser.addOption(update);
    parser.addOption(rollback);

    parser.process(app);

    // Run the GUI normally if neither --update or --rollback are present
    if (!(parser.isSet(update) || parser.isSet(rollback))) {
        std::cout << "Note: you can run \"" << argv[0] << " --help\" to view command-line functionality for this application." << std::endl;
        return;
    }

    if (parser.isSet(update) && parser.isSet(rollback)) {
        std::cerr << "You cannot use --update and --rollback at the same time." << std::endl;
        exit(EXIT_FAILURE);
    }

#ifdef BAZZITE_UPDATER_UI_PROBE
    std::cerr << "System commands are disabled in the UI probe build." << std::endl;
    exit(EXIT_FAILURE);
#endif

    auto cmd = QString();

    if (parser.isSet(update))
        cmd = configIni.getValue(u"Commands"_s, u"systemUpdateCommand"_s);

    if (parser.isSet(rollback))
        cmd = configIni.getValue(u"Commands"_s, u"systemRollbackCommand"_s);

    if (cmd.isEmpty()) {
        std::cerr << "Command not defined." << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto args = QProcess::splitCommand(cmd);

    // prepare arguments for execvp
    QList<char *> execArgs;
    execArgs.reserve(args.size() + 1);

    for (const QString &arg : args)
        execArgs.append(qstrdup(arg.toLocal8Bit().constData()));

    execArgs.append(nullptr);

    std::cout << ">" << args.join(u" "_s).toStdString() << std::endl;

    execvp(execArgs.first(), execArgs.data());

    perror("execvp failed");
    exit(EXIT_FAILURE);
}

void setup_theming()
{
    if (!Utils::KDE_SESSION) {
        // If the Qt style is manually being set by anything, don't mess with it
        if (qEnvironmentVariableIsSet("QT_QPA_PLATFORMTHEME") || qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")
            || qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE"))
            return;

        QQuickStyle::setStyle(u"org.kde.breeze"_s);
        new Utils::DbusListener(qGuiApp);
    }
}
