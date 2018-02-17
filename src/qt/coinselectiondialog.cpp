// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinselectiondialog.h"
#include "ui_coinselectiondialog.h"

#include "unspenttablemodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "txmempool.h"
#include "walletmodel.h"

#include "coincontrol.h"
#include "init.h"
#include "main.h" // For minRelayTxFee
#include "wallet/wallet.h"

#include <boost/assign/list_of.hpp> // for 'map_list_of()'

#include "wallet/asyncrpcoperation_sendmany.h"

#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QSettings>
#include <QString>
#include <QSortFilterProxyModel>

QList<CAmount> CoinSelectionDialog::payAmounts;
CCoinControl* CoinSelectionDialog::coinSelection = new CCoinControl();
bool CoinSelectionDialog::fSubtractFeeFromAmount = false;

CoinSelectionDialog::CoinSelectionDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinSelectionDialog),
    model(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    ui->tableViewZ->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewT->setEditTriggers(QAbstractItemView::NoEditTriggers);

    setWindowTitle(tr("Select input coins"));

    // clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(clipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(clipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(clipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(clipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(clipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(clipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(clipboardChange()));

    ui->labelCoinSelectionQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinSelectionAmount->addAction(clipboardAmountAction);
    ui->labelCoinSelectionFee->addAction(clipboardFeeAction);
    ui->labelCoinSelectionAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinSelectionBytes->addAction(clipboardBytesAction);
    ui->labelCoinSelectionPriority->addAction(clipboardPriorityAction);
    ui->labelCoinSelectionLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinSelectionChange->addAction(clipboardChangeAction);

    // ok button
    connect(ui->buttonBox, SIGNAL(clicked( QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

    connect(ui->tableViewZ->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionUnspentZChanged()));
    connect(ui->tableViewT->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionUnspentTChanged()));
}

CoinSelectionDialog::~CoinSelectionDialog()
{
    delete ui;
}

void CoinSelectionDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(!model)
        return;

    if(model && model->getOptionsModel() && model->getUnspentTableModel())
    {
        CoinSelectionDialog::updateLabels(model, this);
    }
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

//    if(model && walletModel->getOptionsModel() && walletModel->getUnspentTableModel())
//        CoinSelectionDialog::updateLabels(walletModel, this);
}

// ok button
void CoinSelectionDialog::buttonBoxClicked(QAbstractButton* button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        done(QDialog::Accepted); // closes the dialog
}

// copy label "Quantity" to clipboard
void CoinSelectionDialog::clipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionQuantity->text());
}

// copy label "Amount" to clipboard
void CoinSelectionDialog::clipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionAmount->text().left(ui->labelCoinSelectionAmount->text().indexOf(" ")));
}

// copy label "Fee" to clipboard
void CoinSelectionDialog::clipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionFee->text().left(ui->labelCoinSelectionFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// copy label "After fee" to clipboard
void CoinSelectionDialog::clipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionAfterFee->text().left(ui->labelCoinSelectionAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// copy label "Bytes" to clipboard
void CoinSelectionDialog::clipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionBytes->text().replace(ASYMP_UTF8, ""));
}

// copy label "Priority" to clipboard
void CoinSelectionDialog::clipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionPriority->text());
}

// copy label "Dust" to clipboard
void CoinSelectionDialog::clipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionLowOutput->text());
}

