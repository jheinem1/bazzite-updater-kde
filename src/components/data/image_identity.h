// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtTypes>

namespace ImageIdentity
{
struct Deployment {
    bool booted = false;
    bool staged = false;
    bool pending = false; // A reported next-boot target that is not the booted deployment.
    QString sourceReference; // Exact reported reference; bootc keeps transport/signature in separate JSON fields.
    QString reference;
    QString name;
    QString repository;
    QString tag;
    QString digest;
    QString version;
    qint64 timestamp = 0;
    QStringList layeredPackages;
};

// Parse an OCI reference, optionally wrapped in an rpm-ostree registry transport.
// Unsupported or malformed references remain displayable, with no invented identity.
// No default registry, namespace, tag, or version is added.
Deployment fromReference(const QString &raw);

// Both functions take the complete JSON command result, perform no I/O, and
// preserve the reported deployment roles rather than assuming entry 0 is booted.
QList<Deployment> fromRpmOstree(const QJsonObject &root);
QList<Deployment> fromBootc(const QJsonObject &root);
}
