// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_UNSPENTPAGE_H
#define BITCOIN_QT_UNSPENTPAGE_H

#include <QDialog>

class UnspentTableModel;
class PlatformStyle;

namespace Ui {
    class UnspentPage;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Dialog for unspent transaction list */
class UnspentPage : public QDialog
{
    Q_OBJECT

public:
    explicit UnspentPage(const PlatformStyle *platformStyle,  QWidget *parent = 0);
    ~UnspentPage();

    void setModel(UnspentTableModel *model);

private:
    Ui::UnspentPage *ui;
    UnspentTableModel *model;
    QSortFilterProxyModel *proxyModelUnspentZ;
    QSortFilterProxyModel *proxyModelUnspentT;

private Q_SLOTS:
    /** Set button states based on selected tab and selection */
    void selectionUnspentZChanged();
    void selectionUnspentTChanged();
};

#endif // BITCOIN_QT_UNSPENTPAGE_H
