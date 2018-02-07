// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "zunspentdialog.h"
#include "ui_zunspentdialog.h"

#include "zunspenttablemodel.h"
#include "bitcoingui.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QSortFilterProxyModel>

ZUnspentDialog::ZUnspentDialog(const PlatformStyle *platformStyle, Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZUnspentDialog),
    model(0),
    mode(mode)
{
    ui->setupUi(this);

    ui->tableZView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    switch(mode)
    {
    case ForSelection:
        ui->closeButton->setText(tr("C&hoose"));
        break;
    case ForEditing:
        ui->closeButton->setText(tr("C&lose"));
        break;
    }

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

ZUnspentDialog::~ZUnspentDialog()
{
    delete ui;
}

void ZUnspentDialog::setModel(ZUnspentTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tableZView->setModel(proxyModel);
    ui->tableZView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableZView->horizontalHeader()->setResizeMode(ZUnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableZView->horizontalHeader()->setResizeMode(ZUnspentTableModel::Balance, QHeaderView::ResizeToContents);
#else
    ui->tableZView->horizontalHeader()->setSectionResizeMode(ZUnspentTableModel::Address, QHeaderView::Stretch);
    ui->tableZView->horizontalHeader()->setSectionResizeMode(ZUnspentTableModel::Balance, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableZView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged()));

    selectionChanged();
}

void ZUnspentDialog::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableZView;

    if(!table->selectionModel())
        return;
}

void ZUnspentDialog::done(int retval)
{
    QTableView *table;
    QModelIndexList indexes;

    table = ui->tableZView;
    if(!table->selectionModel() || !table->model()) {
        return;
    }
    indexes = table->selectionModel()->selectedRows(ZUnspentTableModel::Address);

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