// copy label "Change" to clipboard
void CoinSelectionDialog::clipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionChange->text().left(ui->labelCoinSelectionChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
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
        
        // Figure out which address was selected, and return its balance
        QModelIndexList indexes = table->selectionModel()->selectedRows(UnspentTableModel::Balance);

        for (const QModelIndex& index : indexes) {
            QVariant balance = table->model()->data(index);
        }
        CoinSelectionDialog::updateLabels(walletModel, this);
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
        
        // Figure out which address was selected, and return its balance
        QModelIndexList indexes = table->selectionModel()->selectedRows(UnspentTableModel::Balance);

        for (const QModelIndex& index : indexes) {
            QVariant balance = table->model()->data(index);
        }
        CoinSelectionDialog::updateLabels(walletModel, this);
    }
}
/*
// checkbox clicked by user
void CoinSelectionDialog::viewItemChanged(QTreeWidgetItem* item, int column)
{
    if (column == COLUMN_CHECKBOX && item->text(COLUMN_TXHASH).length() == 64) // transaction hash is 64 characters (this means its a child node, so its not a parent node in tree mode)
    {
        COutPoint outpt(uint256S(item->text(COLUMN_TXHASH).toStdString()), item->text(COLUMN_VOUT_INDEX).toUInt());

        if (item->checkState(COLUMN_CHECKBOX) == Qt::Unchecked)
            coinSelection->UnSelect(outpt);
        else if (item->isDisabled()) // locked (this happens if "check all" through parent node)
            item->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        else
            coinSelection->Select(outpt);

        // selection changed -> update labels
        if (ui->treeWidget->isEnabled()) // do not update on every click for (un)select all
            CoinSelectionDialog::updateLabels(model, this);
    }

    // TODO: Remove this temporary qt5 fix after Qt5.3 and Qt5.4 are no longer used.
    //       Fixed in Qt5.5 and above: https://bugreports.qt.io/browse/QTBUG-43473
#if QT_VERSION >= 0x050000
    else if (column == COLUMN_CHECKBOX && item->childCount() > 0)
    {
        if (item->checkState(COLUMN_CHECKBOX) == Qt::PartiallyChecked && item->child(0)->checkState(COLUMN_CHECKBOX) == Qt::PartiallyChecked)
            item->setCheckState(COLUMN_CHECKBOX, Qt::Checked);
    }
#endif
}
*/

// return human readable label for priority number
QString CoinSelectionDialog::getPriorityLabel(double dPriority, double mempoolEstimatePriority)
{
    double dPriorityMedium = mempoolEstimatePriority;

    if (dPriorityMedium <= 0)
        dPriorityMedium = AllowFreeThreshold(); // not enough data, back to hard-coded

    if      (dPriority / 1000000 > dPriorityMedium) return tr("highest");
    else if (dPriority / 100000 > dPriorityMedium)  return tr("higher");
    else if (dPriority / 10000 > dPriorityMedium)   return tr("high");
    else if (dPriority / 1000 > dPriorityMedium)    return tr("medium-high");
    else if (dPriority > dPriorityMedium)           return tr("medium");
    else if (dPriority * 10 > dPriorityMedium)      return tr("low-medium");
    else if (dPriority * 100 > dPriorityMedium)     return tr("low");
    else if (dPriority * 1000 > dPriorityMedium)    return tr("lower");
    else                                            return tr("lowest");
}

