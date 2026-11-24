// SPDX-License-Identifier: GPL-2.0-or-later

#include "update_output.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <iterator>

using namespace Qt::StringLiterals;

int main()
{
    struct Case {
        const char *name;
        const char *module;
        const char *output;
        const char *text;
        int current = -1;
        int total = -1;
    };
    constexpr auto invalidJson = "{not valid JSON";
    const Case cases[] = {
        {"bootc byte counters above INT_MAX", "system",
         R"({"type":"ProgressBytes","description":"Downloading image [1/9]","bytes":3221225472,"bytesTotal":6442450944})",
         "Downloading image [1/9]", 50, 100},
        {"zero downloaded is valid", "system", R"({"type":"ProgressBytes","description":"Waiting","bytes":0,"bytesTotal":123})", "Waiting", 0, 100},
        {"complete download", "system", R"({"type":"ProgressBytes","description":"Complete","bytes":123,"bytesTotal":123})", "Complete", 100, 100},
        {"overshoot capped before converting", "system",
         R"({"type":"ProgressBytes","description":"Complete","bytes":18446744073709551615,"bytesTotal":1})", "Complete", 100, 100},
        {"very large counters stay bounded", "system",
         R"({"type":"ProgressBytes","description":"Downloading","bytes":1e300,"bytesTotal":0.001})", "Downloading", 100, 100},
        {"unknown total stays indeterminate", "system",
         R"({"type":"ProgressBytes","description":"Preparing","bytes":9,"bytesTotal":0})", "Preparing"},
        {"missing bytes stays indeterminate", "system", R"({"type":"ProgressBytes","description":"Waiting","bytesTotal":10})", "Waiting"},
        {"missing total stays indeterminate", "system", R"({"type":"ProgressBytes","description":"Waiting","bytes":5})", "Waiting"},
        {"negative byte count rejected", "system", R"({"type":"ProgressBytes","description":"Waiting","bytes":-2,"bytesTotal":3})", "Waiting"},
        {"negative total rejected", "system", R"({"type":"ProgressBytes","description":"Waiting","bytes":2,"bytesTotal":-3})", "Waiting"},
        {"quoted count not accepted", "system", R"({"type":"ProgressBytes","description":"Waiting","bytes":"2","bytesTotal":3})", "Waiting"},
        {"fractional percent floors without overflowing", "system",
         R"({"type":"ProgressBytes","description":"Waiting","bytes":2,"bytesTotal":3})", "Waiting", 66, 100},
        {"spaced JSON recognized with text retained", "system",
         R"(  {"type":"ProgressBytes","bytes":2,"bytesTotal":3}  )", R"(  {"type":"ProgressBytes","bytes":2,"bytesTotal":3}  )", 66, 100},
        {"non-byte event keeps description without fake progress", "system",
         R"({"type":"ProgressSteps","description":"Staging [1/3]"})", "Staging [1/3]"},
        {"foreign JSON does not get interpreted as bootc", "flatpak",
         R"({"type":"ProgressBytes","description":"Keep original JSON","bytes":2,"bytesTotal":3})",
         R"({"type":"ProgressBytes","description":"Keep original JSON","bytes":2,"bytesTotal":3})"},
        {"plain system progress", "system", "Receiving objects [3/8]", "Receiving objects [3/8]", 3, 8},
        {"plain Flatpak progress", "flatpak", "Installing [2/6]", "Installing [2/6]", 2, 6},
        {"legacy zero total", "flatpak", "Installing [1/0]", "Installing [1/0]"},
        {"legacy integer overflow", "flatpak", "Installing [3000000000/3000000001]", "Installing [3000000000/3000000001]"},
        {"malformed JSON remains visible", "system", invalidJson, invalidJson},
        {"message fallback", "system", "", "System started"},
    };

    int failures = 0;
    for (const auto &test : cases) {
        const auto line = UpdateOutput::parse(QString::fromUtf8(test.module), QString::fromUtf8(test.output), u"System started"_s);
        const bool progress = test.current == -1 ? !line.progress
                                                 : line.progress && line.progress->current == test.current && line.progress->total == test.total;
        if (!progress || line.consoleText != QString::fromUtf8(test.text)) {
            qCritical() << "Failed:" << test.name << line.consoleText << (line.progress ? line.progress->current : -1)
                        << (line.progress ? line.progress->total : -1);
            ++failures;
        }
    }

    // Exercise the wrapped representation seen by SystemUpdateBackend, rather
    // than only supplying the nested object directly.
    const auto record = QJsonDocument::fromJson(
                            R"({"level":"INFO","module":"system","msg":"Command output","output":"{\"type\":\"ProgressBytes\",\"description\":\"Writing image\",\"bytes\":9000000000,\"bytesTotal\":12000000000}\n"})")
                            .object();
    const auto wrapped = UpdateOutput::parse(record.value(u"module"_s).toString(), record.value(u"output"_s).toString(), record.value(u"msg"_s).toString());
    if (!wrapped.progress || wrapped.progress->current != 75 || wrapped.progress->total != 100 || wrapped.consoleText != u"Writing image"_s) {
        qCritical() << "Failed: wrapped UUPD JSON record";
        ++failures;
    }

    qInfo() << std::size(cases) + 1 << "updater output cases;" << failures << "failed";
    return failures == 0 ? 0 : 1;
}
