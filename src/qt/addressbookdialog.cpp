// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressbookdialog.h"
#include "ui_addressbookdialog.h"

AddressBookDialog::AddressBookDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookDialog),
    model(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

}

void AddressBookDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(!model)
        return;
}

AddressBookDialog::~AddressBookDialog()
{
    delete ui;
}


// Receiving Addresses Tab
void AddressBookDialog::on_newReceivingZAddress_clicked()
{

}

void AddressBookDialog::on_copyReceivingZAddress_clicked()
{

}

void AddressBookDialog::on_exportReceivingZAddress_clicked()
{

}

void AddressBookDialog::on_newReceivingAddress_clicked()
{

}

void AddressBookDialog::on_copyReceivingAddress_clicked()
{

}

void AddressBookDialog::on_exportReceivingAddress_clicked()
{

}


// Sending Addresses Tab
void AddressBookDialog::on_newSendingAddress_clicked()
{

}

void AddressBookDialog::on_copySendingAddress_clicked()
{

}

void AddressBookDialog::on_deleteSendingAddress_clicked()
{

}

void AddressBookDialog::on_exportSendingAddress_clicked()
{

}

