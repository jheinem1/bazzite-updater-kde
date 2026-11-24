// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <optional>

namespace UpdateOutput
{
struct Progress {
    int current;
    int total;
};

struct Line {
    QString consoleText;
    std::optional<Progress> progress;
};

// Parse the output field of a UUPD record, not the whole record.
Line parse(const QString &module, const QString &output, const QString &message);
}
