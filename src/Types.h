#pragma once

#include <QHash>
#include <QJsonArray>
#include <QMetaType>
#include <QSet>
#include <QString>
#include <QVector>

// A single UVa problem as returned by /api/p. Field order and meaning come from
// the uhunt API documentation (each problem is a 21 element JSON array).
struct Problem {
    int pid = 0;      // internal problem id (used by submissions)
    int num = 0;      // problem number shown on onlinejudge.org
    QString title;
    int dacu = 0;     // distinct accepted users
    int mrun = 0;     // best accepted runtime (ms)
    int mmem = 0;     // best accepted memory
    int ce = 0;
    int re = 0;
    int ole = 0;
    int tle = 0;
    int mle = 0;
    int wa = 0;
    int pe = 0;
    int ac = 0;       // number of accepted submissions
    int rtl = 0;      // problem run time limit (ms)
    int status = 0;   // 0 = unavailable, 1 = normal, 2 = special judge
    int sube = 0;     // submission errors
};

// One submission from /api/subs-user. Hex tuple documented in the API:
// [sid, pid, verdict, runtime, submit-time, language, rank]
struct Submission {
    int sid = 0;
    int pid = 0;
    int ver = 0;
    int run = 0;
    qint64 sbt = 0;
    int lan = 0;
    int rank = 0;
};

struct UserRank {
    int rank = 0;
    int old = 0;
    int userid = 0;
    QString name;
    QString username;
    int ac = 0;
    int nos = 0;
    QVector<int> activity;  // accepted in 2d, 7d, 31d, 3mo, 1yr
};

// A live event from /api/poll (long polling feed).
struct LiveEvent {
    qint64 id = 0;
    QString type;
    int sid = 0;
    int uid = 0;
    int pid = 0;
    int ver = 0;
    int lan = 0;
    int run = 0;
    int mem = 0;
    int rank = 0;
    qint64 sbt = 0;
    QString name;
    QString uname;
};

// A flattened Competitive Programming Book exercise category. `nums` holds
// absolute problem numbers; `starred` is the subset that are "starred"
// (harder, must-try) problems in the book.
struct Category {
    QString path;
    QVector<int> nums;
    QSet<int> starred;
};

// Shared problem lookup tables. Rebuilt whenever /api/p is refreshed.
struct ProblemCatalog {
    QVector<Problem> problems;
    QHash<int, Problem> byPid;
    QHash<int, int> pidByNum;  // problem number -> pid

    void rebuild();
    const Problem *problem(int pid) const;
    const Problem *problemByNum(int num) const;
};

Q_DECLARE_METATYPE(Problem)
Q_DECLARE_METATYPE(Submission)
Q_DECLARE_METATYPE(UserRank)
Q_DECLARE_METATYPE(Category)
Q_DECLARE_METATYPE(LiveEvent)

// Verdict / language formatting helpers.
QString verdictName(int ver);
QString verdictShort(int ver);
QString verdictForeground(int ver);
QString languageName(int lan);
QString formatRuntime(int run, int ver);
QString formatUnixTime(qint64 ts);
