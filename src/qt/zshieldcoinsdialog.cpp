// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "zshieldcoinsdialog.h"
#include "ui_zshieldcoinsdialog.h"

#include "addressbookpage.h"
#include "zaddresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"

#include <QApplication>

ZShieldCoinsDialog::ZShieldCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZShieldCoinsDialog),
    model(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    ui->shieldButton->setEnabled(false);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(accept()));
}

ZShieldCoinsDialog::~ZShieldCoinsDialog()
{
    delete ui;
}

void ZShieldCoinsDialog::on_shieldButton_clicked()
{
    // todo
    // todo
}

void ZShieldCoinsDialog::on_AddressBookButton_clicked()
{
    if(!model)
        return;

    AddressBookPage dlg(platformStyle, AddressBookPage::ForZSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    dlg.setZModel(model->getZAddressTableModel());
    if(dlg.exec())
    {
        ui->reqShieldAddress->setText(dlg.getReturnValue());
        ui->shieldButton->setEnabled(true);
        ui->shieldButton->setFocus();
    }
}

void ZShieldCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;
    clear();
}

void ZShieldCoinsDialog::setAddress(const QString &address)
{
    ui->reqShieldAddress->setText(address);
    ui->shieldButton->setFocus();
}

void ZShieldCoinsDialog::setFocus()
{
    ui->reqShieldAddress->setFocus();
}

void ZShieldCoinsDialog::clear()
{
    // clear UI elements
    ui->reqShieldAddress->clear();
}

bool ZShieldCoinsDialog::isClear()
{
    return ui->reqShieldAddress->text().isEmpty();
}
