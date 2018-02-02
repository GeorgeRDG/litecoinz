// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZADDRESSTABLEMODEL_H
#define BITCOIN_QT_ZADDRESSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class ZAddressTablePriv;
class WalletModel;

class CWallet;

/**
   Qt model of the address book in the core. This allows views to access and modify the address book.
 */
class ZAddressTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ZAddressTableModel(CWallet *wallet, WalletModel *parent = 0);
    ~ZAddressTableModel();

    enum ColumnIndex {
        Address = 0  /**< LitecoinZ z-address */
    };

    /** Return status of insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        INVALID_ADDRESS,        /**< Unparseable address */
        DUPLICATE_ADDRESS,      /**< Address already in address book */
        WALLET_UNLOCK_FAILURE,  /**< Wallet could not be unlocked to create new receiving address */
        KEY_GENERATION_FAILURE  /**< Generating a new public key for a receiving address failed */
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    /*@}*/

    /* Add an address to the model.
       Returns the added address on success, and an empty string otherwise.
     */
    QString addRow(const QString &address);

    /* Look up row index of an address in the model.
       Return -1 if not found.
     */
    int lookupAddress(const QString &address) const;

private:
    WalletModel *walletModel;
    CWallet *wallet;
    ZAddressTablePriv *priv;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    friend class ZAddressTablePriv;
};

#endif // BITCOIN_QT_ZADDRESSTABLEMODEL_H
