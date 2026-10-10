#pragma once

#include <QSet>
#include <QSortFilterProxyModel>

#include "ProblemTableModel.h"

// Filters the problem list by free text, solved status and CP book category.
class ProblemFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum StatusFilter { All = 0, Solved = 1, Unsolved = 2 };

    explicit ProblemFilterProxyModel(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setStatusFilter(StatusFilter filter);
    void setCategoryNumbers(const QSet<int> &nums);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_search;
    StatusFilter m_status = All;
    QSet<int> m_categoryNums;
};
