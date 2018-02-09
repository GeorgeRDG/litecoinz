// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressbookdialog.h"
#include "ui_addressbookdialog.h"

#include "addresstablemodelnew.h"
#include "guiutil.h"
#include "editaddressdialog.h"
#include "platformstyle.h"

#include <QIcon>
#include <QMenu>
#include <QSortFilterProxyModel>

AddressBookDialog::AddressBookDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookDialog),
    model(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons()) {
        ui->newReceivingZAddress->setIcon(QIcon());
        ui->newReceivingTAddress->setIcon(QIcon());
        ui->newSendingTAddress->setIcon(QIcon());

        ui->copyReceivingZAddress->setIcon(QIcon());
        ui->copyReceivingTAddress->setIcon(QIcon());
        ui->copySendingTAddress->setIcon(QIcon());

        ui->exportReceivingZAddress->setIcon(QIcon());
        ui->exportReceivingTAddress->setIcon(QIcon());
        ui->exportSendingTAddress->setIcon(QIcon());

        ui->deleteSendingTAddress->setIcon(QIcon());
    } else {
        ui->newReceivingZAddress->setIcon(QIcon(":/images/res/images/add1.png"));
        ui->newReceivingTAddress->setIcon(QIcon(":/images/res/images/add2.png"));
        ui->newSendingTAddress->setIcon(QIcon(":/images/res/images/add1.png"));

        ui->copyReceivingZAddress->setIcon(QIcon(":/images/res/images/copy.png"));
        ui->copyReceivingTAddress->setIcon(QIcon(":/images/res/images/copy.png"));
        ui->copySendingTAddress->setIcon(QIcon(":/images/res/images/copy.png"));

        ui->exportReceivingZAddress->setIcon(QIcon(":/images/res/images/export.png"));
        ui->exportReceivingTAddress->setIcon(QIcon(":/images/res/images/export.png"));
        ui->exportSendingTAddress->setIcon(QIcon(":/images/res/images/export.png"));

        ui->deleteSendingTAddress->setIcon(QIcon(":/images/res/images/remove1.png"));
    }

    // Context menu actions
    QAction *copyReceivingZAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyReceivingZLabelAction = new QAction(tr("Copy &Label"), this);

    QAction *copyReceivingTAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyReceivingTLabelAction = new QAction(tr("Copy &Label"), this);

    QAction *copySendingTAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copySendingTLabelAction = new QAction(tr("Copy &Label"), this);

    QAction *editReceivingTAction = new QAction(tr("&Edit"), this);
    QAction *editSendingTAction = new QAction(tr("&Edit"), this);

    deleteSendingTAction = new QAction(ui->deleteSendingTAddress->text(), this);

    // Build context menu
    contextReceivingZMenu = new QMenu(this);
    contextReceivingZMenu->addAction(copyReceivingZAddressAction);
    contextReceivingZMenu->addAction(copyReceivingZLabelAction);
    contextReceivingZMenu->addSeparator();

    contextReceivingTMenu = new QMenu(this);
    contextReceivingTMenu->addAction(copyReceivingTAddressAction);
    contextReceivingTMenu->addAction(copyReceivingTLabelAction);
    contextReceivingTMenu->addAction(editReceivingTAction);
    contextReceivingTMenu->addSeparator();

    contextSendingTMenu = new QMenu(this);
    contextSendingTMenu->addAction(copySendingTAddressAction);
    contextSendingTMenu->addAction(copySendingTLabelAction);
    contextSendingTMenu->addAction(editSendingTAction);
    contextSendingTMenu->addAction(deleteSendingTAction);
    contextSendingTMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyReceivingZAddressAction, SIGNAL(triggered()), this, SLOT(on_copyReceivingZAddress_clicked()));
    connect(copyReceivingTAddressAction, SIGNAL(triggered()), this, SLOT(on_copyReceivingTAddress_clicked()));
    connect(copySendingTAddressAction, SIGNAL(triggered()), this, SLOT(on_copySendingTAddress_clicked()));

    connect(copyReceivingZLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelReceivingZAction()));
    connect(copyReceivingTLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelReceivingTAction()));
    connect(copySendingTLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelSendingTAction()));

    connect(editReceivingTAction, SIGNAL(triggered()), this, SLOT(onEditReceivingTAction()));
    connect(editSendingTAction, SIGNAL(triggered()), this, SLOT(onEditSendingTAction()));

    connect(deleteSendingTAction, SIGNAL(triggered()), this, SLOT(on_deleteAddress_clicked()));

    connect(ui->tableViewReceivingZ, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualReceivingZMenu(QPoint)));
    connect(ui->tableViewReceivingT, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualReceivingTMenu(QPoint)));
    connect(ui->tableViewSendingT, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualSendingTMenu(QPoint)));
}

