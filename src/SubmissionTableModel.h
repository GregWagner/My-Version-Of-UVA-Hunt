#pragma once

#include <QAbstractTableModel>

#include "Types.h"

// Table of the current user's submissions.
class SubmissionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColSid = 0,
        ColProblem,
        ColVerdict,
        ColLanguage,
        ColRuntime,
        ColRank,
        ColTime,
        ColumnCount,
    };

    explicit SubmissionTableModel(const ProblemCatalog *catalog, QObject *parent = nullptr);

    void setSubmissions(const QVector<Submission> &submissions);
    void clear();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    const ProblemCatalog *m_catalog;
    QVector<Submission> m_subs;
};
