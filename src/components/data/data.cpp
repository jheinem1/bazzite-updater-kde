#include "data.h"
#include "image_identity.h"
#include <qchar.h>
#include <qcontainerfwd.h>
#include <qjsonobject.h>
#include <qtconcurrentrun.h>
#include <qtmetamacros.h>

AppData::AppData(QObject *parent)
    : QObject(parent)
{
    reloadOstree();
    loadBestDrivers();
    loadOsRelease();
    loadStorageSpace();
};

StorageSpace StorageSpace::load()
{
    for (const QString &path : {u"/sysroot"_s, u"/var"_s}) {
        if (!QDir(path).exists())
            continue;

        QStorageInfo info(path);
        if (!info.isValid() || !info.isReady())
            continue;

        return StorageSpace{info.bytesTotal(), info.bytesTotal() - info.bytesFree()};
    }
    return {};
}

void AppData::loadStorageSpace()
{
    QtConcurrent::run([]() {
        return StorageSpace::load();
    }).then(this, [this](StorageSpace data) {
        storageSpace = data;
        Q_EMIT storageSpaceChanged();
    });
}

// (ex: u"bazzite-updater/config.ini"_s) Returns the full path to the config file that should be used.
QString findConfigFile(const QString &relativePath)
{
    QStringList bases;

    /* do NOT check the home config path. Any app with home folder access could
     * run arbitrary code in place of the normal system update. Only look for
     * configs in the standard places under /etc and /usr
     */

    // bases.append(qEnvironmentVariable("XDG_CONFIG_HOME", QDir::homePath() + u"/.config"_s));

    bases.append({u"/etc"_s, u"/usr/local/etc"_s, u"/usr/etc"_s});

    // append XDG_CONFIG_DIRS last because any user (or malicious program run by the user) can change it
    if (auto xdg_config = qEnvironmentVariable("XDG_CONFIG_DIRS"); xdg_config.isEmpty())
        bases.append(u"/etc/xdg"_s);
    else
        bases.append(xdg_config.split(u':', Qt::SkipEmptyParts));

    for (const QString &dir : bases) {
        const QString path = QDir(dir).filePath(relativePath);
        QFile file(path);

        if (file.exists()) {
            if (file.open(QIODevice::ReadOnly))
                return path;
            else
                qWarning() << "Config found at " << path << " but could not open: " << file.errorString();
        }
    }
    qWarning() << "App Config file not found: " << relativePath;
    return u""_s;
}

void AppData::loadBestDrivers()
{
    QtConcurrent::run([]() {
        return GpuDrivers::load();
    }).then(this, [this](GpuDrivers g) {
        this->gpu = std::move(g);
        Q_EMIT gpuChanged();
    });
}

