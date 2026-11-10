// SPDX-License-Identifier: GPL-2.0-or-later

// Standalone QtCore regression test; no GUI, system commands, files, or moc.
#include "../src/components/data/image_identity.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

using namespace Qt::StringLiterals;

namespace
{
int checks = 0;
int failures = 0;

void check(bool passed, const char *expression, int line)
{
    ++checks;
    if (!passed) {
        ++failures;
        qCritical().noquote() << "Line" << line << "failed:" << expression;
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)

QJsonObject json(const char *source)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(source, &error);
    CHECK(error.error == QJsonParseError::NoError);
    CHECK(document.isObject());
    return document.object();
}

void referenceForms()
{
    struct Case {
        QString input;
        QString reference;
        QString repository;
        QString name;
        QString tag;
        QString digest;
    };
    const QString digest = u"sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"_s;
    const QList<Case> cases{
        {u"ostree-unverified-registry:ghcr.io/jheinem1/james-os:latest"_s, u"ghcr.io/jheinem1/james-os:latest"_s, u"ghcr.io/jheinem1/james-os"_s,
         u"james-os"_s, u"latest"_s, {}},
        {u"ostree-image-signed:docker://ghcr.io/ublue-os/bazzite:stable"_s, u"ghcr.io/ublue-os/bazzite:stable"_s, u"ghcr.io/ublue-os/bazzite"_s,
         u"bazzite"_s, u"stable"_s, {}},
        {u"ostree-image-signed:registry:registry.example:5000/team/nested/james-os:custom"_s, u"registry.example:5000/team/nested/james-os:custom"_s,
         u"registry.example:5000/team/nested/james-os"_s, u"james-os"_s, u"custom"_s, {}},
        {u"ostree-unverified-image:docker://ghcr.io/team/custom-os:testing"_s, u"ghcr.io/team/custom-os:testing"_s, u"ghcr.io/team/custom-os"_s,
         u"custom-os"_s, u"testing"_s, {}},
        {u"ostree-unverified-image:registry:ghcr.io/team/custom-os"_s, u"ghcr.io/team/custom-os"_s, u"ghcr.io/team/custom-os"_s, u"custom-os"_s, {}, {}},
        {u"ostree-remote-registry:fedora:quay.io/fedora/fedora-coreos:stable"_s, u"quay.io/fedora/fedora-coreos:stable"_s,
         u"quay.io/fedora/fedora-coreos"_s, u"fedora-coreos"_s, u"stable"_s, {}},
        {u"ostree-remote-image:fedora:docker://quay.io/fedora/fedora-coreos:next"_s, u"quay.io/fedora/fedora-coreos:next"_s,
         u"quay.io/fedora/fedora-coreos"_s, u"fedora-coreos"_s, u"next"_s, {}},
        {u"ostree-remote-image:custom:registry:registry.example:5000/team/os"_s, u"registry.example:5000/team/os"_s,
         u"registry.example:5000/team/os"_s, u"os"_s, {}, {}},
        {u"docker://localhost:5000/team/os:42"_s, u"localhost:5000/team/os:42"_s, u"localhost:5000/team/os"_s, u"os"_s, u"42"_s, {}},
        {u"registry:registry.example:5000/team/os"_s, u"registry.example:5000/team/os"_s, u"registry.example:5000/team/os"_s, u"os"_s, {}, {}},
        {u"docker://[::1]:5000/team/os:dev"_s, u"[::1]:5000/team/os:dev"_s, u"[::1]:5000/team/os"_s, u"os"_s, u"dev"_s, {}},
        {u"ghcr.io/jheinem1/james-os@"_s + digest, u"ghcr.io/jheinem1/james-os@"_s + digest, u"ghcr.io/jheinem1/james-os"_s, u"james-os"_s, {},
         digest},
        {u"ostree-image-signed:docker://ghcr.io/team/os:release@"_s + digest, u"ghcr.io/team/os:release@"_s + digest, u"ghcr.io/team/os"_s,
         u"os"_s, u"release"_s, digest},
        {u"ghcr.io/team/nested/os"_s, u"ghcr.io/team/nested/os"_s, u"ghcr.io/team/nested/os"_s, u"os"_s, {}, {}},
        {u"james-os"_s, u"james-os"_s, u"james-os"_s, u"james-os"_s, {}, {}},
        {u"  ghcr.io/team/os:Latest.42  "_s, u"ghcr.io/team/os:Latest.42"_s, u"ghcr.io/team/os"_s, u"os"_s, u"Latest.42"_s, {}},
    };
    for (const Case &test : cases) {
        const auto deployment = ImageIdentity::fromReference(test.input);
        CHECK(deployment.sourceReference == test.input);
        CHECK(deployment.reference == test.reference);
        CHECK(deployment.repository == test.repository);
        CHECK(deployment.name == test.name);
        CHECK(deployment.tag == test.tag);
        CHECK(deployment.digest == test.digest);
        CHECK(deployment.version.isEmpty());
        CHECK(!deployment.booted && !deployment.staged && !deployment.pending);
    }

    for (const QString &raw : {u""_s, u"ostree-unverified-registry:"_s, u"ostree-image-signed:ghcr.io/team/os:tag"_s,
                               u"ostree-remote-registry:missing-image"_s, u"ostree-remote-image::docker://ghcr.io/team/os"_s,
                               u"ostree-unknown:docker://ghcr.io/team/os"_s, u"ghcr.io/team/os:"_s, u"ghcr.io/team/os@"_s,
                               u"ghcr.io/team/os@sha256:abc@sha256:def"_s, u"ghcr.io/team/os:not a tag"_s, u"ghcr.io//os:tag"_s,
                               u"ghcr.io/team/:tag"_s, u"ghcr.io:invalid/team/os:tag"_s, u"https://ghcr.io/team/os:tag"_s,
                               u"fedora:fedora/42/x86_64/silverblue"_s, u"oci:/var/lib/local-os"_s, u"oci:local-os"_s,
                               u"ostree-unverified-image:oci:/var/lib/local-os"_s, u"containers-storage:localhost/os"_s}) {
        const auto deployment = ImageIdentity::fromReference(raw);
        CHECK(deployment.sourceReference == raw);
        CHECK(deployment.reference == raw);
        CHECK(deployment.name.isEmpty());
        CHECK(deployment.repository.isEmpty());
        CHECK(deployment.tag.isEmpty());
        CHECK(deployment.digest.isEmpty());
    }
}

void rpmOstreeRolesAndIdentity()
{
    const auto deployments = ImageIdentity::fromRpmOstree(json(R"({
        "deployments": [
            {"staged": true, "booted": false,
             "container-image-reference": "ostree-image-signed:docker://ghcr.io/ublue-os/bazzite:stable",
             "container-image-reference-digest": "sha256:staged", "version": "43.20261110", "timestamp": 1794268800},
            {"booted": true, "staged": false,
             "container-image-reference": "ostree-unverified-registry:ghcr.io/jheinem1/james-os:latest",
             "container-image-reference-digest": "sha256:booted", "version": "latest", "timestamp": 1794182400,
             "requested-packages": ["vim", 42, null, "", {"name":"ignored"}],
             "requested-local-packages": ["custom-package-1.0-1.x86_64", false]},
            {"booted": false, "staged": false,
             "container-image-reference": "ostree-unverified-registry:ghcr.io/jheinem1/james-os:latest",
             "container-image-reference-digest": "sha256:rollback", "version": "latest", "timestamp": 1794096000}
        ],
        "image-info": {"image-name":"bazzite", "image-tag":"stable"}
    })"));
    CHECK(deployments.size() == 3);
    if (deployments.size() != 3)
        return;
    CHECK(deployments[0].staged && !deployments[0].booted);
    CHECK(deployments[0].pending && !deployments[1].pending && !deployments[2].pending);
    CHECK(deployments[0].name == u"bazzite"_s);
    CHECK(deployments[1].booted && !deployments[1].staged);
    CHECK(deployments[1].reference == u"ghcr.io/jheinem1/james-os:latest"_s);
    CHECK(deployments[1].sourceReference == u"ostree-unverified-registry:ghcr.io/jheinem1/james-os:latest"_s);
    CHECK(deployments[1].repository == u"ghcr.io/jheinem1/james-os"_s);
    CHECK(deployments[1].name == u"james-os"_s);
    CHECK(deployments[1].tag == u"latest"_s);
    CHECK(deployments[1].digest == u"sha256:booted"_s);
    CHECK(deployments[1].version == u"latest"_s);
    CHECK(deployments[1].timestamp == 1794182400);
    CHECK((deployments[1].layeredPackages == QStringList{u"vim"_s, u"custom-package-1.0-1.x86_64"_s}));
    CHECK(!deployments[2].booted && !deployments[2].staged);
    CHECK(deployments[2].digest == u"sha256:rollback"_s);

    const auto noStaged = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {"booted":true, "container-image-reference":"ghcr.io/team/os@sha256:reference", "container-image-reference-digest":"sha256:explicit"},
        {"container-image-reference":"ghcr.io/team/os@sha256:reference"}
    ]})"));
    CHECK(noStaged.size() == 2);
    CHECK(noStaged.first().booted && !noStaged.first().staged);
    CHECK(!noStaged.first().pending && !noStaged.last().pending);
    CHECK(noStaged.first().digest == u"sha256:explicit"_s);
    CHECK(noStaged.last().digest == u"sha256:reference"_s);
    CHECK(noStaged.first().tag.isEmpty() && noStaged.first().version.isEmpty());
}

