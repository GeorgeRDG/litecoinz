// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SHIELDCOINSDIALOG_H
#define BITCOIN_QT_SHIELDCOINSDIALOG_H

#include "walletmodel.h"
#include <univalue.h>

#include <QDialog>

class AddressTableModel;
class WalletModel;
class PlatformStyle;

namespace Ui {
    class ShieldCoinsDialog;
}

extern UniValue z_shieldcoinbase(const UniValue& params, bool fHelp); // in rpcwallet.cpp

/** Widget that shows a list of sending or receiving addresses.
  */
class ShieldCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShieldCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent);
    ~ShieldCoinsDialog();

    void setModel(WalletModel *model);
    void setAddress(const QString &address);
    void setFocus();
    bool isClear();

public Q_SLOTS:
    void clear();
    void done(int retval);

private Q_SLOTS:
    /** Create a new address for receiving coins and / or add a new address book entry */
    void on_AddressBookButton_clicked();
    /** Clear z-address field */
    void on_deleteButton_clicked();
    /** Shield coins in coinbase from t-address to z-address */
    void on_shieldButton_clicked();

    void updateDisplayUnit();
    void setMinimumFee();
    void updateGlobalFeeVariables();

private:
    Ui::ShieldCoinsDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;
};

#endif // BITCOIN_QT_SHIELDCOINSDIALOG_H