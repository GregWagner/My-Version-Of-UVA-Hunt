#pragma once

#include <QDialog>

#include "Types.h"

// Shows the full statistics for one problem. All data comes from the already
// loaded problem catalog, so no extra network request is needed.
class ProblemDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    ProblemDetailsDialog(const Problem &problem, bool solved, QWidget *parent = nullptr);
};
