// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "unspenttablemodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

#include <QFont>
#include <QDebug>

struct UnspentTableEntry
{
    QString address;
    QString balance;

    UnspentTableEntry() {}
    UnspentTableEntry(const QString &address, const QString &balance):
        address(address), balance(balance) {}
};

struct UnspentTableEntryLessThan
{
    bool operator()(const UnspentTableEntry &a, const UnspentTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const UnspentTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const UnspentTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class UnspentTablePriv
{
public:
    CWallet *wallet;
    QList<UnspentTableEntry> cachedUnspentTable;
    UnspentTableModel *parent;

    UnspentTablePriv(CWallet *wallet, UnspentTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshUnspentTable()
    {
        cachedUnspentTable.clear();
        {
            std::vector<COutput> vecOutputs;

            LOCK(wallet->cs_wallet);
            wallet->AvailableCoins(vecOutputs, false, NULL, true);

            BOOST_FOREACH(const COutput& out, vecOutputs) {
                if (out.tx->IsCoinBase())
                    continue;

                if (!out.fSpendable)
                    continue;

                CTxDestination address;
                if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, address)) {
                    cachedUnspentTable.append(UnspentTableEntry(
                                                  QString::fromStdString(CBitcoinAddress(address).ToString()),
                                                  BitcoinUnits::format(BitcoinUnits::LTZ, out.tx->vout[out.i].nValue)
                                             )
                    );
                }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedUnspentTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedUnspentTable.begin(), cachedUnspentTable.end(), UnspentTableEntryLessThan());
    }

    int size()
    {
        return cachedUnspentTable.size();
    }

    UnspentTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedUnspentTable.size())
        {
            return &cachedUnspentTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

UnspentTableModel::UnspentTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Address") << tr("Balance");
    priv = new UnspentTablePriv(wallet, this);
    priv->refreshUnspentTable();
}

UnspentTableModel::~UnspentTableModel()
{
    delete priv;
}

int UnspentTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int UnspentTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant UnspentTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    UnspentTableEntry *rec = static_cast<UnspentTableEntry*>(index.internalPointer());

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

QVariant UnspentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags UnspentTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    UnspentTableEntry *rec = static_cast<UnspentTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex UnspentTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    UnspentTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}
