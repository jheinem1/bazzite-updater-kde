// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "update_output.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

UpdateOutput::Line UpdateOutput::parse(const QString &module, const QString &output, const QString &message)
{
    Line line{output.isEmpty() ? message : output, std::nullopt};
    // UUPD wraps bootc's progress JSON in the system module's output field.
    if (module == u"system"_s && output.trimmed().startsWith(u'{')) {
        const auto json = QJsonDocument::fromJson(output.toUtf8()).object();
        const auto type = json.value(u"type"_s).toString();
        if (!type.isEmpty()) {
            const auto description = json.value(u"description"_s).toString();
            if (!description.trimmed().isEmpty())
                line.consoleText = description;

            if (type == u"ProgressBytes"_s) {
                const double bytes = json.value(u"bytes"_s).toDouble(-1);
                const double total = json.value(u"bytesTotal"_s).toDouble(-1);
                if (std::isfinite(bytes) && std::isfinite(total) && bytes >= 0 && total > 0) {
                    // Cap before dividing or narrowing. Counts can exceed the size of an int.
                    const int percent = static_cast<int>(100 * (std::min(bytes, total) / total));
                    line.progress = Progress{percent, 100};
                }
            }
            // A bracketed number in a bootc description is text, not legacy progress.
            return line;
        }
    }

    static const QRegularExpression legacyProgress(QStringLiteral(R"(\[(\d+)/(\d+)\])"));
    const auto match = legacyProgress.match(output);
    if (match.hasMatch()) {
        bool currentOk = false;
        bool totalOk = false;
        const int current = match.captured(1).toInt(&currentOk);
        const int total = match.captured(2).toInt(&totalOk);
        if (currentOk && totalOk && total > 0)
            line.progress = Progress{std::min(current, total), total};
    }
    return line;
}