void rpmOstreeUnknownAndMalformed()
{
    CHECK(ImageIdentity::fromRpmOstree({}).isEmpty());
    CHECK(ImageIdentity::fromRpmOstree(json(R"({"deployments":null})")).isEmpty());
    CHECK(ImageIdentity::fromRpmOstree(json(R"({"deployments":{}})")).isEmpty());
    CHECK(ImageIdentity::fromRpmOstree(json(R"({"deployments":[null, false, "wrong", 3]})")).isEmpty());

    const auto deployments = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {"staged":true, "container-image-reference":"ghcr.io/team/staged:latest"},
        {"booted":true},
        {"origin":"fedora:fedora/42/x86_64/silverblue", "version":"42.1", "timestamp":2208988800},
        {"origin":"fedora/42/x86_64/silverblue"},
        {"origin":"ostree-unverified-registry:ghcr.io/team/old-format:testing"},
        {"checksum":"known-deployment", "booted":"true", "staged":1, "container-image-reference":{}, "container-image-reference-digest":7,
         "version":true, "timestamp":"not-a-timestamp", "requested-packages":"wrong", "requested-local-packages":{}},
        {},
        {"id":"negative-timestamp", "timestamp":-1}, {"id":"fractional-timestamp", "timestamp":1.5}, {"id":"overflow-timestamp", "timestamp":1e100}
    ]})"));
    CHECK(deployments.size() == 9);
    if (deployments.size() != 9)
        return;
    CHECK(!deployments[0].booted);
    CHECK(deployments[1].booted);
    CHECK(deployments[1].name.isEmpty() && deployments[1].tag.isEmpty() && deployments[1].reference.isEmpty());
    CHECK(deployments[2].reference == u"fedora:fedora/42/x86_64/silverblue"_s);
    CHECK(deployments[2].sourceReference == u"fedora:fedora/42/x86_64/silverblue"_s);
    CHECK(deployments[2].repository.isEmpty() && deployments[2].name.isEmpty() && deployments[2].tag.isEmpty());
    CHECK(deployments[2].version == u"42.1"_s && deployments[2].timestamp == 2208988800LL);
    CHECK(deployments[3].reference == u"fedora/42/x86_64/silverblue"_s);
    CHECK(deployments[3].name.isEmpty() && deployments[3].repository.isEmpty());
    CHECK(deployments[4].reference == u"ghcr.io/team/old-format:testing"_s && deployments[4].name == u"old-format"_s);
    CHECK(!deployments[5].booted && !deployments[5].staged);
    CHECK(deployments[5].reference.isEmpty() && deployments[5].digest.isEmpty() && deployments[5].version.isEmpty());
    CHECK(deployments[5].timestamp == 0 && deployments[5].layeredPackages.isEmpty());
    CHECK(deployments[6].timestamp == 0 && deployments[7].timestamp == 0 && deployments[8].timestamp == 0);

    const auto unmarked = ImageIdentity::fromRpmOstree(json(R"({"deployments":[{"container-image-reference":"ghcr.io/team/os:tag"}]})"));
    CHECK(!unmarked.first().booted && !unmarked.first().staged);
    CHECK(!unmarked.first().pending);
}

