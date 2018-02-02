// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zeditaddressdialog.h"
#include "ui_zeditaddressdialog.h"

#include "zaddresstablemodel.h"
#include "guiutil.h"

#include <QDataWidgetMapper>
#include <QMessageBox>

ZEditAddressDialog::ZEditAddressDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZEditAddressDialog),
    mapper(0),
    mode(mode),
    model(0)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->addressEdit, this);

    switch(mode)
    {
    case NewReceivingAddress:
        setWindowTitle(tr("New receiving address"));
        ui->addressEdit->setEnabled(false);
        break;
    case NewSendingAddress:
        setWindowTitle(tr("New sending address"));
        break;
    case EditReceivingAddress:
        setWindowTitle(tr("Edit receiving address"));
        ui->addressEdit->setEnabled(false);
        break;
    case EditSendingAddress:
        setWindowTitle(tr("Edit sending address"));
        break;
    }

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

ZEditAddressDialog::~ZEditAddressDialog()
{
    delete ui;
}

void ZEditAddressDialog::setModel(ZAddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    mapper->setModel(model);
    mapper->addMapping(ui->addressEdit, ZAddressTableModel::Address);
}

void ZEditAddressDialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);
}

bool ZEditAddressDialog::saveCurrentRow()
{
    if(!model)
        return false;

    switch(mode)
    {
    case NewReceivingAddress:
    case NewSendingAddress:
    case EditReceivingAddress:
    case EditSendingAddress:
        if(mapper->submit())
        {
            address = ui->addressEdit->text();
        }
        break;
    }
    return !address.isEmpty();
}

void ZEditAddressDialog::accept()
{
    if(!model)
        return;

    QDialog::accept();
}

QString ZEditAddressDialog::getAddress() const
{
    return address;
}

void ZEditAddressDialog::setAddress(const QString &address)
{
    this->address = address;
    ui->addressEdit->setText(address);
}
