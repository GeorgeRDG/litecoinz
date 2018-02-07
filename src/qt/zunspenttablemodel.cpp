// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zunspenttablemodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

#include <QFont>
#include <QDebug>

struct ZUnspentTableEntry
{
    QString address;
    QString balance;

    ZUnspentTableEntry() {}
    ZUnspentTableEntry(const QString &address, const QString &balance):
        address(address), balance(balance) {}
};

struct ZUnspentTableEntryLessThan
{
    bool operator()(const ZUnspentTableEntry &a, const ZUnspentTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const ZUnspentTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const ZUnspentTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class ZUnspentTablePriv
{
public:
    CWallet *wallet;
    QList<ZUnspentTableEntry> cachedZUnspentTable;
    ZUnspentTableModel *parent;

    ZUnspentTablePriv(CWallet *wallet, ZUnspentTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshZUnspentTable()
    {
        cachedZUnspentTable.clear();
        {
            std::set<libzcash::PaymentAddress> zaddrs = {};
            int nMinDepth = 1;
            int nMaxDepth = 9999999;

            LOCK(wallet->cs_wallet);

            std::set<libzcash::PaymentAddress> addresses;
            wallet->GetPaymentAddresses(addresses);
            for (auto addr : addresses ) {
                if (wallet->HaveSpendingKey(addr)) {
                    zaddrs.insert(addr);
                }
            }

            if (zaddrs.size() > 0) {
                std::vector<CUnspentNotePlaintextEntry> entries;
                wallet->GetUnspentFilteredNotes(entries, zaddrs, nMinDepth, nMaxDepth);
                for (CUnspentNotePlaintextEntry & entry : entries) {
                    cachedZUnspentTable.append(ZUnspentTableEntry(
                                                  QString::fromStdString(CZCPaymentAddress(entry.address).ToString()),
                                                  BitcoinUnits::format(BitcoinUnits::LTZ, CAmount(entry.plaintext.value))
                                             )
                    );
                }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedZUnspentTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedZUnspentTable.begin(), cachedZUnspentTable.end(), ZUnspentTableEntryLessThan());
    }

    int size()
    {
        return cachedZUnspentTable.size();
    }

    ZUnspentTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedZUnspentTable.size())
        {
            return &cachedZUnspentTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

ZUnspentTableModel::ZUnspentTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Address") << tr("Balance");
    priv = new ZUnspentTablePriv(wallet, this);
    priv->refreshZUnspentTable();
}

ZUnspentTableModel::~ZUnspentTableModel()
{
    delete priv;
}

int ZUnspentTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ZUnspentTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ZUnspentTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ZUnspentTableEntry *rec = static_cast<ZUnspentTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case Address:
            return rec->address;
        case Balance:
            return rec->balance;
        }
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

QVariant ZUnspentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags ZUnspentTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    ZUnspentTableEntry *rec = static_cast<ZUnspentTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex ZUnspentTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ZUnspentTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}
