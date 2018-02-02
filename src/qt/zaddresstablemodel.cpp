// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zaddresstablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

#include <QFont>
#include <QDebug>

struct ZAddressTableEntry
{
    QString address;

    ZAddressTableEntry() {}
    ZAddressTableEntry(const QString &address):
        address(address) {}
};

struct ZAddressTableEntryLessThan
{
    bool operator()(const ZAddressTableEntry &a, const ZAddressTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const ZAddressTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const ZAddressTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class ZAddressTablePriv
{
public:
    CWallet *wallet;
    QList<ZAddressTableEntry> cachedAddressTable;
    ZAddressTableModel *parent;

    ZAddressTablePriv(CWallet *wallet, ZAddressTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshAddressTable()
    {
        cachedAddressTable.clear();
        {
            LOCK(wallet->cs_wallet);

            std::set<libzcash::PaymentAddress> addresses;
            wallet->GetPaymentAddresses(addresses);

            for (auto addr : addresses ) {
                if (wallet->HaveSpendingKey(addr)) {
                    cachedAddressTable.append(ZAddressTableEntry(QString::fromStdString(CZCPaymentAddress(addr).ToString())));
               }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedAddressTable.begin(), cachedAddressTable.end(), ZAddressTableEntryLessThan());
    }

    int size()
    {
        return cachedAddressTable.size();
    }

    ZAddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTable.size())
        {
            return &cachedAddressTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

ZAddressTableModel::ZAddressTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Receiving Z-Address");
    priv = new ZAddressTablePriv(wallet, this);
    priv->refreshAddressTable();
}

ZAddressTableModel::~ZAddressTableModel()
{
    delete priv;
}

int ZAddressTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ZAddressTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ZAddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ZAddressTableEntry *rec = static_cast<ZAddressTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return rec->address;
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if(index.column() == Address)
        {
            font = GUIUtil::fixedPitchFont();
        }
        return font;
    }
    return QVariant();
}

QVariant ZAddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

QModelIndex ZAddressTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ZAddressTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

QString ZAddressTableModel::addRow(const QString &address)
{
    std::string strAddress = address.toStdString();

/*
    // Generate a new address
    CPubKey newKey;
    if(!wallet->GetKeyFromPool(newKey))
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid())
        {
            // Unlock wallet failed or was cancelled
            editStatus = WALLET_UNLOCK_FAILURE;
            return QString();
        }
        if(!wallet->GetKeyFromPool(newKey))
        {
            editStatus = KEY_GENERATION_FAILURE;
            return QString();
        }
    }
    strAddress = CBitcoinAddress(newKey.GetID()).ToString();
    else
    {
        return QString();
    }

    // Add entry
    {
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(CBitcoinAddress(strAddress).Get(), "",
                               (type == Send ? "send" : "receive"));
    }
*/
    return QString::fromStdString(strAddress);
}

void ZAddressTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
