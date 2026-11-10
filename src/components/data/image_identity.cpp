// SPDX-License-Identifier: GPL-2.0-or-later

#include "image_identity.h"

#include <QDateTime>
#include <QJsonArray>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace
{
bool hasContainerTransport(const QString &reference)
{
    return reference.startsWith(u"ostree-unverified-registry:"_s) || reference.startsWith(u"ostree-image-signed:"_s)
        || reference.startsWith(u"ostree-unverified-image:"_s) || reference.startsWith(u"ostree-remote-registry:"_s)
        || reference.startsWith(u"ostree-remote-image:"_s) || reference.startsWith(u"docker://"_s) || reference.startsWith(u"registry:"_s);
}

// Mirrors the registry/signature forms accepted by ostree-ext, including a
// named OSTree signing remote. Local archives and directory transports are not
// registry repositories and must not be mistaken for one.
QString registryReference(QString reference)
{
    bool needsTransport = false;
    if (reference.startsWith(u"ostree-unverified-registry:"_s)) {
        reference.remove(0, QStringView(u"ostree-unverified-registry:").size());
    } else if (reference.startsWith(u"ostree-remote-registry:"_s) || reference.startsWith(u"ostree-remote-image:"_s)) {
        needsTransport = reference.startsWith(u"ostree-remote-image:"_s);
        reference = reference.mid(reference.indexOf(u':') + 1);
        const qsizetype remoteEnd = reference.indexOf(u':');
        if (remoteEnd <= 0)
            return {};
        reference = reference.mid(remoteEnd + 1);
    } else if (reference.startsWith(u"ostree-image-signed:"_s) || reference.startsWith(u"ostree-unverified-image:"_s)) {
        needsTransport = true;
        reference = reference.mid(reference.indexOf(u':') + 1);
    } else if (reference.startsWith(u"ostree-"_s)) {
        return {};
    }

    if (reference.startsWith(u"docker://"_s)) {
        reference.remove(0, QStringView(u"docker://").size());
    } else if (reference.startsWith(u"registry:"_s)) {
        reference.remove(0, QStringView(u"registry:").size());
    } else if (needsTransport) {
        return {};
    }

    for (const auto &transport : {u"oci:"_s, u"oci-archive:"_s, u"docker-archive:"_s, u"containers-storage:"_s, u"dir:"_s, u"http:"_s, u"https:"_s,
                                  u"docker:"_s}) {
        if (reference.startsWith(transport))
            return {};
    }
    return reference;
}

bool validRepository(const QString &repository)
{
    // The last ':' in an image reference denotes a tag only after the last '/'.
    // A registry's port (including an IPv6 registry) belongs to its first component.
    static const QRegularExpression component(u"^[a-z0-9]+(?:(?:[._]|__|-+)[a-z0-9]+)*$"_s);
    static const QRegularExpression registry(u"^(?:[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?|\\[[0-9A-Fa-f:]+\\])(?::[0-9]+)?$"_s);
    const QStringList parts = repository.split(u'/');
    qsizetype firstPath = 0;
    if (parts.size() > 1 && (parts.first().contains(u'.') || parts.first().contains(u':') || parts.first() == u"localhost"_s)) {
        if (!registry.match(parts.first()).hasMatch())
            return false;
        firstPath = 1;
    }
    for (qsizetype i = firstPath; i < parts.size(); ++i) {
        if (!component.match(parts.at(i)).hasMatch())
            return false;
    }
    return true;
}

qint64 timestampSeconds(const QJsonValue &value)
{
    if (value.isDouble())
        return qMax(qint64(0), value.toInteger());
    if (!value.isString())
        return 0;

    // bootc uses RFC3339; do not interpret timezone-less input in the local zone.
    static const QRegularExpression rfc3339(u"^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}(?:\\.\\d+)?(?:Z|[+-]\\d{2}:\\d{2})$"_s);
    const QString text = value.toString();
    if (!rfc3339.match(text).hasMatch())
        return 0;
    const QDateTime date = QDateTime::fromString(text, Qt::ISODateWithMs);
    return date.isValid() ? qMax(qint64(0), date.toSecsSinceEpoch()) : 0;
}

void appendPackages(QStringList &packages, const QJsonValue &value)
{
    for (const QJsonValue &package : value.toArray()) {
        if (package.isString() && !package.toString().isEmpty())
            packages.append(package.toString());
    }
}

bool hasDeploymentMarker(const QJsonObject &source)
{
    if (source.value(u"booted"_s).isBool() || source.value(u"staged"_s).isBool())
        return true;
    for (const QString &key : {u"container-image-reference"_s, u"container-image-reference-digest"_s, u"origin"_s, u"checksum"_s, u"id"_s}) {
        if (!source.value(key).toString().trimmed().isEmpty())
            return true;
    }
    return false;
}
}

