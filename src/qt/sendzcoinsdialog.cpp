// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendzcoinsdialog.h"
#include "ui_sendzcoinsdialog.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "coinselectiondialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "sendzcoinsentry.h"
#include "walletmodel.h"

#include "base58.h"
#include "coincontrol.h"
#include "main.h" // mempool and minRelayTxFee
#include "ui_interface.h"
#include "txmempool.h"
#include "wallet/wallet.h"

#include <univalue.h>
#include "rpc/server.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>

#define SEND_CONFIRM_DELAY   3
#define ASYMP_UTF8 "\xE2\x89\x88"
#define ASYNC_RPC_OPERATION_DEFAULT_MINERS_FEE   1000

QList<CAmount> SendZCoinsDialog::payAmounts;
bool SendZCoinsDialog::fSubtractFeeFromAmount = false;

SendZCoinsDialog::SendZCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendZCoinsDialog),
    model(0),
    fNewRecipientAllowed(true),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons()) {
        ui->addButton->setIcon(QIcon());
        ui->clearButton->setIcon(QIcon());
        ui->sendButton->setIcon(QIcon());
    } else {
        ui->addButton->setIcon(QIcon(":/images/add3"));
        ui->clearButton->setIcon(QIcon(":/images/clear"));
        ui->sendButton->setIcon(QIcon(":/images/send2"));
    }

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonCoinSelection, SIGNAL(clicked()), this, SLOT(coinSelectionButtonClicked()));

    // Coin Control: clipboard actions
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));

    ui->labelCoinSelectionAmount->addAction(clipboardAmountAction);
    ui->labelCoinSelectionFee->addAction(clipboardFeeAction);
    ui->labelCoinSelectionAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinSelectionLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinSelectionChange->addAction(clipboardChangeAction);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
}

void SendZCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendZCoinsEntry *entry = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(),
                   model->getTBalance(), model->getZBalance(false), model->getUnshielded());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        coinControlUpdateLabels();

        // fee section
        connect(ui->checkBoxCustomFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        ui->customFee->setSingleStep(CWallet::minTxFee.GetFeePerK());
        updateFeeSectionControls();
        updateGlobalFeeVariables();
    }
}

SendZCoinsDialog::~SendZCoinsDialog()
{
    QSettings settings;
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());

    delete ui;
}

void SendZCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendZCoinsEntry *entry = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    fNewRecipientAllowed = false;

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    Q_FOREACH(const SendCoinsRecipient &rcp, currentTransaction.getRecipients())
    {
        // generate bold amount string
        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    Q_FOREACH(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1")
        .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    SendZConfirmationDialog confirmationDialog(tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendZCoinsReturn sendZStatus = model->sendZCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
//marco    processSendCoinsReturn(sendZStatus);

    if (sendZStatus.status == WalletModel::OK)
    {
        QString opid = sendZStatus.opid;
        ui->labelCoinSelectionFeatures->setText(opid);
        accept();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void SendZCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void SendZCoinsDialog::reject()
{
    clear();
}

void SendZCoinsDialog::accept()
{
    clear();
}

SendZCoinsEntry *SendZCoinsDialog::addEntry()
{
    SendZCoinsEntry *entry = new SendZCoinsEntry(platformStyle, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendZCoinsEntry*)), this, SLOT(removeEntry(SendZCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));
    connect(entry, SIGNAL(subtractFeeFromAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    updateTabsAndLabels();
    return entry;
}

void SendZCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendZCoinsDialog::removeEntry(SendZCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *SendZCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendZCoinsEntry *entry = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void SendZCoinsDialog::setAddress(const QString &address)
{
    SendZCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendZCoinsEntry *first = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void SendZCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendZCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendZCoinsEntry *first = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool SendZCoinsDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void SendZCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                  const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance,
                                  const CAmount& t_balance, const CAmount& z_balance, const CAmount& unshielded)
{
    Q_UNUSED(balance);
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), (t_balance + z_balance - unshielded)));
    }
}

void SendZCoinsDialog::updateDisplayUnit()
{
    setBalance(0, 0, 0, 0, 0, 0, model->getTBalance(), model->getZBalance(false), model->getUnshielded());
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
}

void SendZCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendZCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid. Please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
        break;
    case WalletModel::PaymentRequestExpired:
        msgParams.first = tr("Payment request expired.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void SendZCoinsDialog::updateFeeSectionControls()
{
    ui->customFee->setEnabled(ui->checkBoxCustomFee->isChecked());
}

void SendZCoinsDialog::updateGlobalFeeVariables()
{
    nTxConfirmTarget = defaultZConfirmTarget;
    payTxFee = CFeeRate(ui->customFee->value());

    // if user has selected to set a minimum absolute fee, pass the value to coincontrol
    // set nMinimumTotalFee to 0 in case of user has selected that the fee is per KB
    nMinimumTotalFee = ui->checkBoxCustomFee->isChecked() ? ui->customFee->value() : 0;
}

// Coin Control: copy label "Amount" to clipboard
void SendZCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionAmount->text().left(ui->labelCoinSelectionAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendZCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionFee->text().left(ui->labelCoinSelectionFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void SendZCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionAfterFee->text().left(ui->labelCoinSelectionAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// copy label "Dust" to clipboard
void SendZCoinsDialog::clipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendZCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinSelectionChange->text().left(ui->labelCoinSelectionChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: button inputs -> show actual coin selection dialog
void SendZCoinsDialog::coinSelectionButtonClicked()
{
    CoinSelectionDialog dlg(platformStyle);
    dlg.setModel(model->getCoinSelectionTableModel());
    if(dlg.exec())
    {
        inputAddress = dlg.getReturnAddress();
        inputAmount = AmountFromValue(dlg.getReturnAmount().toStdString());
        ui->coinSelectionText->setText(inputAddress);
        coinControlUpdateLabels();
    }
}

// Coin Control: update labels
void SendZCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    // only enable the feature if inputs are selected
    ui->checkBoxCustomFee->setEnabled(!ui->coinSelectionText->text().isEmpty());

    // set pay amounts
    SendZCoinsDialog::payAmounts.clear();
    SendZCoinsDialog::fSubtractFeeFromAmount = false;
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendZCoinsEntry *entry = qobject_cast<SendZCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry && !entry->isHidden())
        {
            SendCoinsRecipient rcp = entry->getValue();
            SendZCoinsDialog::payAmounts.append(rcp.amount);
            if (rcp.fSubtractFeeFromAmount)
                SendZCoinsDialog::fSubtractFeeFromAmount = true;
        }
    }

    if (!ui->coinSelectionText->text().isEmpty())
        // actual coin control calculation
        SendZCoinsDialog::updateLabels();
    else
        ui->labelCoinSelectionInsuffFunds->hide();
}

void SendZCoinsDialog::updateLabels()
{
    // nPayAmount
    CAmount nPayAmount = 0;
    bool fDust = false;
    CMutableTransaction txDummy;

    Q_FOREACH(const CAmount &amount, SendZCoinsDialog::payAmounts)
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

    CAmount nAmount             = 0;
    CAmount nPayFee             = 0;
    CAmount nAfterFee           = 0;
    CAmount nChange             = 0;
    bool fAllowFree             = false;

    // Amount
    nAmount += inputAmount;

    // Fee
    nPayFee = ASYNC_RPC_OPERATION_DEFAULT_MINERS_FEE;
    if (nPayFee > 0 && nMinimumTotalFee > nPayFee)
        nPayFee = nMinimumTotalFee;

    // Allow free?
    if (fSendFreeTransactions)
        if (fAllowFree)
            nPayFee = 0;

    if (nPayAmount > 0)
    {
        nChange = nAmount - nPayAmount;
        if (!SendZCoinsDialog::fSubtractFeeFromAmount)
            nChange -= nPayFee;

        // Never create dust outputs; if we would, just add the dust to the fee.
        if (nChange > 0 && nChange < MIN_CHANGE)
        {
            CTxOut txout(nChange, (CScript)std::vector<unsigned char>(24, 0));
            if (txout.IsDust(::minRelayTxFee))
            {
                if (SendZCoinsDialog::fSubtractFeeFromAmount) // dust-change will be raised until no dust
                    nChange = txout.GetDustThreshold(::minRelayTxFee);
                else
                {
                    nPayFee += nChange;
                    nChange = 0;
                }
            }
        }
    }

    // after fee
    nAfterFee = nAmount - nPayFee;
    if (nAfterFee < 0)
        nAfterFee = 0;

    // actually update labels
    int nDisplayUnit = BitcoinUnits::LTZ;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    // enable/disable "dust" and "change"
    ui->labelCoinSelectionLowOutputText->setEnabled(nPayAmount > 0);
    ui->labelCoinSelectionLowOutput->setEnabled(nPayAmount > 0);
    ui->labelCoinSelectionChangeText->setEnabled(nPayAmount > 0);
    ui->labelCoinSelectionChange->setEnabled(nPayAmount > 0);
 
    // stats
    ui->labelCoinSelectionAmount->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));     // Amount
    ui->labelCoinSelectionFee->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee));        // Fee
    ui->labelCoinSelectionAfterFee->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee)); // After Fee
    ui->labelCoinSelectionChange->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nChange));     // Change
    ui->labelCoinSelectionLowOutput->setText(fDust ? tr("yes") : tr("no"));                            // Dust
    if (nPayFee > 0 && (nMinimumTotalFee < nPayFee))
    {
        ui->labelCoinSelectionFee->setText(ASYMP_UTF8 + ui->labelCoinSelectionFee->text());
        ui->labelCoinSelectionAfterFee->setText(ASYMP_UTF8 + ui->labelCoinSelectionAfterFee->text());
        if (nChange > 0 && !SendZCoinsDialog::fSubtractFeeFromAmount)
            ui->labelCoinSelectionChange->setText(ASYMP_UTF8 + ui->labelCoinSelectionChange->text());
    }

    ui->labelCoinSelectionLowOutput->setStyleSheet((fDust) ? "color:red;" : "");                                     // Dust = "yes"

    QString toolTip3 = tr("This label turns red if any recipient receives an amount smaller than the current dust threshold.");

    // how many satoshis the estimated fee can vary per byte we guess wrong
    double dFeeVary;
    if (payTxFee.GetFeePerK() > 0)
        dFeeVary = (double)std::max(CWallet::minTxFee.GetFeePerK(), payTxFee.GetFeePerK()) / 1000;
    else
        dFeeVary = (double)std::max(CWallet::minTxFee.GetFeePerK(), mempool.estimateFee(nTxConfirmTarget).GetFeePerK()) / 1000;
    QString toolTip4 = tr("Can vary +/- %1 satoshi(s) per input.").arg(dFeeVary);

    ui->labelCoinSelectionFee->setToolTip(toolTip4);
    ui->labelCoinSelectionAfterFee->setToolTip(toolTip4);
    ui->labelCoinSelectionLowOutput->setToolTip(toolTip3);
    ui->labelCoinSelectionChange->setToolTip(toolTip4);

    // Insufficient funds
    ui->labelCoinSelectionInsuffFunds->setVisible(nChange < 0);
}

SendZConfirmationDialog::SendZConfirmationDialog(const QString &title, const QString &text, int secDelay,
    QWidget *parent) :
    QMessageBox(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::Cancel, parent), secDelay(secDelay)
{
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));
}

int SendZConfirmationDialog::exec()
{
    updateYesButton();
    countDownTimer.start(1000);
    return QMessageBox::exec();
}

void SendZConfirmationDialog::countDown()
{
    secDelay--;
    updateYesButton();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

void SendZConfirmationDialog::updateYesButton()
{
    if(secDelay > 0)
    {
        yesButton->setEnabled(false);
        yesButton->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    }
    else
    {
        yesButton->setEnabled(true);
        yesButton->setText(tr("Yes"));
    }
}
