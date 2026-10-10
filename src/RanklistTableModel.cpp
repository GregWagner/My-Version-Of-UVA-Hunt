#include "RanklistTableModel.h"

RanklistTableModel::RanklistTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void RanklistTableModel::setRanks(const QVector<UserRank> &ranks)
{
    beginResetModel();
    m_ranks = ranks;
    endResetModel();
}

int RanklistTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_ranks.size();
}

int RanklistTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant RanklistTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_ranks.size())
        return {};

    const UserRank &r = m_ranks.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColRank: return r.rank;
        case ColUser: return r.username;
        case ColName: return r.name;
        case ColAc: return r.ac;
        case ColSubs: return r.nos;
        case ColActivity:
            if (r.activity.size() >= 5)
                return QStringLiteral("%1 / %2 / %3 / %4 / %5")
                    .arg(r.activity.at(0))
                    .arg(r.activity.at(1))
                    .arg(r.activity.at(2))
                    .arg(r.activity.at(3))
                    .arg(r.activity.at(4));
            return QString();
        default: break;
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case ColRank:
        case ColAc:
        case ColSubs:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    case Qt::UserRole:
        switch (index.column()) {
        case ColRank: return r.rank;
        case ColUser: return r.username;
        case ColName: return r.name;
        case ColAc: return r.ac;
        case ColSubs: return r.nos;
        default: break;
        }
        break;
    default:
        break;
    }
    return {};
}

QVariant RanklistTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColRank: return QStringLiteral("Rank");
    case ColUser: return QStringLiteral("Username");
    case ColName: return QStringLiteral("Name");
    case ColAc: return QStringLiteral("Solved");
    case ColSubs: return QStringLiteral("Submissions");
    case ColActivity: return QStringLiteral("Activity 2d/7d/31d/3mo/1yr");
    default: break;
    }
    return {};
}
