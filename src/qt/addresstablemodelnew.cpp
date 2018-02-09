// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addresstablemodelnew.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

#include <QFont>
#include <QDebug>

const QString AddressTableNewModel::Send = "S";
const QString AddressTableNewModel::Receive = "R";
const QString AddressTableNewModel::ZReceive = "Z";

struct AddressTableNewEntry
{
    enum Type {
        Sending,
        Receiving,
        ZReceiving,
        Hidden /* QSortFilterProxyModel will filter these out */
    };

    Type type;
    QString label;
    QString address;

    AddressTableNewEntry() {}
    AddressTableNewEntry(Type type, const QString &label, const QString &address):
        type(type), label(label), address(address) {}
};

struct AddressTableNewEntryLessThan
{
    bool operator()(const AddressTableNewEntry &a, const AddressTableNewEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const AddressTableNewEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const AddressTableNewEntry &b) const
    {
        return a < b.address;
    }
};

/* Determine address type from address purpose */
static AddressTableNewEntry::Type translateTransactionType(const QString &strPurpose, bool isMine)
{
    AddressTableNewEntry::Type addressType = AddressTableNewEntry::Hidden;
    // "refund" addresses aren't shown, and change addresses aren't in mapAddressBook at all.
    if (strPurpose == "send")
        addressType = AddressTableNewEntry::Sending;
    else if (strPurpose == "receive")
        addressType = AddressTableNewEntry::Receiving;
    else if (strPurpose == "unknown" || strPurpose == "") // if purpose not set, guess
        addressType = (isMine ? AddressTableNewEntry::Receiving : AddressTableNewEntry::Sending);
    return addressType;
}

// Private implementation
class AddressTableNewPriv
{
public:
    CWallet *wallet;
    QList<AddressTableNewEntry> cachedAddressTableNew;
    AddressTableNewModel *parent;

    AddressTableNewPriv(CWallet *wallet, AddressTableNewModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshAddressTableNew()
    {
        cachedAddressTableNew.clear();
        {
            LOCK(wallet->cs_wallet);

            // Transparent address
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
                bool fMine = IsMine(*wallet, address.Get());
                AddressTableNewEntry::Type addressType = translateTransactionType(
                        QString::fromStdString(item.second.purpose), fMine);
                const std::string& strName = item.second.name;
                cachedAddressTableNew.append(AddressTableNewEntry(addressType,
                                  QString::fromStdString(strName),
                                  QString::fromStdString(address.ToString())));
            }

            // Shielded address
            std::set<libzcash::PaymentAddress> addresses;
            wallet->GetPaymentAddresses(addresses);
            for (auto addr : addresses ) {
                AddressTableNewEntry::Type addressType = AddressTableNewEntry::ZReceiving;
                const std::string& strName = "";
                if (wallet->HaveSpendingKey(addr)) {
                    cachedAddressTableNew.append(AddressTableNewEntry(addressType,
                                      QString::fromStdString(strName),
                                      QString::fromStdString(CZCPaymentAddress(addr).ToString())));
               }
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTableNew list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedAddressTableNew.begin(), cachedAddressTableNew.end(), AddressTableNewEntryLessThan());
    }

    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status)
    {
        // Find address / label in model
        QList<AddressTableNewEntry>::iterator lower = qLowerBound(
            cachedAddressTableNew.begin(), cachedAddressTableNew.end(), address, AddressTableNewEntryLessThan());
        QList<AddressTableNewEntry>::iterator upper = qUpperBound(
            cachedAddressTableNew.begin(), cachedAddressTableNew.end(), address, AddressTableNewEntryLessThan());
        int lowerIndex = (lower - cachedAddressTableNew.begin());
        int upperIndex = (upper - cachedAddressTableNew.begin());
        bool inModel = (lower != upper);
        AddressTableNewEntry::Type newEntryType = translateTransactionType(purpose, isMine);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "AddressTableNewPriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedAddressTableNew.insert(lowerIndex, AddressTableNewEntry(newEntryType, label, address));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "AddressTableNewPriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            lower->type = newEntryType;
            lower->label = label;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "AddressTableNewPriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedAddressTableNew.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedAddressTableNew.size();
    }

    AddressTableNewEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTableNew.size())
        {
            return &cachedAddressTableNew[idx];
        }
        else
        {
            return 0;
        }
    }
};

AddressTableNewModel::AddressTableNewModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Label") << tr("Address") << tr("AddressType");
    priv = new AddressTableNewPriv(wallet, this);
    priv->refreshAddressTableNew();
}

AddressTableNewModel::~AddressTableNewModel()
{
    delete priv;
}

int AddressTableNewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AddressTableNewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AddressTableNewModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    AddressTableNewEntry *rec = static_cast<AddressTableNewEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            if(rec->label.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no label)");
            }
            else
            {
                return rec->label;
            }
        case Address:
            return rec->address;
        case AddressType:
            return rec->type;

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
        case AddressTableNewEntry::Sending:
            return Send;
        case AddressTableNewEntry::Receiving:
            return Receive;
        case AddressTableNewEntry::ZReceiving:
            return ZReceive;
        default: break;
        }
    }
    return QVariant();
}

bool AddressTableNewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    AddressTableNewEntry *rec = static_cast<AddressTableNewEntry*>(index.internalPointer());
    std::string strPurpose = (rec->type == AddressTableNewEntry::Sending ? "send" : "receive");
    editStatus = OK;

    if(role == Qt::EditRole)
    {
        LOCK(wallet->cs_wallet); /* For SetAddressBook / DelAddressBook */
        CTxDestination curAddress = CBitcoinAddress(rec->address.toStdString()).Get();
        if(index.column() == Label)
        {
            // Do nothing, if old label == new label
            if(rec->label == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            wallet->SetAddressBook(curAddress, value.toString().toStdString(), strPurpose);
        } else if(index.column() == Address) {
            CTxDestination newAddress = CBitcoinAddress(value.toString().toStdString()).Get();
            // Refuse to set invalid address, set error status and return false
            if(boost::get<CNoDestination>(&newAddress))
            {
                editStatus = INVALID_ADDRESS;
                return false;
            }
            // Do nothing, if old address == new address
            else if(newAddress == curAddress)
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate addresses to prevent accidental deletion of addresses, if you try
            // to paste an existing address over another address (with a different label)
            else if(wallet->mapAddressBook.count(newAddress))
            {
                editStatus = DUPLICATE_ADDRESS;
                return false;
            }
            // Double-check that we're not overwriting a receiving address
            else if(rec->type == AddressTableNewEntry::Sending)
            {
                // Remove old entry
                wallet->DelAddressBook(curAddress);
                // Add new entry with new address
                wallet->SetAddressBook(newAddress, rec->label.toStdString(), strPurpose);
            }
        }
        return true;
    }
    return false;
}

QVariant AddressTableNewModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags AddressTableNewModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    AddressTableNewEntry *rec = static_cast<AddressTableNewEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    if(rec->type == AddressTableNewEntry::Sending ||
      (rec->type == AddressTableNewEntry::Receiving && index.column()==Label))
    {
        retval |= Qt::ItemIsEditable;
    }
    return retval;
}

QModelIndex AddressTableNewModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AddressTableNewEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void AddressTableNewModel::updateEntry(const QString &address,
        const QString &label, bool isMine, const QString &purpose, int status)
{
    // Update address book model from LitecoinZ core
    priv->updateEntry(address, label, isMine, purpose, status);
}

QString AddressTableNewModel::addRow(const QString &type, const QString &label, const QString &address)
{
    std::string strLabel = label.toStdString();
    std::string strAddress = address.toStdString();

    editStatus = OK;

    if(type == Send)
    {
        if(!walletModel->validateAddress(address))
        {
            editStatus = INVALID_ADDRESS;
            return QString();
        }
        // Check for duplicate addresses
        {
            LOCK(wallet->cs_wallet);
            if(wallet->mapAddressBook.count(CBitcoinAddress(strAddress).Get()))
            {
                editStatus = DUPLICATE_ADDRESS;
                return QString();
            }
        }
    }
    else if(type == Receive)
    {
        // Generate a new address to associate with given label
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
    }
    else if(type == ZReceive)
    {
        // Generate a new z-address
        // TODO
    }
    else
    {
        return QString();
    }

    // Add entry
    {
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(CBitcoinAddress(strAddress).Get(), strLabel,
                               (type == Send ? "send" : "receive"));
    }
    return QString::fromStdString(strAddress);
}

bool AddressTableNewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    AddressTableNewEntry *rec = priv->index(row);
    if(count != 1 || !rec || rec->type == AddressTableNewEntry::Receiving)
    {
        // Can only remove one row at a time, and cannot remove rows not in model.
        // Also refuse to remove receiving addresses.
        return false;
    }
    {
        LOCK(wallet->cs_wallet);
        wallet->DelAddressBook(CBitcoinAddress(rec->address.toStdString()).Get());
    }
    return true;
}

/* Look up label for address in address book, if not found return empty string.
 */
QString AddressTableNewModel::labelForAddress(const QString &address) const
{
    {
        LOCK(wallet->cs_wallet);
        CBitcoinAddress address_parsed(address.toStdString());
        std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(address_parsed.Get());
        if (mi != wallet->mapAddressBook.end())
        {
            return QString::fromStdString(mi->second.name);
        }
    }
    return QString();
}

int AddressTableNewModel::lookupAddress(const QString &address) const
{
    QModelIndexList lst = match(index(0, Address, QModelIndex()),
                                Qt::EditRole, address, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void AddressTableNewModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
