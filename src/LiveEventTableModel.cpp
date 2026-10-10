#include "LiveEventTableModel.h"

#include <QBrush>
#include <QColor>

LiveEventTableModel::LiveEventTableModel(const ProblemCatalog *catalog, QObject *parent)
    : QAbstractTableModel(parent)
    , m_catalog(catalog)
{
}

void LiveEventTableModel::clear()
{
    beginResetModel();
    m_events.clear();
    endResetModel();
}

void LiveEventTableModel::addEvents(const QVector<LiveEvent> &events)
{
    if (events.isEmpty())
        return;

    beginInsertRows(QModelIndex(), 0, events.size() - 1);
    for (int i = events.size() - 1; i >= 0; --i)
        m_events.prepend(events.at(i));
    endInsertRows();

    if (m_events.size() > kMaxRows) {
        beginRemoveRows(QModelIndex(), kMaxRows, m_events.size() - 1);
        m_events.resize(kMaxRows);
        endRemoveRows();
    }
}

int LiveEventTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_events.size();
}

int LiveEventTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant LiveEventTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_events.size())
        return {};

    const LiveEvent &e = m_events.at(index.row());
    const Problem *p = m_catalog ? m_catalog->problem(e.pid) : nullptr;

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColSid: return e.sid;
        case ColProblem:
            return p ? QStringLiteral("%1 - %2").arg(p->num).arg(p->title)
                     : QStringLiteral("pid %1").arg(e.pid);
        case ColUser:
            return e.name.isEmpty() ? e.uname : QStringLiteral("%1 (%2)").arg(e.name, e.uname);
        case ColVerdict: return verdictName(e.ver);
        case ColLanguage: return languageName(e.lan);
        case ColRuntime: return formatRuntime(e.run, e.ver);
        case ColRank: return e.rank > 0 ? QString::number(e.rank) : QStringLiteral("-");
        case ColTime: return formatUnixTime(e.sbt);
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
        return QBrush(QColor(verdictForeground(e.ver)));
    case Qt::UserRole:
        switch (index.column()) {
        case ColSid: return e.sid;
        case ColProblem: return p ? p->num : e.pid;
        case ColUser: return e.uname;
        case ColVerdict: return e.ver;
        case ColLanguage: return e.lan;
        case ColRuntime: return e.run;
        case ColRank: return e.rank;
        case ColTime: return e.sbt;
        default: break;
        }
        break;
    default:
        break;
    }
    return {};
}

QVariant LiveEventTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColSid: return QStringLiteral("SID");
    case ColProblem: return QStringLiteral("Problem");
    case ColUser: return QStringLiteral("User");
    case ColVerdict: return QStringLiteral("Verdict");
    case ColLanguage: return QStringLiteral("Language");
    case ColRuntime: return QStringLiteral("Runtime (s)");
    case ColRank: return QStringLiteral("Rank");
    case ColTime: return QStringLiteral("Submitted");
    default: break;
    }
    return {};
}
