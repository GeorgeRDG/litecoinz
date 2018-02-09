// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ADDRESSBOOKDIALOG_H
#define BITCOIN_QT_ADDRESSBOOKDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class PlatformStyle;

namespace Ui {
    class AddressBookDialog;
}

/** Dialog for LitecoinZ Address Book */
class AddressBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddressBookDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~AddressBookDialog();

    void setModel(WalletModel *model);

public Q_SLOTS:

private:
    Ui::AddressBookDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    // Receiving Addresses Tab
    void on_newReceivingZAddress_clicked();
    void on_copyReceivingZAddress_clicked();
    void on_exportReceivingZAddress_clicked();
    void on_newReceivingAddress_clicked();
    void on_copyReceivingAddress_clicked();
    void on_exportReceivingAddress_clicked();

    // Sending Addresses Tab
    void on_newSendingAddress_clicked();
    void on_copySendingAddress_clicked();
    void on_deleteSendingAddress_clicked();
    void on_exportSendingAddress_clicked();

Q_SIGNALS:

};

#endif // BITCOIN_QT_ADDRESSBOOKDIALOG_H