void AddressBookDialog::onCopyLabelReceivingZAction()
{
    GUIUtil::copyEntryData(ui->tableViewReceivingZ, AddressTableNewModel::Label);
}

void AddressBookDialog::onCopyLabelReceivingTAction()
{
    GUIUtil::copyEntryData(ui->tableViewReceivingT, AddressTableNewModel::Label);
}

void AddressBookDialog::onCopyLabelSendingTAction()
{
    GUIUtil::copyEntryData(ui->tableViewSendingT, AddressTableNewModel::Label);
}

void AddressBookDialog::onEditReceivingTAction()
{
    if(!model)
        return;

    if(!ui->tableViewReceivingT->selectionModel())
        return;
    QModelIndexList indexes = ui->tableViewReceivingT->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(EditAddressDialog::EditReceivingAddress, this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModelReceivingT->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookDialog::onEditSendingTAction()
{
    if(!model)
        return;

    if(!ui->tableViewSendingT->selectionModel())
        return;
    QModelIndexList indexes = ui->tableViewSendingT->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(EditAddressDialog::EditSendingAddress, this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModelSendingT->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookDialog::contextualReceivingZMenu(const QPoint &point)
{
    QModelIndex index = ui->tableViewReceivingZ->indexAt(point);
    if(index.isValid())
    {
        contextReceivingZMenu->exec(QCursor::pos());
    }
}

void AddressBookDialog::contextualReceivingTMenu(const QPoint &point)
{
    QModelIndex index = ui->tableViewReceivingT->indexAt(point);
    if(index.isValid())
    {
        contextReceivingTMenu->exec(QCursor::pos());
    }
}

void AddressBookDialog::contextualSendingTMenu(const QPoint &point)
{
    QModelIndex index = ui->tableViewSendingT->indexAt(point);
    if(index.isValid())
    {
        contextSendingTMenu->exec(QCursor::pos());
    }
}

void AddressBookDialog::setModel(AddressTableNewModel *model)
{
    this->model = model;

    if(!model)
        return;

    proxyModelReceivingZ = new QSortFilterProxyModel(this);
    proxyModelReceivingZ->setSourceModel(model);
    proxyModelReceivingZ->setDynamicSortFilter(true);
    proxyModelReceivingZ->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelReceivingZ->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelReceivingT = new QSortFilterProxyModel(this);
    proxyModelReceivingT->setSourceModel(model);
    proxyModelReceivingT->setDynamicSortFilter(true);
    proxyModelReceivingT->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelReceivingT->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelSendingT = new QSortFilterProxyModel(this);
    proxyModelSendingT->setSourceModel(model);
    proxyModelSendingT->setDynamicSortFilter(true);
    proxyModelSendingT->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelSendingT->setFilterCaseSensitivity(Qt::CaseInsensitive);

    // Receive filter (z-addresses)
    proxyModelReceivingZ->setFilterRole(AddressTableNewModel::TypeRole);
    proxyModelReceivingZ->setFilterFixedString(AddressTableNewModel::ZReceive);
    ui->tableViewReceivingZ->setModel(proxyModelReceivingZ);
    ui->tableViewReceivingZ->sortByColumn(0, Qt::AscendingOrder);

    // Receive filter (t-addresses)
    proxyModelReceivingT->setFilterRole(AddressTableNewModel::TypeRole);
    proxyModelReceivingT->setFilterFixedString(AddressTableNewModel::Receive);
    ui->tableViewReceivingT->setModel(proxyModelReceivingT);
    ui->tableViewReceivingT->sortByColumn(0, Qt::AscendingOrder);

    // Send filter
    proxyModelSendingT->setFilterRole(AddressTableNewModel::TypeRole);
    proxyModelSendingT->setFilterFixedString(AddressTableNewModel::Send);
    ui->tableViewSendingT->setModel(proxyModelSendingT);
    ui->tableViewSendingT->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableViewReceivingZ->horizontalHeader()->setResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewReceivingZ->horizontalHeader()->setResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
    ui->tableViewReceivingT->horizontalHeader()->setResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewReceivingT->horizontalHeader()->setResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
    ui->tableViewSendingT->horizontalHeader()->setResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewSendingT->horizontalHeader()->setResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
#else
    ui->tableViewReceivingZ->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewReceivingZ->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
    ui->tableViewReceivingT->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewReceivingT->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
    ui->tableViewSendingT->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Label, QHeaderView::Stretch);
    ui->tableViewSendingT->horizontalHeader()->setSectionResizeMode(AddressTableNewModel::Address, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableViewReceivingZ->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionReceivingZChanged()));
    connect(ui->tableViewReceivingT->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionReceivingTChanged()));
    connect(ui->tableViewSendingT->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionSendingTChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));

    selectionReceivingZChanged();
    selectionReceivingTChanged();
    selectionSendingTChanged();
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
    GUIUtil::copyEntryData(ui->tableViewReceivingZ, AddressTableNewModel::Address);
}

