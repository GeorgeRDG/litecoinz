// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RESULTSDIALOG_H
#define BITCOIN_QT_RESULTSDIALOG_H

#include "platformstyle.h"
#include <QDialog>

namespace Ui {
    class ResultsDialog;
}

/** Dialog for editing an address and associated information.
 */
class ResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResultsDialog(const PlatformStyle *platformStyle, const QString& strTitle, const QString& strLabel_1, const QString& strLabel_2, const QString& strLabel_3, const QString& strLabel_4, const QString& strLabel_5, const QString& strLabel_6, QWidget *parent);
    ~ResultsDialog();

public Q_SLOTS:
    void on_buttonBox_clicked();

private:
    Ui::ResultsDialog *ui;
    QString strTitle;
    QString strLabel_1;
    QString strLabel_2;
    QString strLabel_3;
    QString strLabel_4;
    QString strLabel_5;
    QString strLabel_6;
};

#endif // BITCOIN_QT_RESULTSDIALOG_H
