// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_COINSELECTIONDIALOG_H
#define BITCOIN_QT_COINSELECTIONDIALOG_H

#include <QDialog>

class UnspentTableModel;
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

class CoinSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoinSelectionDialog(const PlatformStyle *platformStyle,  QWidget *parent = 0);
    ~CoinSelectionDialog();

    void setModel(UnspentTableModel *model);

private:
    Ui::CoinSelectionDialog *ui;
    UnspentTableModel *model;
    QSortFilterProxyModel *proxyModelUnspentZ;
    QSortFilterProxyModel *proxyModelUnspentT;

private Q_SLOTS:
    /** Set button states based on selected tab and selection */
    void selectionUnspentZChanged();
    void selectionUnspentTChanged();
};

#endif // BITCOIN_QT_COINSELECTIONDIALOG_H
