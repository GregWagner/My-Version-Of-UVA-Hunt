#include "UhuntClient.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

constexpr const char *kBaseUrl = "https://uhunt.onlinejudge.org/api";
constexpr int kLivePollTimeoutMs = 70000;  // server holds the request up to 60s

QJsonArray parseArray(const QByteArray &data, QString *parseError = nullptr)
{
    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        if (parseError)
            *parseError = err.errorString();
        return {};
    }
    if (doc.isArray())
        return doc.array();
    if (doc.isObject())
        return QJsonArray { doc.object() };
    return {};
}

}  // namespace

UhuntClient::UhuntClient(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QString::fromLatin1(kBaseUrl))
{
}

QNetworkReply *UhuntClient::get(const QString &path, int timeoutMs)
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("hunt/0.1"));
    req.setRawHeader("Accept", "application/json");
    if (timeoutMs > 0)
        req.setTransferTimeout(timeoutMs);
    return m_nam.get(req);
}

QVector<Problem> UhuntClient::parseProblems(const QJsonArray &arr)
{
    QVector<Problem> out;
    out.reserve(arr.size());
    for (const QJsonValue &value : arr) {
        const QJsonArray a = value.toArray();
        if (a.size() < 21)
            continue;
        Problem p;
        p.pid = a.at(0).toInt();
        p.num = a.at(1).toInt();
        p.title = a.at(2).toString();
        p.dacu = a.at(3).toInt();
        p.mrun = a.at(4).toInt();
        p.mmem = a.at(5).toInt();
        p.sube = a.at(7).toInt();
        p.ce = a.at(10).toInt();
        p.re = a.at(12).toInt();
        p.ole = a.at(13).toInt();
        p.tle = a.at(14).toInt();
        p.mle = a.at(15).toInt();
        p.wa = a.at(16).toInt();
        p.pe = a.at(17).toInt();
        p.ac = a.at(18).toInt();
        p.rtl = a.at(19).toInt();
        p.status = a.at(20).toInt();
        out.push_back(p);
    }
    return out;
}

QVector<Submission> UhuntClient::parseSubmissions(const QJsonArray &arr)
{
    QVector<Submission> out;
    out.reserve(arr.size());
    for (const QJsonValue &value : arr) {
        const QJsonArray a = value.toArray();
        if (a.size() < 7)
            continue;
        Submission s;
        s.sid = a.at(0).toInt();
        s.pid = a.at(1).toInt();
        s.ver = a.at(2).toInt();
        s.run = a.at(3).toInt();
        s.sbt = static_cast<qint64>(a.at(4).toDouble());
        s.lan = a.at(5).toInt();
        s.rank = a.at(6).toInt();
        out.push_back(s);
    }
    return out;
}

QVector<UserRank> UhuntClient::parseRanklist(const QJsonArray &arr)
{
    QVector<UserRank> out;
    out.reserve(arr.size());
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        UserRank r;
        r.rank = o.value(QStringLiteral("rank")).toInt();
        r.old = o.value(QStringLiteral("old")).toInt();
        r.userid = o.value(QStringLiteral("userid")).toInt();
        r.name = o.value(QStringLiteral("name")).toString();
        r.username = o.value(QStringLiteral("username")).toString();
        r.ac = o.value(QStringLiteral("ac")).toInt();
        r.nos = o.value(QStringLiteral("nos")).toInt();
        for (const QJsonValue &v : o.value(QStringLiteral("activity")).toArray())
            r.activity.push_back(v.toInt());
        out.push_back(r);
    }
    return out;
}

QVector<Category> UhuntClient::parseCpBook(const QJsonArray &arr)
{
    QVector<Category> out;
    for (const QJsonValue &chapterValue : arr) {
        const QJsonObject chapter = chapterValue.toObject();
        const QString chapterTitle = chapter.value(QStringLiteral("title")).toString();
        for (const QJsonValue &sectionValue : chapter.value(QStringLiteral("arr")).toArray()) {
            const QJsonObject section = sectionValue.toObject();
            const QString sectionTitle = section.value(QStringLiteral("title")).toString();
            for (const QJsonValue &groupValue : section.value(QStringLiteral("arr")).toArray()) {
                const QJsonArray group = groupValue.toArray();
                if (group.isEmpty())
                    continue;
                Category c;
                c.path = QStringLiteral("%1 / %2 / %3")
                             .arg(chapterTitle, sectionTitle, group.first().toString());
                for (int i = 1; i < group.size(); ++i) {
                    const int raw = group.at(i).toInt();
                    const int num = qAbs(raw);
                    c.nums.push_back(num);
                    if (raw < 0)
                        c.starred.insert(num);  // negative == starred (harder)
                }
                out.push_back(c);
            }
        }
    }
    return out;
}