void rpmOstreeSkipsNonDeployments()
{
    const auto deployments = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {}, {"unexpected":"value"}, {"booted":"true", "staged":1},
        {"container-image-reference":[], "origin":false, "checksum":null, "id":17},
        {"container-image-reference":"  ", "origin":"", "checksum":"  ", "id":""},
        {"version":"metadata without a deployment", "timestamp":1794182400},
        {"booted":true}, {"staged":true}, {"booted":false}, {"staged":false},
        {"checksum":"known-commit"}, {"id":"known-deployment"},
        {"container-image-reference-digest":"sha256:known-image"}
    ]})"));
    CHECK(deployments.size() == 7);
    if (deployments.size() != 7)
        return;
    CHECK(deployments.first().booted && !deployments.first().staged);
    CHECK(deployments.first().reference.isEmpty() && deployments.first().name.isEmpty());
    CHECK(deployments[1].staged && !deployments[1].booted);
    CHECK(!deployments[2].booted && !deployments[2].staged);
    CHECK(!deployments[3].booted && !deployments[3].staged);
    CHECK(deployments[4].reference.isEmpty() && deployments[4].name.isEmpty());
    CHECK(deployments[5].reference.isEmpty() && deployments[5].name.isEmpty());
    CHECK(deployments[6].reference.isEmpty() && deployments[6].digest == u"sha256:known-image"_s);

    const auto onlyBooted = ImageIdentity::fromRpmOstree(json(R"({"deployments":[{}, {"booted":true}, {}]})"));
    CHECK(onlyBooted.size() == 1 && onlyBooted.first().booted && !onlyBooted.first().staged && !onlyBooted.first().pending);
}

