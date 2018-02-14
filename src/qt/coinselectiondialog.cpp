// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "coinselectiondialog.h"
#include "ui_coinselectiondialog.h"

#include "unspenttablemodel.h"
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

    setWindowTitle(tr("Select input coins"));
}

CoinSelectionDialog::~CoinSelectionDialog()
{
    delete ui;
}

void CoinSelectionDialog::setModel(UnspentTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModelUnspentZ = new QSortFilterProxyModel(this);
    proxyModelUnspentZ->setSourceModel(model);
    proxyModelUnspentZ->setDynamicSortFilter(true);
    proxyModelUnspentZ->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelUnspentZ->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelUnspentT = new QSortFilterProxyModel(this);
    proxyModelUnspentT->setSourceModel(model);
    proxyModelUnspentT->setDynamicSortFilter(true);
    proxyModelUnspentT->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModelUnspentT->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModelUnspentZ->setFilterRole(UnspentTableModel::TypeRole);
    proxyModelUnspentZ->setFilterFixedString(UnspentTableModel::ZUnspent);
    ui->tableViewZ->setModel(proxyModelUnspentZ);
    ui->tableViewZ->sortByColumn(0, Qt::AscendingOrder);

    proxyModelUnspentT->setFilterRole(UnspentTableModel::TypeRole);
    proxyModelUnspentT->setFilterFixedString(UnspentTableModel::TUnspent);
    ui->tableViewT->setModel(proxyModelUnspentT);
    ui->tableViewT->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableViewZ->horizontalHeader()->setResizeMode(UnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableViewZ->horizontalHeader()->setResizeMode(UnspentTableModel::Balance, QHeaderView::ResizeToContents);
    ui->tableViewT->horizontalHeader()->setResizeMode(UnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableViewT->horizontalHeader()->setResizeMode(UnspentTableModel::Balance, QHeaderView::ResizeToContents);
#else
    ui->tableViewZ->horizontalHeader()->setSectionResizeMode(UnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableViewZ->horizontalHeader()->setSectionResizeMode(UnspentTableModel::Balance, QHeaderView::ResizeToContents);
    ui->tableViewT->horizontalHeader()->setSectionResizeMode(UnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableViewT->horizontalHeader()->setSectionResizeMode(UnspentTableModel::Balance, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableViewZ->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionUnspentZChanged()));

    connect(ui->tableViewT->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionUnspentTChanged()));

    selectionUnspentZChanged();
    selectionUnspentTChanged();
}

void CoinSelectionDialog::selectionUnspentZChanged()
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

void CoinSelectionDialog::selectionUnspentTChanged()
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