namespace ImageIdentity
{
Deployment fromReference(const QString &raw)
{
    Deployment result;
    result.sourceReference = raw;
    result.reference = raw.trimmed();
    const QString reference = registryReference(result.reference);
    if (reference.isEmpty())
        return result;

    QString repository = reference;
    QString digest;
    QString tag;
    const qsizetype digestStart = repository.indexOf(u'@');
    if (digestStart >= 0) {
        static const QRegularExpression digestPattern(u"^[A-Za-z][A-Za-z0-9]*(?:[+._-][A-Za-z][A-Za-z0-9]*)*:[A-Za-z0-9=_-]+$"_s);
        digest = repository.mid(digestStart + 1);
        repository.truncate(digestStart);
        if (!digestPattern.match(digest).hasMatch())
            return result;
    }

    const qsizetype tagStart = repository.lastIndexOf(u':');
    if (tagStart > repository.lastIndexOf(u'/')) {
        static const QRegularExpression tagPattern(u"^[A-Za-z0-9_][A-Za-z0-9_.-]{0,127}$"_s);
        tag = repository.mid(tagStart + 1);
        repository.truncate(tagStart);
        if (!tagPattern.match(tag).hasMatch())
            return result;
    }
    if (!validRepository(repository))
        return result;

    result.reference = reference;
    result.repository = repository;
    result.name = repository.section(u'/', -1);
    result.tag = tag;
    result.digest = digest;
    return result;
}

QList<Deployment> fromRpmOstree(const QJsonObject &root)
{
    QList<Deployment> result;
    for (const QJsonValue &value : root.value(u"deployments"_s).toArray()) {
        if (!value.isObject())
            continue;
        const QJsonObject source = value.toObject();
        // Empty or unrelated objects are not deployment slots. Keeping one at
        // index 0 would falsely make it the next-boot target ahead of booted.
        if (!hasDeploymentMarker(source))
            continue;
        const QString containerReference = source.value(u"container-image-reference"_s).toString();
        Deployment deployment;
        if (!containerReference.trimmed().isEmpty()) {
            deployment = fromReference(containerReference);
        } else {
            // Traditional OSTree refs are not OCI references, even if a bare ref
            // happens to look like an image path. Never invent a container name.
            const QString origin = source.value(u"origin"_s).toString();
            if (hasContainerTransport(origin.trimmed()))
                deployment = fromReference(origin);
            else {
                deployment.sourceReference = origin;
                deployment.reference = origin.trimmed();
            }
        }
        deployment.booted = source.value(u"booted"_s).toBool();
        deployment.staged = source.value(u"staged"_s).toBool();
        const QString digest = source.value(u"container-image-reference-digest"_s).toString();
        if (!digest.isEmpty())
            deployment.digest = digest;
        deployment.version = source.value(u"version"_s).toString();
        deployment.timestamp = timestampSeconds(source.value(u"timestamp"_s));
        appendPackages(deployment.layeredPackages, source.value(u"requested-packages"_s));
        appendPackages(deployment.layeredPackages, source.value(u"requested-local-packages"_s));
        result.append(deployment);
    }

    // rpm-ostree lists deployments in boot order. A finalized update need not
    // be staged any longer, but is pending when it precedes the booted entry.
    // Without a booted marker, only an explicit staged role establishes this.
    qsizetype bootedIndex = -1;
    for (qsizetype i = 0; i < result.size(); ++i) {
        if (result.at(i).booted) {
            bootedIndex = i;
            break;
        }
    }
    if (bootedIndex > 0) {
        result.first().pending = true;
    } else if (bootedIndex < 0) {
        for (Deployment &deployment : result) {
            if (deployment.staged) {
                deployment.pending = true;
                break;
            }
        }
    }
    return result;
}

QList<Deployment> fromBootc(const QJsonObject &root)
{
    QList<Deployment> result;
    const QJsonObject status = root.value(u"status"_s).toObject();
    const bool rollbackQueued = status.value(u"rollbackQueued"_s).toBool();
    for (const QString &slot : {u"staged"_s, u"booted"_s, u"rollback"_s}) {
        const QJsonValue entry = status.value(slot);
        if (!entry.isObject())
            continue;

        // Host -> status -> BootEntry -> ImageStatus -> ImageReference.
        // spec.image and cachedUpdate describe desired/future images, not the
        // image actually deployed in this slot, so neither is a fallback.
        const QJsonObject imageStatus = entry.toObject().value(u"image"_s).toObject();
        const QJsonObject image = imageStatus.value(u"image"_s).toObject();
        const QString reference = image.value(u"image"_s).toString();
        const QString transport = image.value(u"transport"_s).toString();
        Deployment deployment;
        if (transport.isEmpty() || transport == u"registry"_s || transport == u"docker"_s)
            deployment = fromReference(reference);
        else {
            deployment.sourceReference = reference;
            deployment.reference = reference;
        }
        deployment.booted = slot == u"booted"_s;
        deployment.staged = slot == u"staged"_s;
        // rollbackQueued overrides a staged image. A download-only staged
        // entry is not finalized on shutdown, so it is not a next-boot target.
        deployment.pending = rollbackQueued ? slot == u"rollback"_s : deployment.staged && !entry.toObject().value(u"downloadOnly"_s).toBool();
        const QString digest = imageStatus.value(u"imageDigest"_s).toString();
        if (!digest.isEmpty())
            deployment.digest = digest;
        deployment.version = imageStatus.value(u"version"_s).toString();
        deployment.timestamp = timestampSeconds(imageStatus.value(u"timestamp"_s));
        result.append(deployment);
    }
    return result;
}
}
