#include "ProblemFilterProxyModel.h"

ProblemFilterProxyModel::ProblemFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSortRole(Qt::UserRole);
    setDynamicSortFilter(true);
}

void ProblemFilterProxyModel::setSearchText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (m_search == trimmed)
        return;
    beginFilterChange();
    m_search = trimmed;
    endFilterChange();
}

void ProblemFilterProxyModel::setStatusFilter(StatusFilter filter)
{
    if (m_status == filter)
        return;
    beginFilterChange();
    m_status = filter;
    endFilterChange();
}

void ProblemFilterProxyModel::setCategoryNumbers(const QSet<int> &nums)
{
    beginFilterChange();
    m_categoryNums = nums;
    endFilterChange();
}

bool ProblemFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto *model = qobject_cast<const ProblemTableModel *>(sourceModel());
    if (!model)
        return true;
    const Problem *p = model->problemAt(sourceRow);
    if (!p)
        return false;

    if (!m_search.isEmpty()) {
        const QString number = QString::number(p->num);
        if (!number.contains(m_search) && !p->title.contains(m_search, Qt::CaseInsensitive))
            return false;
    }

    if (m_status != All) {
        const bool solved = model->data(model->index(sourceRow, ProblemTableModel::ColSolved),
                                        Qt::UserRole)
                                .toInt()
            != 0;
        if (m_status == Solved && !solved)
            return false;
        if (m_status == Unsolved && solved)
            return false;
    }

    if (!m_categoryNums.isEmpty() && !m_categoryNums.contains(p->num))
        return false;

    return true;
}
