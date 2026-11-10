#pragma once

#include <QDateTime>
#include <QLocale>
#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qjsengine.h>
#include <qjsonobject.h>
#include <qjsvalue.h>
#include <qlist.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include <QtConcurrent>
#include <qtypes.h>

using namespace Qt::StringLiterals;

struct dataStatus {
    bool loading = false;
    bool succeeded = false;
};

namespace Gpu
{
Q_NAMESPACE
QML_ELEMENT

enum class Drivers {
    UNKNOWN = -1,
    BASE,
    NVIDIA,
    NVIDIA_OPEN,
    UNSUPPORTED
};

Q_ENUM_NS(Drivers)
}

struct GpuDrivers {
    Q_GADGET;
    QML_VALUE_TYPE(gpu);

public:
    Gpu::Drivers best = Gpu::Drivers::UNKNOWN;
    Q_PROPERTY(Gpu::Drivers best READ getBest CONSTANT)
    Gpu::Drivers getBest() const
    {
        return best;
    }

    static GpuDrivers load();
};

struct OstreeDeployment {
    Q_GADGET;
    QML_VALUE_TYPE(ostreeDeployment);

public:
    QStringList layeredPackages;
    QStringList getLayeredPackages() const
    {
        return layeredPackages;
    }
    bool isCurrent = false;
    bool getIsCurrent() const
    {
        return isCurrent;
    }
    bool isStaged = false;
    bool isPending = false;
    bool getIsStaged() const
    {
        return isStaged;
    }
    bool present = false;
    QString reference;
    QString name;
    QString repository;
    QString tag;
    QString digest;
    QString version;
    qint64 timestamp = 0;
    Q_PROPERTY(bool present MEMBER present CONSTANT)
    Q_PROPERTY(QString reference MEMBER reference CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QString repository MEMBER repository CONSTANT)
    Q_PROPERTY(QString tag MEMBER tag CONSTANT)
    Q_PROPERTY(QString digest MEMBER digest CONSTANT)
    Q_PROPERTY(QString version MEMBER version CONSTANT)
    Q_PROPERTY(QString buildDate READ getBuildDate CONSTANT)
    QString getBuildDate() const
    {
        return timestamp > 0 ? QLocale().toString(QDateTime::fromSecsSinceEpoch(timestamp).date(), QLocale::LongFormat) : QString();
    }
    Q_PROPERTY(QStringList layeredPackages READ getLayeredPackages CONSTANT)
    Q_PROPERTY(bool isCurrent READ getIsCurrent CONSTANT)
    Q_PROPERTY(bool isStaged READ getIsStaged CONSTANT)
    Q_PROPERTY(bool isPending MEMBER isPending CONSTANT)
    bool operator==(const OstreeDeployment &) const = default;
};

struct Ostree {
    Q_GADGET;
    QML_VALUE_TYPE(ostree);

public:
    QList<OstreeDeployment> deployments;
    Q_PROPERTY(QList<OstreeDeployment> deployments READ getDeployments CONSTANT)
    const QList<OstreeDeployment> &getDeployments() const
    {
        return deployments;
    }
    Q_INVOKABLE bool onLatestDeployment() const
    {
        return getBootedDeployment().present && !hasPendingDeployment();
    }

    Q_PROPERTY(OstreeDeployment bootedDeployment READ getBootedDeployment CONSTANT)
    Q_PROPERTY(OstreeDeployment pendingDeployment READ getPendingDeployment CONSTANT)
    OstreeDeployment getBootedDeployment() const
    {
        for (const auto &deployment : deployments)
            if (deployment.isCurrent)
                return deployment;
        return {};
    }
    OstreeDeployment getPendingDeployment() const
    {
        for (const auto &deployment : deployments)
            if (deployment.isPending && !deployment.isCurrent)
                return deployment;
        return {};
    }
    Q_INVOKABLE bool hasPendingDeployment() const
    {
        return getPendingDeployment().present;
    }
    static Ostree load();
    bool operator==(const Ostree &) const = default;
};

struct StorageSpace {
    Q_GADGET
    QML_VALUE_TYPE(storageSpace)
public:
    qint64 total = 0;
    qint64 used = 0;
    Q_PROPERTY(qint64 total MEMBER total CONSTANT)
    Q_PROPERTY(qint64 used MEMBER used CONSTANT)

    static StorageSpace load();
};

// Asynchronously loads data
class AppData : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_DISABLE_COPY_MOVE(AppData)
    AppData(QObject *parent = nullptr);

public: // singleton methods
    static AppData *instance()
    {
        static AppData *s_instance = new AppData();
        return s_instance;
    }

    static AppData *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine)
        AppData *instancePtr = instance();
        qmlEngine->setObjectOwnership(instancePtr, QQmlEngine::CppOwnership);
        return instancePtr;
    }

public:
    Q_PROPERTY(Ostree ostree READ getOstree NOTIFY ostreeChanged)
    Q_PROPERTY(bool ostreeLoading READ isOstreeLoading NOTIFY ostreeChanged)
    bool isOstreeLoading() const
    {
        return ostreeStatus.loading;
    }
    const Ostree &getOstree() const
    {
        return ostree;
    }
    Ostree ostree;
    dataStatus ostreeStatus{};
    Q_INVOKABLE void reloadOstree(QJSValue callback = QJSValue());
    Q_SIGNAL void ostreeChanged();

    Q_PROPERTY(GpuDrivers gpu READ getGpu NOTIFY gpuChanged)
    const GpuDrivers &getGpu() const
    {
        return gpu;
    }
    GpuDrivers gpu;
    void loadBestDrivers();
    Q_SIGNAL void gpuChanged();

    //   /etc/os-release
    Q_PROPERTY(QJsonObject osRelease READ getOsRelease NOTIFY osReleaseChanged)
    QJsonObject osRelease;
    dataStatus osReleaseStatus{};
    const QJsonObject &getOsRelease() const
    {
        return osRelease;
    }
    Q_SIGNAL void osReleaseChanged();
    void loadOsRelease();

    // partially depends on os-release, so it runs in that same thread
    Q_PROPERTY(QJsonObject aboutDataOs READ getAboutDataOs NOTIFY aboutDataOsChanged)
    QJsonObject aboutDataOs;
    dataStatus aboutDataOsStatus{};
    const QJsonObject &getAboutDataOs() const
    {
        return aboutDataOs;
    }
    Q_SIGNAL void aboutDataOsChanged();
    void loadAboutDataOs(const QJsonObject &osRelease); // only run after os-release is completed

    Q_PROPERTY(StorageSpace storageSpace READ getStorageSpace NOTIFY storageSpaceChanged)
    StorageSpace storageSpace;
    const StorageSpace &getStorageSpace() const
    {
        return storageSpace;
    }
    Q_SIGNAL void storageSpaceChanged();
    void loadStorageSpace();
};
inline static AppData *appData = AppData::instance();

QJsonObject runJson(const QString &program, const QStringList &args);

// (ex: u"bazzite-updater/config.ini"_s) Returns the full path to the config file that should be used.
QString findConfigFile(const QString &relativePath);
