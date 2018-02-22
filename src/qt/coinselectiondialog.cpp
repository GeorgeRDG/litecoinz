// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "coinselectiondialog.h"
#include "ui_coinselectiondialog.h"

#include "coinselectiontablemodel.h"
#include "bitcoingui.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QSortFilterProxyModel>


CoinSelectionDialog::CoinSelectionDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinSelectionDialog),
    model(0)
{
    ui->setupUi(this);

    ui->tableViewZ->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewT->setEditTriggers(QAbstractItemView::NoEditTriggers);

    setWindowTitle(tr("CoinSelection transactions list"));

    connect(ui->tableViewT, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
    connect(ui->tableViewZ, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

CoinSelectionDialog::~CoinSelectionDialog()
{
    delete ui;
}

void CoinSelectionDialog::setModel(CoinSelectionTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModelCoinSelectionZ = new QSortFilterProxyModel(this);
    proxyModelCoinSelectionZ->setSourceModel(model);
    proxyModelCoinSelectionZ->setDynamicSortFilter(true);
    proxyModelCoinSelectionZ->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelCoinSelectionZ->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelCoinSelectionT = new QSortFilterProxyModel(this);
    proxyModelCoinSelectionT->setSourceModel(model);
    proxyModelCoinSelectionT->setDynamicSortFilter(true);
    proxyModelCoinSelectionT->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelCoinSelectionT->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelCoinSelectionZ->setFilterRole(CoinSelectionTableModel::TypeRole);
    proxyModelCoinSelectionZ->setFilterFixedString(CoinSelectionTableModel::ZCoinSelection);
    ui->tableViewZ->setModel(proxyModelCoinSelectionZ);
    ui->tableViewZ->sortByColumn(0, Qt::AscendingOrder);

    proxyModelCoinSelectionT->setFilterRole(CoinSelectionTableModel::TypeRole);
    proxyModelCoinSelectionT->setFilterFixedString(CoinSelectionTableModel::TCoinSelection);
    ui->tableViewT->setModel(proxyModelCoinSelectionT);
    ui->tableViewT->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableViewZ->horizontalHeader()->setResizeMode(CoinSelectionTableModel::Address, QHeaderView::Stretch);
    ui->tableViewZ->horizontalHeader()->setResizeMode(CoinSelectionTableModel::Balance, QHeaderView::ResizeToContents);
    ui->tableViewT->horizontalHeader()->setResizeMode(CoinSelectionTableModel::Address, QHeaderView::Stretch);
    ui->tableViewT->horizontalHeader()->setResizeMode(CoinSelectionTableModel::Balance, QHeaderView::ResizeToContents);
#else
    ui->tableViewZ->horizontalHeader()->setSectionResizeMode(CoinSelectionTableModel::Address, QHeaderView::Stretch);
    ui->tableViewZ->horizontalHeader()->setSectionResizeMode(CoinSelectionTableModel::Balance, QHeaderView::ResizeToContents);
    ui->tableViewT->horizontalHeader()->setSectionResizeMode(CoinSelectionTableModel::Address, QHeaderView::Stretch);
    ui->tableViewT->horizontalHeader()->setSectionResizeMode(CoinSelectionTableModel::Balance, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableViewZ->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionCoinSelectionZChanged()));

    connect(ui->tableViewT->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionCoinSelectionTChanged()));

    selectionCoinSelectionZChanged();
    selectionCoinSelectionTChanged();
}

void CoinSelectionDialog::selectionCoinSelectionZChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableViewZ;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableViewT->selectionModel()->clear();
    }
}

void CoinSelectionDialog::selectionCoinSelectionTChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableViewT;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->tableViewZ->selectionModel()->clear();
    }
}

void CoinSelectionDialog::done(int retval)
{
    QTableView *tableT = ui->tableViewT;
    QTableView *tableZ = ui->tableViewZ;

    if((!tableT->selectionModel() || !tableT->model()) && (!tableZ->selectionModel() || !tableZ->model()))
        return;

    if(tableT->selectionModel()->hasSelection())
    {
        QModelIndex index = tableT->currentIndex();
        int rownumber = index.row();
        QModelIndex index1 = index.sibling(rownumber, 0);
        QModelIndex index2 = index.sibling(rownumber, 1);

        QVariant address = index1.data();
        returnAddress = address.toString();
        QVariant balance = index2.data();
        returnAmount = balance.toString();
    }

    if(tableZ->selectionModel()->hasSelection())
    {
        QModelIndex index = tableZ->currentIndex();
        int rownumber = index.row();
        QModelIndex index1 = index.sibling(rownumber, 0);
        QModelIndex index2 = index.sibling(rownumber, 1);

        QVariant address = index1.data();
        returnAddress = address.toString();
        QVariant balance = index2.data();
        returnAmount = balance.toString();
    }

    if(returnAddress.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}
