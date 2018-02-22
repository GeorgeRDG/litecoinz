// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_COINSELECTIONDIALOG_H
#define BITCOIN_QT_COINSELECTIONDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class CoinSelectionTableModel;
class PlatformStyle;

namespace Ui {
    class CoinSelectionDialog;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Dialog for coinselection transaction list */
class CoinSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoinSelectionDialog(const PlatformStyle *platformStyle,  QWidget *parent = 0);
    ~CoinSelectionDialog();

    void setModel(WalletModel *model);
    const QString &getReturnAddress() const { return returnAddress; }
    const QString &getReturnAmount() const { return returnAmount; }

private:
    Ui::CoinSelectionDialog *ui;
    WalletModel *model;
    CoinSelectionTableModel *coinSelectionModel;
    QSortFilterProxyModel *proxyModelCoinSelectionZ;
    QSortFilterProxyModel *proxyModelCoinSelectionT;
    QString returnAddress;
    QString returnAmount;

public Q_SLOTS:
    void done(int retval);

    /** Refresh table model */
    void refreshTableModel();

private Q_SLOTS:
    /** Set button states based on selected tab and selection */
    void selectionCoinSelectionZChanged();
    void selectionCoinSelectionTChanged();
};

#endif // BITCOIN_QT_COINSELECTIONDIALOG_H
