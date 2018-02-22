// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinselectiontablemodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "optionsmodel.h"

#include "base58.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

#include <QFont>
#include <QDebug>

const QString CoinSelectionTableModel::ZCoinSelection = "Z";
const QString CoinSelectionTableModel::TCoinSelection = "T";

struct CoinSelectionTableEntry
{
    enum Type {
        ZCoinSelection,
        TCoinSelection,
        Hidden
    };

    Type type;
    QString address;
    CAmount balance;

    CoinSelectionTableEntry() {}
    CoinSelectionTableEntry(Type type, const QString &address, const CAmount &balance):
        type(type), address(address), balance(balance) {}
};

struct CoinSelectionTableEntryLessThan
{
    bool operator()(const CoinSelectionTableEntry &a, const CoinSelectionTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const CoinSelectionTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const CoinSelectionTableEntry &b) const
    {
        return a < b.address;
    }
};

/* Determine coinselection type from coinselection purpose */
static CoinSelectionTableEntry::Type translateCoinSelectionType(const QString &strPurpose)
{
    CoinSelectionTableEntry::Type coinselectionType = CoinSelectionTableEntry::Hidden;
    if (strPurpose == "tcoinselection")
        coinselectionType = CoinSelectionTableEntry::TCoinSelection;
    else if (strPurpose == "zcoinselection")
        coinselectionType = CoinSelectionTableEntry::ZCoinSelection;
    return coinselectionType;
}

// Private implementation
class CoinSelectionTablePriv
{
public:
    CWallet *wallet;
    WalletModel *walletModel;
    QList<CoinSelectionTableEntry> cachedCoinSelectionTable;
    CoinSelectionTableModel *parent;

    CoinSelectionTablePriv(CWallet *wallet, WalletModel *walletModel, CoinSelectionTableModel *parent):
        wallet(wallet), walletModel(walletModel), parent(parent) {}

    void refreshCoinSelectionTable()
    {
        cachedCoinSelectionTable.clear();
        {
            // T-CoinSelection
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
                    CoinSelectionTableEntry::Type unspentType = translateCoinSelectionType(QString::fromStdString("tcoinselection"));
                    cachedCoinSelectionTable.append(CoinSelectionTableEntry(unspentType,
                                                  QString::fromStdString(CBitcoinAddress(address).ToString()),
                                                  CAmount(out.tx->vout[out.i].nValue)
                                             )
                    );
                }
            }

            // Z-CoinSelection
            std::set<libzcash::PaymentAddress> zaddrs = {};
            int nMinDepth = 1;
            int nMaxDepth = 9999999;

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
                    CoinSelectionTableEntry::Type unspentType = translateCoinSelectionType(QString::fromStdString("zcoinselection"));
                    cachedCoinSelectionTable.append(CoinSelectionTableEntry(unspentType,
                                                  QString::fromStdString(CZCPaymentAddress(entry.address).ToString()),
                                                  CAmount(entry.plaintext.value)
                                             )
                    );
                }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedCoinSelectionTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedCoinSelectionTable.begin(), cachedCoinSelectionTable.end(), CoinSelectionTableEntryLessThan());
    }

    int size()
    {
        return cachedCoinSelectionTable.size();
    }

    CoinSelectionTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedCoinSelectionTable.size())
        {
            return &cachedCoinSelectionTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

CoinSelectionTableModel::CoinSelectionTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent), walletModel(parent), wallet(wallet), priv(0)
{
    columns << tr("Address") << tr("Balance");
    priv = new CoinSelectionTablePriv(wallet, walletModel, this);
    priv->refreshCoinSelectionTable();

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

void CoinSelectionTableModel::updateDisplayUnit()
{
    Q_EMIT dataChanged(index(0, Balance), index(priv->size()-1, Balance));
}

CoinSelectionTableModel::~CoinSelectionTableModel()
{
    delete priv;
}

int CoinSelectionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int CoinSelectionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant CoinSelectionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CoinSelectionTableEntry *rec = static_cast<CoinSelectionTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case Address:
            return rec->address;
        case Balance:
            return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->balance);
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
    else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case CoinSelectionTableEntry::ZCoinSelection:
            return ZCoinSelection;
        case CoinSelectionTableEntry::TCoinSelection:
            return TCoinSelection;
        default: break;
        }
    }
    return QVariant();
}

QVariant CoinSelectionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags CoinSelectionTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    CoinSelectionTableEntry *rec = static_cast<CoinSelectionTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex CoinSelectionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CoinSelectionTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

void CoinSelectionTableModel::refresh()
{
    Q_EMIT layoutAboutToBeChanged();
    priv->refreshCoinSelectionTable();
    Q_EMIT layoutChanged();
}