void rpmOstreePendingBootOrder()
{
    const auto finalized = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {}, {"unexpected":"not a deployment"},
        {"booted":false, "staged":false, "container-image-reference":"ghcr.io/team/next-os:latest"},
        {"booted":true, "container-image-reference":"ghcr.io/team/current-os:latest"},
        {"booted":false, "container-image-reference":"ghcr.io/team/previous-os:latest"}
    ]})"));
    CHECK(finalized.size() == 3);
    if (finalized.size() != 3)
        return;
    CHECK(finalized[0].pending && !finalized[0].staged && !finalized[0].booted);
    CHECK(finalized[0].name == u"next-os"_s);
    CHECK(finalized[1].booted && !finalized[1].pending);
    CHECK(!finalized[2].pending);

    const auto noBooted = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {"container-image-reference":"ghcr.io/team/unknown-first:latest"},
        {"booted":false, "container-image-reference":"ghcr.io/team/unknown-second:latest"}
    ]})"));
    CHECK(noBooted.size() == 2 && !noBooted.first().pending && !noBooted.last().pending);

    const auto stagedWithoutBooted = ImageIdentity::fromRpmOstree(json(R"({"deployments":[
        {"id":"unmarked-first"}, {"staged":true}, {"id":"unmarked-last"}
    ]})"));
    CHECK(stagedWithoutBooted.size() == 3);
    CHECK(!stagedWithoutBooted[0].pending && stagedWithoutBooted[1].pending && !stagedWithoutBooted[2].pending);
    CHECK(stagedWithoutBooted[1].staged && !stagedWithoutBooted[1].booted && stagedWithoutBooted[1].name.isEmpty());
}