void CoinSelectionDialog::updateLabels(WalletModel *model, QDialog* dialog)
{
    if (!model)
        return;

    // nPayAmount
    CAmount nPayAmount = 0;
    bool fDust = false;
    CMutableTransaction txDummy;
    Q_FOREACH(const CAmount &amount, CoinSelectionDialog::payAmounts)
    {
        nPayAmount += amount;

        if (amount > 0)
        {
            CTxOut txout(amount, (CScript)std::vector<unsigned char>(24, 0));
            txDummy.vout.push_back(txout);
            if (txout.IsDust(::minRelayTxFee))
               fDust = true;
        }
    }

    QString sPriorityLabel      = tr("none");
    CAmount nAmount             = 0;
    CAmount nPayFee             = 0;
    CAmount nAfterFee           = 0;
    CAmount nChange             = 0;
    unsigned int nBytes         = 0;
    unsigned int nBytesInputs   = 0;
    double dPriority            = 0;
    double dPriorityInputs      = 0;
    unsigned int nQuantity      = 0;
    int nQuantityUncompressed   = 0;
    bool fAllowFree             = false;
    bool fWitness               = false;

    std::vector<COutPoint> vCoinSelection;
    std::vector<COutput>   vOutputs;
    coinSelection->ListSelected(vCoinSelection);
    model->getOutputs(vCoinSelection, vOutputs);

    BOOST_FOREACH(const COutput& out, vOutputs) {
        // unselect already spent, very unlikely scenario, this could happen
        // when selected are spent elsewhere, like rpc or another computer
        uint256 txhash = out.tx->GetHash();
        COutPoint outpt(txhash, out.i);
        if (model->isSpent(outpt))
        {
            coinSelection->UnSelect(outpt);
            continue;
        }

        // Quantity
        nQuantity++;

        // Amount
        nAmount += out.tx->vout[out.i].nValue;

        // Priority
        dPriorityInputs += (double)out.tx->vout[out.i].nValue * (out.nDepth+1);

        // Bytes
        CTxDestination address;
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;
        if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
        {
            CPubKey pubkey;
            CKeyID *keyid = boost::get<CKeyID>(&address);
            if (keyid && model->getPubKey(*keyid, pubkey))
            {
                nBytesInputs += (pubkey.IsCompressed() ? 148 : 180);
                if (!pubkey.IsCompressed())
                    nQuantityUncompressed++;
            }
            else
                nBytesInputs += 148; // in all error cases, simply assume 148 here
        }
        else nBytesInputs += 148;
    }

    // calculation
    if (nQuantity > 0)
    {
        // Bytes
        nBytes = nBytesInputs + ((CoinSelectionDialog::payAmounts.size() > 0 ? CoinSelectionDialog::payAmounts.size() + 1 : 2) * 34) + 10; // always assume +1 output for change here
        if (fWitness)
        {
            // there is some fudging in these numbers related to the actual virtual transaction size calculation that will keep this estimate from being exact.
            // usually, the result will be an overestimate within a couple of satoshis so that the confirmation dialog ends up displaying a slightly smaller fee.
            // also, the witness stack size value value is a variable sized integer. usually, the number of stack items will be well under the single byte var int limit.
            nBytes += 2; // account for the serialized marker and flag bytes
            nBytes += nQuantity; // account for the witness byte that holds the number of stack items for each input.
        }

        // Priority
        double mempoolEstimatePriority = mempool.estimatePriority(nTxConfirmTarget);
        dPriority = dPriorityInputs / (nBytes - nBytesInputs + (nQuantityUncompressed * 29)); // 29 = 180 - 151 (uncompressed public keys are over the limit. max 151 bytes of the input are ignored for priority)
        sPriorityLabel = CoinSelectionDialog::getPriorityLabel(dPriority, mempoolEstimatePriority);

        // in the subtract fee from amount case, we can tell if zero change already and subtract the bytes, so that fee calculation afterwards is accurate
        if (CoinSelectionDialog::fSubtractFeeFromAmount)
            if (nAmount - nPayAmount == 0)
                nBytes -= 34;

        // Fee
        nPayFee = CWallet::GetMinimumFee(nBytes, nTxConfirmTarget, mempool);
        if (nPayFee > 0 && coinSelection->nMinimumTotalFee > nPayFee)
            nPayFee = coinSelection->nMinimumTotalFee;


        // Allow free? (require at least hard-coded threshold and default to that if no estimate)
        double dPriorityNeeded = std::max(mempoolEstimatePriority, AllowFreeThreshold());
        fAllowFree = (dPriority >= dPriorityNeeded);

        if (fSendFreeTransactions)
            if (fAllowFree && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                nPayFee = 0;

        if (nPayAmount > 0)
        {
            nChange = nAmount - nPayAmount;
            if (!CoinSelectionDialog::fSubtractFeeFromAmount)
                nChange -= nPayFee;

            // Never create dust outputs; if we would, just add the dust to the fee.
            if (nChange > 0 && nChange < MIN_CHANGE)
            {
                CTxOut txout(nChange, (CScript)std::vector<unsigned char>(24, 0));
                if (txout.IsDust(::minRelayTxFee))
                {
                    if (CoinSelectionDialog::fSubtractFeeFromAmount) // dust-change will be raised until no dust
                        nChange = txout.GetDustThreshold(::minRelayTxFee);
                    else
                    {
                        nPayFee += nChange;
                        nChange = 0;
                    }
                }
            }

            if (nChange == 0 && !CoinSelectionDialog::fSubtractFeeFromAmount)
                nBytes -= 34;
        }

        // after fee
        nAfterFee = nAmount - nPayFee;
        if (nAfterFee < 0)
            nAfterFee = 0;
    }

    // actually update labels
    int nDisplayUnit = BitcoinUnits::LTZ;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    QLabel *l1 = dialog->findChild<QLabel *>("labelCoinSelectionQuantity");
    QLabel *l2 = dialog->findChild<QLabel *>("labelCoinSelectionAmount");
    QLabel *l3 = dialog->findChild<QLabel *>("labelCoinSelectionFee");
    QLabel *l4 = dialog->findChild<QLabel *>("labelCoinSelectionAfterFee");
    QLabel *l5 = dialog->findChild<QLabel *>("labelCoinSelectionBytes");
    QLabel *l6 = dialog->findChild<QLabel *>("labelCoinSelectionPriority");
    QLabel *l7 = dialog->findChild<QLabel *>("labelCoinSelectionLowOutput");
    QLabel *l8 = dialog->findChild<QLabel *>("labelCoinSelectionChange");

    // enable/disable "dust" and "change"
    dialog->findChild<QLabel *>("labelCoinSelectionLowOutputText")->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinSelectionLowOutput")    ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinSelectionChangeText")   ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinSelectionChange")       ->setEnabled(nPayAmount > 0);

    // stats
    l1->setText(QString::number(nQuantity));                                 // Quantity
    l2->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));        // Amount
    l3->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee));        // Fee
    l4->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee));      // After Fee
    l5->setText(((nBytes > 0) ? ASYMP_UTF8 : "") + QString::number(nBytes));        // Bytes
    l6->setText(sPriorityLabel);                                             // Priority
    l7->setText(fDust ? tr("yes") : tr("no"));                               // Dust
    l8->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nChange));        // Change
    if (nPayFee > 0 && (coinSelection->nMinimumTotalFee < nPayFee))
    {
        l3->setText(ASYMP_UTF8 + l3->text());
        l4->setText(ASYMP_UTF8 + l4->text());
        if (nChange > 0 && !CoinSelectionDialog::fSubtractFeeFromAmount)
            l8->setText(ASYMP_UTF8 + l8->text());
    }

    // turn labels "red"
    l5->setStyleSheet((nBytes >= MAX_FREE_TRANSACTION_CREATE_SIZE) ? "color:red;" : "");// Bytes >= 1000
    l6->setStyleSheet((dPriority > 0 && !fAllowFree) ? "color:red;" : "");              // Priority < "medium"
    l7->setStyleSheet((fDust) ? "color:red;" : "");                                     // Dust = "yes"

    // tool tips
    QString toolTip1 = tr("This label turns red if the transaction size is greater than 1000 bytes.") + "<br /><br />";
    toolTip1 += tr("This means a fee of at least %1 per kB is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CWallet::minTxFee.GetFeePerK())) + "<br /><br />";
    toolTip1 += tr("Can vary +/- 1 byte per input.");

    QString toolTip2 = tr("Transactions with higher priority are more likely to get included into a block.") + "<br /><br />";
    toolTip2 += tr("This label turns red if the priority is smaller than \"medium\".") + "<br /><br />";
    toolTip2 += tr("This means a fee of at least %1 per kB is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CWallet::minTxFee.GetFeePerK()));

    QString toolTip3 = tr("This label turns red if any recipient receives an amount smaller than the current dust threshold.");

    // how many satoshis the estimated fee can vary per byte we guess wrong
    double dFeeVary;
    if (payTxFee.GetFeePerK() > 0)
        dFeeVary = (double)std::max(CWallet::minTxFee.GetFeePerK(), payTxFee.GetFeePerK()) / 1000;
    else {
        dFeeVary = (double)std::max(CWallet::minTxFee.GetFeePerK(), mempool.estimateFee(nTxConfirmTarget).GetFeePerK()) / 1000;
    }
    QString toolTip4 = tr("Can vary +/- %1 satoshi(s) per input.").arg(dFeeVary);

    l3->setToolTip(toolTip4);
    l4->setToolTip(toolTip4);
    l5->setToolTip(toolTip1);
    l6->setToolTip(toolTip2);
    l7->setToolTip(toolTip3);
    l8->setToolTip(toolTip4);
    dialog->findChild<QLabel *>("labelCoinSelectionFeeText")      ->setToolTip(l3->toolTip());
    dialog->findChild<QLabel *>("labelCoinSelectionAfterFeeText") ->setToolTip(l4->toolTip());
    dialog->findChild<QLabel *>("labelCoinSelectionBytesText")    ->setToolTip(l5->toolTip());
    dialog->findChild<QLabel *>("labelCoinSelectionPriorityText") ->setToolTip(l6->toolTip());
    dialog->findChild<QLabel *>("labelCoinSelectionLowOutputText")->setToolTip(l7->toolTip());
    dialog->findChild<QLabel *>("labelCoinSelectionChangeText")   ->setToolTip(l8->toolTip());

    // Insufficient funds
    QLabel *label = dialog->findChild<QLabel *>("labelCoinSelectionInsuffFunds");
    if (label)
        label->setVisible(nChange < 0);
}
