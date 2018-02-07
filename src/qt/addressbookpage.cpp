// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "addressbookpage.h"
#include "ui_addressbookpage.h"

#include "addresstablemodel.h"
#include "zaddresstablemodel.h"
#include "bitcoingui.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>

AddressBookPage::AddressBookPage(const PlatformStyle *platformStyle, Mode mode, Tabs tab, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookPage),
    model(0),
    mode(mode),
    tab(tab)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons()) {
        ui->newAddress->setIcon(QIcon());
        ui->newZAddress->setIcon(QIcon());
        ui->copyAddress->setIcon(QIcon());
        ui->deleteAddress->setIcon(QIcon());
        ui->exportButton->setIcon(QIcon());
    } else {
        ui->newAddress->setIcon(platformStyle->SingleColorIcon(":/icons/add"));
        ui->newZAddress->setIcon(platformStyle->SingleColorIcon(":/icons/add"));
        ui->copyAddress->setIcon(platformStyle->SingleColorIcon(":/icons/editcopy"));
        ui->deleteAddress->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
        ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }

    switch(mode)
    {
    case ForTSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the transparent address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the transparent address to receive coins with")); break;
        }
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setFocus();
        ui->closeButton->setText(tr("C&hoose"));
        ui->exportButton->hide();
        ui->tableZView->setVisible(false);
        ui->labelZExplanation->setVisible(false);
        resize(sizeHint());
        break;
    case ForZSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the private address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the private address to receive coins with")); break;
        }
        connect(ui->tableZView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableZView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableZView->setFocus();
        ui->closeButton->setText(tr("C&hoose"));
        ui->exportButton->hide();
        ui->tableView->setVisible(false);
        ui->newAddress->setVisible(false);
        ui->copyAddress->setVisible(false);
        ui->labelExplanation->setVisible(false);
        resize(sizeHint());
        break;
    case ForEditing:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Sending addresses")); break;
        case ReceivingTab: setWindowTitle(tr("Receiving addresses")); break;
        }
        break;
    }
    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->setText(tr("These are your LitecoinZ addresses for sending payments. Always check the amount and the receiving address before sending coins."));
        ui->deleteAddress->setVisible(true);
        ui->newZAddress->setVisible(false);
        ui->tableZView->setVisible(false);
        resize(sizeHint());
        break;
    case ReceivingTab:
        ui->labelExplanation->setText(tr("These are your LitecoinZ transparent addresses for receiving payments. It is recommended to use a new receiving address for each transaction."));
        ui->labelZExplanation->setText(tr("These are your LitecoinZ private addresses for shielding coins."));
        ui->deleteAddress->setVisible(false);
        break;
    }

    // Context menu actions
    QAction *copyAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy &Label"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    deleteAction = new QAction(ui->deleteAddress->text(), this);

    // Build context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(editAction);
    if(tab == SendingTab)
        contextMenu->addAction(deleteAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(on_copyAddress_clicked()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(on_deleteAddress_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));

}

AddressBookPage::~AddressBookPage()
{
    delete ui;
}

void AddressBookPage::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
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

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));

    selectionChanged();
}

void AddressBookPage::setZModel(ZAddressTableModel *zmodel)
{
    this->zmodel = zmodel;
    if(!zmodel)
        return;

    proxyZModel = new QSortFilterProxyModel(this);
    proxyZModel->setSourceModel(zmodel);
    proxyZModel->setDynamicSortFilter(true);
    proxyZModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyZModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->tableZView->setModel(proxyZModel);
    ui->tableZView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableZView->horizontalHeader()->setResizeMode(ZAddressTableModel::Address, QHeaderView::Stretch);
#else
    ui->tableZView->horizontalHeader()->setSectionResizeMode(ZAddressTableModel::Address, QHeaderView::Stretch);
#endif

    connect(ui->tableZView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionZChanged()));

    // Select row for newly created z-address
    connect(zmodel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewZAddress(QModelIndex,int,int)));

    selectionZChanged();
}

void AddressBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void AddressBookPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void AddressBookPage::onEditAction()
{
    if(!model)
        return;

    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::EditSendingAddress :
        EditAddressDialog::EditReceivingAddress, this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookPage::on_newAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::NewSendingAddress :
        EditAddressDialog::NewReceivingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}

void AddressBookPage::on_deleteAddress_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void AddressBookPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableZView->selectionModel()->clear();

        switch(tab)
        {
        case SendingTab:
            // In sending tab, allow deletion of selection
            ui->deleteAddress->setEnabled(true);
            ui->deleteAddress->setVisible(true);
            deleteAction->setEnabled(true);
            break;
        case ReceivingTab:
            // Deleting receiving addresses, however, is not allowed
            ui->deleteAddress->setEnabled(false);
            ui->deleteAddress->setVisible(false);
            deleteAction->setEnabled(false);
            break;
        }
        ui->copyAddress->setEnabled(true);
    }
    else
    {
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }
}

void AddressBookPage::selectionZChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableZView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableView->selectionModel()->clear();
    }
}

void AddressBookPage::done(int retval)
{
    QTableView *table;
    QModelIndexList indexes;

    switch(mode)
    {
    case ForTSelection:
        table = ui->tableView;
        if(!table->selectionModel() || !table->model()) {
            return;
        }
        indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);
        break;
    case ForZSelection:
        table = ui->tableZView;
        if(!table->selectionModel() || !table->model()) {
            return;
        }
        indexes = table->selectionModel()->selectedRows(ZAddressTableModel::Address);
        break;
    }

    // Figure out which address was selected, and return it
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

void AddressBookPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Address List"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if(!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"),
            tr("There was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}

void AddressBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void AddressBookPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect))
    {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}

void AddressBookPage::selectNewZAddress(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyZModel->mapFromSource(model->index(begin, ZAddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newZAddressToSelect))
    {
        // Select row of newly created address, once
        ui->tableZView->setFocus();
        ui->tableZView->selectRow(idx.row());
        newZAddressToSelect.clear();
    }
}