void bootcSlots()
{
    // Schema/fixture: bootc-dev/bootc docs/src/host-v1.schema.json and
    // crates/lib/src/fixtures/spec-staged-booted.yaml (v1alpha1 also has this shape).
    const auto deployments = ImageIdentity::fromBootc(json(R"({
        "spec":{"image":{"image":"ghcr.io/ublue-os/bazzite:stable", "transport":"registry"}},
        "status": {
            "staged": {"image": {
                "image":{"image":"registry.example:5000/team/new-os:beta", "transport":"registry"},
                "imageDigest":"sha256:staged", "version":"nightly", "timestamp":"2023-10-14T19:22:15.421234567Z"}},
            "booted": {"image": {
                "image":{"image":"ghcr.io/jheinem1/james-os:latest", "transport":"registry"},
                "imageDigest":"sha256:booted", "version":"latest", "timestamp":"2023-09-30T19:22:16Z"},
                "cachedUpdate":{"image":{"image":"ghcr.io/team/future-os:future"},"imageDigest":"sha256:cached"}},
            "rollback": {"image": {
                "image":{"image":"ghcr.io/jheinem1/james-os@sha256:pinned", "transport":"registry"},
                "version":null, "timestamp":"2023-09-30T21:22:16+02:00"}}
        }
    })"));
    CHECK(deployments.size() == 3);
    if (deployments.size() != 3)
        return;
    CHECK(deployments[0].staged && !deployments[0].booted);
    CHECK(deployments[0].pending && !deployments[1].pending && !deployments[2].pending);
    CHECK(deployments[0].name == u"new-os"_s && deployments[0].tag == u"beta"_s);
    CHECK(deployments[0].digest == u"sha256:staged"_s && deployments[0].version == u"nightly"_s);
    CHECK(deployments[0].timestamp == 1697311335);
    CHECK(deployments[1].booted && !deployments[1].staged);
    CHECK(deployments[1].name == u"james-os"_s && deployments[1].tag == u"latest"_s);
    CHECK(deployments[1].reference == u"ghcr.io/jheinem1/james-os:latest"_s);
    CHECK(deployments[1].digest == u"sha256:booted"_s && deployments[1].version == u"latest"_s);
    CHECK(deployments[1].sourceReference == u"ghcr.io/jheinem1/james-os:latest"_s);
    CHECK(deployments[1].timestamp == 1696101736);
    CHECK(!deployments[2].booted && !deployments[2].staged);
    CHECK(deployments[2].digest == u"sha256:pinned"_s && deployments[2].tag.isEmpty() && deployments[2].version.isEmpty());
    CHECK(deployments[2].timestamp == deployments[1].timestamp);

    const auto noStaged = ImageIdentity::fromBootc(json(R"({"status":{
        "staged":null, "booted":{"image":{"image":{"image":"ghcr.io/team/os", "transport":"registry"}}}, "rollback":null
    }})"));
    CHECK(noStaged.size() == 1 && noStaged.first().booted && !noStaged.first().staged);
    CHECK(!noStaged.first().pending);
    CHECK(noStaged.first().name == u"os"_s && noStaged.first().tag.isEmpty());
}

