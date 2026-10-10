#pragma once

#include <QAbstractTableModel>
#include <QSet>

#include "Types.h"

// Displays a list of problems, optionally highlighting the ones the current
// user has solved.
class ProblemTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColNumber = 0,
        ColTitle,
        ColDacu,
        ColAc,
        ColWa,
        ColTle,
        ColRtl,
        ColStatus,
        ColSolved,
        ColumnCount,
    };

    explicit ProblemTableModel(QObject *parent = nullptr);

    void setProblems(const QVector<Problem> &problems);
    void setSolvedPids(const QSet<int> &solvedPids);
    void clearSolved();

    const Problem *problemAt(int row) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<Problem> m_problems;
    QSet<int> m_solved;
};
