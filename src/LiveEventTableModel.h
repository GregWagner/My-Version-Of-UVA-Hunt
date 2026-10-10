#pragma once

#include <QAbstractTableModel>

#include "Types.h"

// Rolling list of live submissions from /api/poll, newest first.
class LiveEventTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColSid = 0,
        ColProblem,
        ColUser,
        ColVerdict,
        ColLanguage,
        ColRuntime,
        ColRank,
        ColTime,
        ColumnCount,
    };

    explicit LiveEventTableModel(const ProblemCatalog *catalog, QObject *parent = nullptr);

    void addEvents(const QVector<LiveEvent> &events);
    void clear();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    const ProblemCatalog *m_catalog;
    QVector<LiveEvent> m_events;
    static constexpr int kMaxRows = 500;
};