GpuDrivers GpuDrivers::load()
{
    const auto path = u"/usr/libexec/bazzite-detect-nvidia-support-status"_s;

    if (!QFile::exists(path))
        return {};

    GpuDrivers result;

    QProcess proc;

    proc.start(path);
    if (!proc.waitForFinished(15000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        return {};

    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    if (output == u"supported"_s)
        result.best = Gpu::Drivers::NVIDIA_OPEN;
    else if (output == u"legacy"_s)
        result.best = Gpu::Drivers::NVIDIA;
    else if (output == u"unsupported"_s)
        result.best = Gpu::Drivers::UNSUPPORTED;
    else if (output.isEmpty())
        result.best = Gpu::Drivers::BASE;
    else
        result.best = Gpu::Drivers::UNKNOWN;

    return result;
}

void AppData::reloadOstree(QJSValue callback)
{
    if (ostreeStatus.loading)
        return;

    ostreeStatus.loading = true;
    Q_EMIT ostreeChanged();
    QtConcurrent::run([]() {
        return Ostree::load();
    })
        .then(this,
              [this, callback](Ostree os) {
                  this->ostree = std::move(os);
                  this->ostreeStatus.succeeded = !this->ostree.deployments.isEmpty();
                  this->ostreeStatus.loading = false;
                  Q_EMIT ostreeChanged();
                  if (callback.isCallable())
                      callback.call();
              })
        .onCanceled(this, [this, callback]() {
            this->ostree = {};
            this->ostreeStatus.loading = false;
            Q_EMIT ostreeChanged();
            if (callback.isCallable())
                callback.call();
        });
}

QJsonObject runJson(const QString &program, const QStringList &args)
{
    QProcess proc;
    proc.start(program, args);

    if (!proc.waitForStarted(5000)) {
        qWarning() << "Failed to start process:" << program << args << "| Error:" << proc.errorString();
        return {};
    }

    if (!proc.waitForFinished(15000)) {
        qWarning() << "Process timed out after 15s:" << program << args;
        proc.kill();
        return {};
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        const QString stdErr = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        qWarning() << "Process exited with error:" << program << args << "| Exit status:" << proc.exitStatus() << "| Exit code:" << proc.exitCode()
                   << "| stderr:" << stdErr;
        return {};
    }

    return QJsonDocument::fromJson(proc.readAllStandardOutput()).object();
}

static Ostree fromDeploymentImages(const QList<ImageIdentity::Deployment> &images)
{
    Ostree result;
    for (const auto &image : images) {
        OstreeDeployment deployment;
        deployment.present = true;
        deployment.isCurrent = image.booted;
        deployment.isStaged = image.staged;
        deployment.isPending = image.pending;
        deployment.layeredPackages = image.layeredPackages;
        deployment.reference = image.reference;
        deployment.name = image.name;
        deployment.repository = image.repository;
        deployment.tag = image.tag;
        deployment.digest = image.digest;
        deployment.version = image.version;
        deployment.timestamp = image.timestamp;
        result.deployments.append(deployment);
    }
    return result;
}

Ostree Ostree::load()
{
    if (!QStandardPaths::findExecutable(u"rpm-ostree"_s).isEmpty()) {
        const auto result = fromDeploymentImages(ImageIdentity::fromRpmOstree(runJson(u"rpm-ostree"_s, {u"status"_s, u"--json"_s})));
        if (!result.deployments.isEmpty())
            return result;
    }
    if (!QStandardPaths::findExecutable(u"bootc"_s).isEmpty())
        return fromDeploymentImages(ImageIdentity::fromBootc(runJson(u"bootc"_s, {u"status"_s, u"--json"_s})));
    return {};
}

void AppData::loadOsRelease()
{
    if (osReleaseStatus.loading)
        return;

    osReleaseStatus.loading = true;
    QtConcurrent::run([this]() {
        QJsonObject data = {};
        auto file = QFile(u"/etc/os-release"_s);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                QByteArray arr = file.readLine();

                auto line = QString::fromUtf8(arr).trimmed();

                if (line.isEmpty() || line.startsWith(u'#'))
                    continue;

                int split = line.indexOf(u'=');
                if (split == -1)
                    continue;

                auto key = line.left(split);
                auto value = line.mid(split + 1);

                // Remove surrounding quotations, if there are any
                if (value.length() >= 2 && ((value.startsWith(u'"') && value.endsWith(u'"')) || (value.startsWith(u'\'') && value.endsWith(u'\'')))) {
                    value = value.sliced(1, value.length() - 2);
                }

                data.insert(key, value);
            }
        }

        loadAboutDataOs(data);

        return data;
    })
        .then(this,
              [this](QJsonObject data) {
                  if (data != QJsonObject()) {
                      this->osRelease = std::move(data);
                      Q_EMIT osReleaseChanged();
                      this->osReleaseStatus.succeeded = true;
                  }
                  this->osReleaseStatus.loading = false;
                  Q_EMIT aboutDataOsChanged();
              })
        .onCanceled(this, [this]() {
            this->osReleaseStatus.loading = false;
            Q_EMIT aboutDataOsChanged();
        });
}

void AppData::loadAboutDataOs(const QJsonObject &osRelease)
{
    aboutDataOsStatus.loading = true;

    QFile aboutData(findConfigFile(u"bazzite-updater/KAboutData_OS.json"_s));
    if (!aboutData.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open KAboutData_OS";
        aboutDataOs[u"displayName"_s] = u"Failed to fetch information."_s;
    } else {
        QByteArray file = aboutData.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(file);
        aboutDataOs = doc.object();

        // Process specific values
        if (aboutDataOs[u"version"_s].toString() == u"@VERSION@"_s) {
            if (osRelease.contains(u"VERSION"_s))
                aboutDataOs.insert(u"version"_s, osRelease[u"VERSION"_s]);
            else if (osRelease.contains(u"VERSION_ID"_s))
                aboutDataOs.insert(u"version"_s, osRelease[u"VERSION_ID"_s]);
            else
                aboutDataOs.insert(u"version"_s, u"Unknown version"_s);
        }

        if (aboutDataOs[u"license"_s].toString() == u"@Apache-2.0@"_s) {
            // TODO: insert the entire apache 2.0 license
            aboutDataOs[u"license"_s] = u"https://www.apache.org/licenses/LICENSE-2.0.txt"_s;
        }

        // If the desired icon does not exist, allow QML to fallback to qrc:/fallbackIcon
        if (!QFile(aboutDataOs[u"programLogo"_s].toString()).exists())
            aboutDataOs[u"programLogo"_s] = u""_s;

        auto copyright = aboutDataOs[u"copyrightStatement"_s].toString();
        int idx = copyright.indexOf(u"@CURRENT_YEAR@"_s);
        if (idx != -1) {
            copyright.replace(idx, 14, QString::number(QDate::currentDate().year()));
            aboutDataOs.insert(u"copyrightStatement"_s, copyright);
        }

        aboutData.close();
        aboutDataOsStatus.succeeded = true;
    }

    aboutDataOsStatus.loading = false;
}
