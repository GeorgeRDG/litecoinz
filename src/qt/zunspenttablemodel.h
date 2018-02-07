// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZUNSPENTTABLEMODEL_H
#define BITCOIN_QT_ZUNSPENTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class ZUnspentTablePriv;
class WalletModel;

class CWallet;

/**
   Qt model of the unspent transaction.
 */
class ZUnspentTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ZUnspentTableModel(CWallet *wallet, WalletModel *parent = 0);
    ~ZUnspentTableModel();

    enum ColumnIndex {
        Address = 0,
        Balance = 1
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    /*@}*/

private:
    WalletModel *walletModel;
    CWallet *wallet;
    ZUnspentTablePriv *priv;
    QStringList columns;

public Q_SLOTS:

    friend class ZUnspentTablePriv;
};

#endif // BITCOIN_QT_UNSPENTTABLEMODEL_H
