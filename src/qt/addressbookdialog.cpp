// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "addressbookdialog.h"
#include "ui_addressbookdialog.h"

#include "addresstablemodel.h"
#include "bitcoingui.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>

AddressBookDialog::AddressBookDialog(const PlatformStyle *platformStyle, Mode mode, Tabs tab, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookDialog),
    model(0),
    mode(mode),
    tab(tab)
{
    ui->setupUi(this);

    connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setFocus();

    switch(mode)
    {
    case ForTSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the address to receive coins with")); break;
        }
        break;
    case ForZSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Sending z-addresses")); break;
        case ReceivingTab: setWindowTitle(tr("Receiving z-addresses")); break;
        }
        break;
    }

    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->setText(tr("These are your Bitcoin addresses for sending payments. Always check the amount and the receiving address before sending coins."));
        break;
    case ReceivingTab:
        ui->labelExplanation->setText(tr("These are your Bitcoin addresses for receiving payments. It is recommended to use a new receiving address for each transaction."));
        break;
    }

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

AddressBookDialog::~AddressBookDialog()
{
    delete ui;
}

void AddressBookDialog::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    switch(mode)
    {
    case ForTSelection:
        switch(tab)
        {
        case ReceivingTab:
            // Receive filter
            proxyModel->setFilterRole(AddressTableModel::TypeRole);
            proxyModel->setFilterFixedString(AddressTableModel::Receive);
            break;
        case SendingTab:
            // Send filter
            proxyModel->setFilterRole(AddressTableModel::TypeRole);
            proxyModel->setFilterFixedString(AddressTableModel::Send);
            break;
        }
        break;
    case ForZSelection:
        switch(tab)
        {
        case SendingTab:
            // TODO
            break;
        case ReceivingTab:
            // Receive filter (z-addresses)
            proxyModel->setFilterRole(AddressTableModel::TypeRole);
            proxyModel->setFilterFixedString(AddressTableModel::ZReceive);
            break;
        }
        break;
    }

    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#endif
}

void AddressBookDialog::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    Q_FOREACH (const QModelIndex& index, indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}
