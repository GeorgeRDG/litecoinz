// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "shieldcoinsdialog.h"
#include "ui_shieldcoinsdialog.h"

#include "bitcoinunits.h"
#include "addressbookdialog.h"
#include "addresstablemodel.h"
#include "resultsdialog.h"

#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include <univalue.h>
#include "rpc/server.h"

#include "wallet/asyncrpcoperation_shieldcoinbase.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>

ShieldCoinsDialog::ShieldCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShieldCoinsDialog),
    model(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);

    ui->shieldButton->setEnabled(false);
    ui->reqShieldAddress->setEnabled(false);

    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());

    ui->operationLimit->setMinimum(50);
    ui->operationLimit->setMaximum(5000);
    ui->operationLimit->setSingleStep(10);
    ui->operationLimit->setSuffix(" utxos");

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void ShieldCoinsDialog::done(int retval)
{
    clear();
    QDialog::done(retval);
}

ShieldCoinsDialog::~ShieldCoinsDialog()
{
    delete ui;
}

void ShieldCoinsDialog::on_shieldButton_clicked()
{
    UniValue params(UniValue::VARR);
    params.push_back("*");
    params.push_back(ui->reqShieldAddress->text().toStdString());
    params.push_back(ValueFromAmount(payTxFee.GetFeePerK()));
    params.push_back(ui->operationLimit->value());

    UniValue ret = z_shieldcoinbase(params, false);

    QString remainingUTXOs = QString("%1").arg(ret[0].get_int());
    //QString remainingValue = QString("%1").arg(ret[1].get_real());
    QString remainingValue = BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), AmountFromValue(ret[1].get_real()));
    QString shieldingUTXOs = QString("%1").arg(ret[2].get_int());
    //QString shieldingValue = QString("%1").arg(ret[3].get_real());
    QString shieldingValue = BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), AmountFromValue(ret[3].get_real()));
    QString opid = QString::fromStdString(ret[4].get_str());

    QString label1 = "Operation was submitted in background.";
    QString label2 = "remainingUTXOs: " + remainingUTXOs;
    QString label3 = "remainingValue: " + remainingValue;
    QString label4 = "shieldingUTXOs: " + shieldingUTXOs;
    QString label5 = "shieldingValue: " + shieldingValue;
    QString label6 = "opid: " + opid;

    ResultsDialog dlg(platformStyle, "Shielding coinbase.", label1, label2, label3, label4, label5, label6, this);
    dlg.exec();

    this->close();
}

void ShieldCoinsDialog::on_deleteButton_clicked()
{
    ui->reqShieldAddress->clear();
    ui->shieldButton->setEnabled(false);
}

void ShieldCoinsDialog::on_AddressBookButton_clicked()
{
    if(!model)
        return;

    AddressBookDialog dlg(platformStyle, AddressBookDialog::ForZSelection, AddressBookDialog::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->reqShieldAddress->setText(dlg.getReturnValue());
        ui->shieldButton->setEnabled(true);
        ui->shieldButton->setFocus();
    }
}

void ShieldCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // fee section
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));

        ui->customFee->setSingleStep(CWallet::minTxFee.GetFeePerK());
        updateGlobalFeeVariables();
    }
}

void ShieldCoinsDialog::updateDisplayUnit()
{
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
}

void ShieldCoinsDialog::setMinimumFee()
{
    ui->customFee->setValue(CWallet::minTxFee.GetFeePerK());
}

void ShieldCoinsDialog::updateGlobalFeeVariables()
{
    payTxFee = CFeeRate(ui->customFee->value());
}

void ShieldCoinsDialog::setAddress(const QString &address)
{
    ui->reqShieldAddress->setText(address);
    ui->shieldButton->setFocus();
}

void ShieldCoinsDialog::setFocus()
{
    ui->reqShieldAddress->setFocus();
}

void ShieldCoinsDialog::clear()
{
    // clear UI elements
    ui->reqShieldAddress->clear();
}

bool ShieldCoinsDialog::isClear()
{
    return ui->reqShieldAddress->text().isEmpty();
}