void bootcUnknownAndMalformed()
{
    CHECK(ImageIdentity::fromBootc({}).isEmpty());
    CHECK(ImageIdentity::fromBootc(json(R"({"status":null})")).isEmpty());
    CHECK(ImageIdentity::fromBootc(json(R"({"status":[]})")).isEmpty());
    CHECK(ImageIdentity::fromBootc(json(R"({"status":{"staged":null,"booted":false,"rollback":"wrong"}})")).isEmpty());

    const auto deployments = ImageIdentity::fromBootc(json(R"({
        "spec":{"image":{"image":"ghcr.io/ublue-os/bazzite:stable", "transport":"registry"}},
        "status":{
            "staged":{"booted":true, "image":{"image":{"image":"ghcr.io/team/staged:latest"}}},
            "booted":{"image":null, "cachedUpdate":{"image":{"image":"ghcr.io/team/future:latest"}}},
            "rollback":{"image":{"image":{"image":"/var/mnt/osupdate", "transport":"oci"},
                         "imageDigest":"sha256:local", "version":"local", "timestamp":null}}
        }
    })"));
    CHECK(deployments.size() == 3);
    if (deployments.size() != 3)
        return;
    CHECK(!deployments[0].booted && deployments[0].staged);
    CHECK(deployments[1].booted && deployments[1].name.isEmpty() && deployments[1].reference.isEmpty());
    CHECK(deployments[1].tag.isEmpty() && deployments[1].digest.isEmpty() && deployments[1].version.isEmpty());
    CHECK(deployments[2].reference == u"/var/mnt/osupdate"_s && deployments[2].repository.isEmpty() && deployments[2].name.isEmpty());
    CHECK(deployments[2].sourceReference == u"/var/mnt/osupdate"_s);
    CHECK(deployments[2].digest == u"sha256:local"_s && deployments[2].version == u"local"_s && deployments[2].timestamp == 0);

    for (const QJsonValue &timestamp : {QJsonValue(u"2023-09-30T19:22:16"_s), QJsonValue(u"2023-99-30T19:22:16Z"_s), QJsonValue(true),
                                        QJsonValue(QJsonObject{})}) {
        const QJsonObject source{{u"status"_s, QJsonObject{{u"booted"_s, QJsonObject{{u"image"_s, QJsonObject{{u"timestamp"_s, timestamp}}}}}}}};
        CHECK(ImageIdentity::fromBootc(source).first().timestamp == 0);
    }
    const auto noBooted = ImageIdentity::fromBootc(json(R"({"status":{"staged":{},"rollback":{}}})"));
    CHECK(noBooted.size() == 2 && !noBooted.first().booted && !noBooted.last().booted);
}

void bootcPendingRoles()
{
    struct Case {
        const char *source;
        int count;
        int pendingIndex;
    };
    const QList<Case> cases{
        {R"({"status":{"staged":{},"booted":{},"rollback":{},"rollbackQueued":true}})", 3, 2},
        {R"({"status":{"staged":null,"booted":{},"rollback":{},"rollbackQueued":true}})", 2, 1},
        {R"({"status":{"staged":{},"booted":{},"rollback":{},"rollbackQueued":false}})", 3, 0},
        {R"({"status":{"staged":{},"booted":{},"rollback":{}}})", 3, 0},
        {R"({"status":{"staged":null,"booted":{},"rollback":{},"rollbackQueued":false}})", 2, -1},
        {R"({"status":{"staged":{},"booted":{},"rollback":null,"rollbackQueued":true}})", 2, -1},
        {R"({"status":{"staged":{},"booted":{},"rollbackQueued":true}})", 2, -1},
        {R"({"status":{"staged":{"downloadOnly":true},"booted":{},"rollback":{}}})", 3, -1},
        {R"({"status":{"staged":{"downloadOnly":false},"booted":{},"rollback":{}}})", 3, 0},
        {R"({"status":{"staged":{"downloadOnly":true},"booted":{},"rollback":{},"rollbackQueued":true}})", 3, 2},
        {R"({"status":{"staged":{"downloadOnly":false},"booted":{},"rollback":{},"rollbackQueued":true}})", 3, 2},
    };
    for (const Case &test : cases) {
        const QJsonObject source = json(test.source);
        const auto deployments = ImageIdentity::fromBootc(source);
        CHECK(deployments.size() == test.count);
        for (qsizetype i = 0; i < deployments.size(); ++i) {
            CHECK(deployments[i].pending == (i == test.pendingIndex));
            if (deployments[i].booted)
                CHECK(!deployments[i].pending && !deployments[i].staged);
        }
        if (source.value(u"status"_s).toObject().value(u"staged"_s).isObject())
            CHECK(deployments.first().staged && !deployments.first().booted);
    }
}
}

int main()
{
    referenceForms();
    rpmOstreeRolesAndIdentity();
    rpmOstreeUnknownAndMalformed();
    rpmOstreeSkipsNonDeployments();
    rpmOstreePendingBootOrder();
    bootcSlots();
    bootcUnknownAndMalformed();
    bootcPendingRoles();
    qInfo() << "Image identity:" << checks << "checks," << failures << "failures";
    return failures == 0 ? 0 : 1;
}
