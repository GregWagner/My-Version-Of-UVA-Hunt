#include "Types.h"

#include <QDateTime>
#include <QLocale>

void ProblemCatalog::rebuild()
{
    byPid.clear();
    pidByNum.clear();
    byPid.reserve(problems.size());
    pidByNum.reserve(problems.size());
    for (const Problem &p : problems) {
        byPid.insert(p.pid, p);
        pidByNum.insert(p.num, p.pid);
    }
}

const Problem *ProblemCatalog::problem(int pid) const
{
    auto it = byPid.constFind(pid);
    return it == byPid.constEnd() ? nullptr : &it.value();
}

const Problem *ProblemCatalog::problemByNum(int num) const
{
    auto it = pidByNum.constFind(num);
    return it == pidByNum.constEnd() ? nullptr : problem(it.value());
}

QString verdictName(int ver)
{
    switch (ver) {
    case 10: return QStringLiteral("Submission error");
    case 15: return QStringLiteral("Can't be judged");
    case 20: return QStringLiteral("In queue");
    case 30: return QStringLiteral("Compile error");
    case 35: return QStringLiteral("Restricted function");
    case 40: return QStringLiteral("Runtime error");
    case 45: return QStringLiteral("Output limit exceeded");
    case 50: return QStringLiteral("Time limit exceeded");
    case 60: return QStringLiteral("Memory limit exceeded");
    case 70: return QStringLiteral("Wrong answer");
    case 80: return QStringLiteral("Presentation error");
    case 90: return QStringLiteral("Accepted");
    default: break;
    }
    if (ver == 0)
        return QStringLiteral("In queue");
    return QStringLiteral("Verdict %1").arg(ver);
}

QString verdictShort(int ver)
{
    switch (ver) {
    case 10: return QStringLiteral("SE");
    case 15: return QStringLiteral("CJ");
    case 20: return QStringLiteral("QU");
    case 30: return QStringLiteral("CE");
    case 35: return QStringLiteral("RF");
    case 40: return QStringLiteral("RE");
    case 45: return QStringLiteral("OL");
    case 50: return QStringLiteral("TL");
    case 60: return QStringLiteral("ML");
    case 70: return QStringLiteral("WA");
    case 80: return QStringLiteral("PE");
    case 90: return QStringLiteral("AC");
    default: break;
    }
    if (ver == 0)
        return QStringLiteral("QU");
    return QStringLiteral("V%1").arg(ver);
}

QString verdictForeground(int ver)
{
    if (ver == 90)
        return QStringLiteral("#1b7f3b");   // green, accepted
    if (ver == 20 || ver == 0)
        return QStringLiteral("#8a6d00");   // amber, pending
    return QStringLiteral("#b00020");       // red, failure
}

QString languageName(int lan)
{
    switch (lan) {
    case 1: return QStringLiteral("ANSI C");
    case 2: return QStringLiteral("Java");
    case 3: return QStringLiteral("C++");
    case 4: return QStringLiteral("Pascal");
    case 5: return QStringLiteral("C++11");
    case 6: return QStringLiteral("C++11");
    default: break;
    }
    return QStringLiteral("Language %1").arg(lan);
}

QString formatRuntime(int run, int ver)
{
    if (ver != 90)
        return QStringLiteral("-");
    if (run <= 0)
        return QStringLiteral("0.000");
    return QString::number(run / 1000.0, 'f', 3);
}

QString formatUnixTime(qint64 ts)
{
    if (ts <= 0)
        return QStringLiteral("-");
    return QDateTime::fromSecsSinceEpoch(ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}