void AddressBookDialog::on_exportReceivingZAddress_clicked()
{

}

void AddressBookDialog::on_newReceivingTAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(EditAddressDialog::NewReceivingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newReceivingTAddressToSelect = dlg.getAddress();
    }
}

void AddressBookDialog::on_copyReceivingTAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableViewReceivingT, AddressTableNewModel::Address);
}

void AddressBookDialog::on_exportReceivingTAddress_clicked()
{

}


// Sending Addresses Tab
void AddressBookDialog::on_newSendingTAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(EditAddressDialog::NewSendingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newSendingTAddressToSelect = dlg.getAddress();
    }
}

void AddressBookDialog::on_copySendingTAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableViewSendingT, AddressTableNewModel::Address);
}

void AddressBookDialog::on_deleteSendingTAddress_clicked()
{
    QTableView *table = ui->tableViewSendingT;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void AddressBookDialog::on_exportSendingTAddress_clicked()
{

}

void AddressBookDialog::selectionReceivingZChanged()
{
    // Set button states based on selection
    QTableView *table = ui->tableViewReceivingZ;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableViewReceivingT->selectionModel()->clear();
        ui->tableViewSendingT->selectionModel()->clear();
        ui->copyReceivingZAddress->setEnabled(true);
    }
    else
    {
        ui->copyReceivingZAddress->setEnabled(false);
    }
}

void AddressBookDialog::selectionReceivingTChanged()
{
    // Set button states based on selection
    QTableView *table = ui->tableViewReceivingT;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableViewReceivingZ->selectionModel()->clear();
        ui->tableViewSendingT->selectionModel()->clear();
        ui->copyReceivingTAddress->setEnabled(true);
    }
    else
    {
        ui->copyReceivingTAddress->setEnabled(false);
    }
}

void AddressBookDialog::selectionSendingTChanged()
{
    // Set button states based on selection
    QTableView *table = ui->tableViewSendingT;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableViewReceivingZ->selectionModel()->clear();
        ui->tableViewReceivingT->selectionModel()->clear();
        ui->copySendingTAddress->setEnabled(true);
        ui->deleteSendingTAddress->setEnabled(true);
    }
    else
    {
        ui->copySendingTAddress->setEnabled(false);
        ui->deleteSendingTAddress->setEnabled(false);
    }
}
