// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZUNSPENTDIALOG_H
#define BITCOIN_QT_ZUNSPENTDIALOG_H

#include <QDialog>

class ZUnspentTableModel;
class PlatformStyle;

namespace Ui {
    class ZUnspentDialog;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Widget that shows a list of unspent z-transaction.
  */
class ZUnspentDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        ForSelection,
        ForEditing
    };

    explicit ZUnspentDialog(const PlatformStyle *platformStyle, Mode mode, QWidget *parent);
    ~ZUnspentDialog();

    void setModel(ZUnspentTableModel *model);
    const QString &getReturnValue() const { return returnValue; }

public Q_SLOTS:
    void done(int retval);

private:
    Ui::ZUnspentDialog *ui;
    ZUnspentTableModel *model;
    Mode mode;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;

private Q_SLOTS:
    /** Set button states based on selected tab and selection */
    void selectionChanged();

Q_SIGNALS:
    void sendCoins(QString addr);
};

#endif // BITCOIN_QT_ZUNSPENTDIALOG_H
