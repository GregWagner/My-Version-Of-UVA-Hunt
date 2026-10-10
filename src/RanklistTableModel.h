#pragma once

#include <QAbstractTableModel>

#include "Types.h"

class RanklistTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColRank = 0,
        ColUser,
        ColName,
        ColAc,
        ColSubs,
        ColActivity,
        ColumnCount,
    };

    explicit RanklistTableModel(QObject *parent = nullptr);

    void setRanks(const QVector<UserRank> &ranks);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<UserRank> m_ranks;
};
