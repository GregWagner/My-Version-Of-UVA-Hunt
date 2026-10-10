#include "ProblemTableModel.h"

#include <QBrush>
#include <QColor>
#include <QFont>

ProblemTableModel::ProblemTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ProblemTableModel::setProblems(const QVector<Problem> &problems)
{
    beginResetModel();
    m_problems = problems;
    endResetModel();
}

void ProblemTableModel::setSolvedPids(const QSet<int> &solvedPids)
{
    beginResetModel();
    m_solved = solvedPids;
    endResetModel();
}

void ProblemTableModel::clearSolved()
{
    if (m_solved.isEmpty())
        return;
    beginResetModel();
    m_solved.clear();
    endResetModel();
}

const Problem *ProblemTableModel::problemAt(int row) const
{
    if (row < 0 || row >= m_problems.size())
        return nullptr;
    return &m_problems.at(row);
}

int ProblemTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_problems.size();
}

int ProblemTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ProblemTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_problems.size())
        return {};

    const Problem &p = m_problems.at(index.row());
    const bool solved = m_solved.contains(p.pid);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColNumber: return p.num;
        case ColTitle: return p.title;
        case ColDacu: return p.dacu;
        case ColAc: return p.ac;
        case ColWa: return p.wa;
        case ColTle: return p.tle;
        case ColRtl: return p.rtl;
        case ColStatus:
            return p.status == 2 ? QStringLiteral("special")
                                 : (p.status == 1 ? QStringLiteral("normal") : QStringLiteral("offline"));
        case ColSolved: return solved ? QStringLiteral("yes") : QString();
        default: break;
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case ColNumber:
        case ColDacu:
        case ColAc:
        case ColWa:
        case ColTle:
        case ColRtl:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    case Qt::BackgroundRole:
        if (solved)
            return QBrush(QColor(210, 240, 210));
        break;
    case Qt::ForegroundRole:
        if (solved)
            return QBrush(QColor(20, 90, 30));
        break;
    case Qt::FontRole:
        if (solved) {
            QFont f;
            f.setBold(true);
            return f;
        }
        break;
    case Qt::UserRole:  // sort key
        switch (index.column()) {
        case ColNumber: return p.num;
        case ColTitle: return p.title;
        case ColDacu: return p.dacu;
        case ColAc: return p.ac;
        case ColWa: return p.wa;
        case ColTle: return p.tle;
        case ColRtl: return p.rtl;
        case ColStatus: return p.status;
        case ColSolved: return solved ? 1 : 0;
        default: break;
        }
        break;
    case Qt::ToolTipRole:
        return QStringLiteral("%1 - %2\nDACU: %3\nAccepted submissions: %4\nRun time limit: %5 ms")
            .arg(p.num)
            .arg(p.title)
            .arg(p.dacu)
            .arg(p.ac)
            .arg(p.rtl);
    default:
        break;
    }
    return {};
}

QVariant ProblemTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColNumber: return QStringLiteral("#");
    case ColTitle: return QStringLiteral("Title");
    case ColDacu: return QStringLiteral("DACU");
    case ColAc: return QStringLiteral("AC");
    case ColWa: return QStringLiteral("WA");
    case ColTle: return QStringLiteral("TLE");
    case ColRtl: return QStringLiteral("RTL (ms)");
    case ColStatus: return QStringLiteral("Judge");
    case ColSolved: return QStringLiteral("Solved");
    default: break;
    }
    return {};
}
