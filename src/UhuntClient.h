#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>

#include "Types.h"

class QNetworkReply;

// Thin wrapper around the public uhunt JSON API
// (https://uhunt.onlinejudge.org/api). Every call is asynchronous; results are
// delivered through signals on the application's event loop.
class UhuntClient : public QObject
{
    Q_OBJECT

public:
    explicit UhuntClient(QObject *parent = nullptr);

    void fetchUserId(const QString &uname);
    void fetchProblems();
    void fetchUserSubmissions(int uid, int minSid = 0);
    void fetchUserSubmissionsLast(int uid, int count);
    void fetchRanklist(int uid, int nabove = 5, int nbelow = 5);
    // Up to `nabove` ranks above the user, plus the user's own row. The
    // /ranklist endpoint silently caps nabove at 100, so this looks up the
    // user's current rank and pulls the window from the global ranklist.
    void fetchRanklistWindow(int uid, int nabove);
    void fetchGlobalRank(int pos, int count);
    void fetchCpBook(int edition);

    // Load CP-book exercises bundled with the app (Qt resource). Used for the
    // 4th edition, which the uhunt API does not serve (only 1-3 are online).
    void loadCpBook(const QString &resourcePath);

    // Live submission feed. Keeps one long-poll request open at a time and
    // re-arms itself until stopLivePolling() is called.
    void startLivePolling(qint64 lastId = 0);
    void stopLivePolling();
    bool isLivePolling() const { return m_liveActive; }

signals:
    void userIdResolved(const QString &uname, int uid);
    void problemsFetched(const QVector<Problem> &problems);
    void userSubmissionsFetched(int uid, const QVector<Submission> &subs);
    void ranklistFetched(const QVector<UserRank> &list);
    void cpBookFetched(const QVector<Category> &categories);
    void liveEventsReceived(const QVector<LiveEvent> &events, qint64 lastId);
    void livePollingStopped();
    void errorOccurred(const QString &context, const QString &message);

private:
    QNetworkReply *get(const QString &path, int timeoutMs = 15000);
    static QVector<Problem> parseProblems(const QJsonArray &arr);
    static QVector<Submission> parseSubmissions(const QJsonArray &arr);
    static QVector<UserRank> parseRanklist(const QJsonArray &arr);
    static QVector<Category> parseCpBook(const QJsonArray &arr);
    static QVector<LiveEvent> parseLiveEvents(const QJsonArray &arr, qint64 &lastId);
    void pollOnce(qint64 pollId);

    QString m_baseUrl;
    QNetworkAccessManager m_nam;
    QNetworkReply *m_liveReply = nullptr;
    bool m_liveActive = false;
    qint64 m_lastPollId = 0;
};
