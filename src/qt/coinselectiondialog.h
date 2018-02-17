// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_COINSELECTIONDIALOG_H
#define BITCOIN_QT_COINSELECTIONDIALOG_H

#include "amount.h"

#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QString>
#include <QSortFilterProxyModel>

class PlatformStyle;
class WalletModel;
class CCoinControl;
class UnspentTableModel;

namespace Ui {
    class CoinSelectionDialog;
}

#define ASYMP_UTF8 "\xE2\x89\x88"

class CoinSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoinSelectionDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CoinSelectionDialog();

    void setModel(UnspentTableModel *model);
    void setWalletModel(WalletModel *model);

    // static because also called from sendcoinsdialog
    static void updateLabels(WalletModel*, QDialog*);
    static QString getPriorityLabel(double dPriority, double mempoolEstimatePriority);

    static QList<CAmount> payAmounts;
    static CCoinControl *coinSelection;
    static bool fSubtractFeeFromAmount;

private:
    Ui::CoinSelectionDialog *ui;
    UnspentTableModel *model;
    WalletModel *walletModel;
    QSortFilterProxyModel *proxyModelUnspentZ;
    QSortFilterProxyModel *proxyModelUnspentT;

    const PlatformStyle *platformStyle;

    void updateView();

private Q_SLOTS:
    void clipboardQuantity();
    void clipboardAmount();
    void clipboardFee();
    void clipboardAfterFee();
    void clipboardBytes();
    void clipboardPriority();
    void clipboardLowOutput();
    void clipboardChange();

    void selectionUnspentZChanged();
    void selectionUnspentTChanged();

    void buttonBoxClicked(QAbstractButton*);
};

#endif // BITCOIN_QT_COINSELECTIONDIALOG_H
