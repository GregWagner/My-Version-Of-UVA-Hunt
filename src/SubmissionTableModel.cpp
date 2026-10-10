#include "SubmissionTableModel.h"

#include <QBrush>
#include <QColor>

SubmissionTableModel::SubmissionTableModel(const ProblemCatalog *catalog, QObject *parent)
    : QAbstractTableModel(parent)
    , m_catalog(catalog)
{
}

void SubmissionTableModel::setSubmissions(const QVector<Submission> &submissions)
{
    beginResetModel();
    m_subs = submissions;
    endResetModel();
}

void SubmissionTableModel::clear()
{
    beginResetModel();
    m_subs.clear();
    endResetModel();
}

int SubmissionTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_subs.size();
}

int SubmissionTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SubmissionTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_subs.size())
        return {};

    const Submission &s = m_subs.at(index.row());
    const Problem *p = m_catalog ? m_catalog->problem(s.pid) : nullptr;

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColSid: return s.sid;
        case ColProblem:
            return p ? QStringLiteral("%1 - %2").arg(p->num).arg(p->title)
                     : QStringLiteral("pid %1").arg(s.pid);
        case ColVerdict: return verdictName(s.ver);
        case ColLanguage: return languageName(s.lan);
        case ColRuntime: return formatRuntime(s.run, s.ver);
        case ColRank: return s.rank > 0 ? QString::number(s.rank) : QStringLiteral("-");
        case ColTime: return formatUnixTime(s.sbt);
        default: break;
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case ColSid:
        case ColRuntime:
        case ColRank:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    case Qt::ForegroundRole:
        return QBrush(QColor(verdictForeground(s.ver)));
    case Qt::UserRole:
        switch (index.column()) {
        case ColSid: return s.sid;
        case ColProblem: return p ? p->num : s.pid;
        case ColVerdict: return s.ver;
        case ColLanguage: return s.lan;
        case ColRuntime: return s.run;
        case ColRank: return s.rank;
        case ColTime: return s.sbt;
        default: break;
        }
        break;
    default:
        break;
    }
    return {};
}

QVariant SubmissionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColSid: return QStringLiteral("SID");
    case ColProblem: return QStringLiteral("Problem");
    case ColVerdict: return QStringLiteral("Verdict");
    case ColLanguage: return QStringLiteral("Language");
    case ColRuntime: return QStringLiteral("Runtime (s)");
    case ColRank: return QStringLiteral("Rank");
    case ColTime: return QStringLiteral("Submitted");
    default: break;
    }
    return {};
}
