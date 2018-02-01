// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZSHIELDCOINSDIALOG_H
#define BITCOIN_QT_ZSHIELDCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class WalletModel;
class PlatformStyle;

namespace Ui {
    class ZShieldCoinsDialog;
}

/** Widget that shows a list of sending or receiving addresses.
  */
class ZShieldCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZShieldCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent);
    ~ZShieldCoinsDialog();

    void setModel(WalletModel *model);
    void setAddress(const QString &address);
    void setFocus();
    bool isClear();

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    /** Create a new address for receiving coins and / or add a new address book entry */
    void on_AddressBookButton_clicked();
    /** Shield coins in coinbase from t-address to z-address */
    void on_shieldButton_clicked();

private:
    Ui::ZShieldCoinsDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;
};

#endif // BITCOIN_QT_ZSHIELDCOINSDIALOG_H