QVector<LiveEvent> UhuntClient::parseLiveEvents(const QJsonArray &arr, qint64 &lastId)
{
    QVector<LiveEvent> out;
    out.reserve(arr.size());
    for (const QJsonValue &value : arr) {
        const QJsonObject o = value.toObject();
        LiveEvent e;
        e.id = static_cast<qint64>(o.value(QStringLiteral("id")).toDouble());
        e.type = o.value(QStringLiteral("type")).toString();
        const QJsonObject msg = o.value(QStringLiteral("msg")).toObject();
        e.sid = msg.value(QStringLiteral("sid")).toInt();
        e.uid = msg.value(QStringLiteral("uid")).toInt();
        e.pid = msg.value(QStringLiteral("pid")).toInt();
        e.ver = msg.value(QStringLiteral("ver")).toInt();
        e.lan = msg.value(QStringLiteral("lan")).toInt();
        e.run = msg.value(QStringLiteral("run")).toInt();
        e.mem = msg.value(QStringLiteral("mem")).toInt();
        e.rank = msg.value(QStringLiteral("rank")).toInt();
        e.sbt = static_cast<qint64>(msg.value(QStringLiteral("sbt")).toDouble());
        e.name = msg.value(QStringLiteral("name")).toString();
        e.uname = msg.value(QStringLiteral("uname")).toString();
        if (e.id > lastId)
            lastId = e.id;
        out.push_back(e);
    }
    return out;
}

void UhuntClient::fetchUserId(const QString &uname)
{
    QNetworkReply *reply =
        get(QStringLiteral("/uname2uid/") + QString::fromUtf8(QUrl::toPercentEncoding(uname)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, uname]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Look up user"),
                               QStringLiteral("Could not reach uhunt: %1").arg(reply->errorString()));
            return;
        }
        const int uid = reply->readAll().trimmed().toInt();
        emit userIdResolved(uname, uid);
    });
}

void UhuntClient::fetchProblems()
{
    QNetworkReply *reply = get(QStringLiteral("/p"), 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load problems"), reply->errorString());
            return;
        }
        QString parseError;
        const QJsonArray arr = parseArray(reply->readAll(), &parseError);
        if (!parseError.isEmpty()) {
            emit errorOccurred(QStringLiteral("Load problems"), parseError);
            return;
        }
        emit problemsFetched(parseProblems(arr));
    });
}

void UhuntClient::fetchUserSubmissions(int uid, int minSid)
{
    const QString path = minSid > 0 ? QStringLiteral("/subs-user/%1/%2").arg(uid).arg(minSid)
                                    : QStringLiteral("/subs-user/%1").arg(uid);
    QNetworkReply *reply = get(path, 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply, uid]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load submissions"), reply->errorString());
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        emit userSubmissionsFetched(uid, parseSubmissions(root.value(QStringLiteral("subs")).toArray()));
    });
}

void UhuntClient::fetchUserSubmissionsLast(int uid, int count)
{
    QNetworkReply *reply = get(QStringLiteral("/subs-user-last/%1/%2").arg(uid).arg(count), 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply, uid]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load submissions"), reply->errorString());
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        emit userSubmissionsFetched(uid, parseSubmissions(root.value(QStringLiteral("subs")).toArray()));
    });
}

void UhuntClient::fetchRanklist(int uid, int nabove, int nbelow)
{
    QNetworkReply *reply =
        get(QStringLiteral("/ranklist/%1/%2/%3").arg(uid).arg(nabove).arg(nbelow), 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load ranklist"), reply->errorString());
            return;
        }
        emit ranklistFetched(parseRanklist(parseArray(reply->readAll())));
    });
}

void UhuntClient::fetchGlobalRank(int pos, int count)
{
    QNetworkReply *reply = get(QStringLiteral("/rank/%1/%2").arg(pos).arg(count), 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load ranklist"), reply->errorString());
            return;
        }
        emit ranklistFetched(parseRanklist(parseArray(reply->readAll())));
    });
}

void UhuntClient::fetchCpBook(int edition)
{
    QNetworkReply *reply = get(QStringLiteral("/cpbook/%1").arg(edition), 30000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Load CP book categories"), reply->errorString());
            return;
        }
        emit cpBookFetched(parseCpBook(parseArray(reply->readAll())));
    });
}

void UhuntClient::loadCpBook(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QStringLiteral("Load CP book exercises"),
                           QStringLiteral("Could not open the bundled exercise data (%1).")
                               .arg(resourcePath));
        return;
    }
    emit cpBookFetched(parseCpBook(parseArray(file.readAll())));
}

void UhuntClient::startLivePolling(qint64 lastId)
{
    if (m_liveActive)
        return;
    m_liveActive = true;
    m_lastPollId = lastId;
    pollOnce(m_lastPollId);
}

void UhuntClient::stopLivePolling()
{
    if (!m_liveActive)
        return;
    m_liveActive = false;
    if (QNetworkReply *reply = m_liveReply) {
        // Clear the member first: abort() emits finished() synchronously, and
        // the finished handler must not see a dangling/duplicated reply.
        m_liveReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    emit livePollingStopped();
}

void UhuntClient::pollOnce(qint64 pollId)
{
    if (!m_liveActive)
        return;
    QNetworkReply *reply = get(QStringLiteral("/poll/%1").arg(pollId), kLivePollTimeoutMs);
    m_liveReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (m_liveReply == reply)
            m_liveReply = nullptr;
        reply->deleteLater();
        if (!m_liveActive || reply->error() == QNetworkReply::OperationCanceledError)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Live feed"), reply->errorString());
            return;
        }
        const QJsonArray arr = parseArray(reply->readAll());
        if (!arr.isEmpty()) {
            const QVector<LiveEvent> events = parseLiveEvents(arr, m_lastPollId);
            emit liveEventsReceived(events, m_lastPollId);
        }
        // Re-arm the long poll.
        if (m_liveActive)
            pollOnce(m_lastPollId);
    });
}
