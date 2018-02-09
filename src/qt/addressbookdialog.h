// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ADDRESSBOOKDIALOG_H
#define BITCOIN_QT_ADDRESSBOOKDIALOG_H

#include <QDialog>

class AddressTableNewModel;
class OptionsModel;
class PlatformStyle;

namespace Ui {
    class AddressBookDialog;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QMenu;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Dialog for LitecoinZ Address Book */
class AddressBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddressBookDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~AddressBookDialog();

    void setModel(AddressTableNewModel *model);

public Q_SLOTS:

private:
    Ui::AddressBookDialog *ui;
    AddressTableNewModel *model;
    QSortFilterProxyModel *proxyModelReceivingZ;
    QSortFilterProxyModel *proxyModelReceivingT;
    QSortFilterProxyModel *proxyModelSendingT;

    QMenu *contextReceivingZMenu;
    QMenu *contextReceivingTMenu;
    QMenu *contextSendingTMenu;

    QAction *deleteSendingTAction; // to be able to explicitly disable it

    QString newReceivingZAddressToSelect;
    QString newReceivingTAddressToSelect;
    QString newSendingTAddressToSelect;

    const PlatformStyle *platformStyle;

private Q_SLOTS:
    // Receiving Addresses Tab
    void on_newReceivingZAddress_clicked();
    void on_newReceivingTAddress_clicked();
    void on_copyReceivingZAddress_clicked();
    void on_copyReceivingTAddress_clicked();
    void on_exportReceivingZAddress_clicked();
    void on_exportReceivingTAddress_clicked();

    // Sending Addresses Tab
    void on_newSendingTAddress_clicked();
    void on_copySendingTAddress_clicked();
    void on_deleteSendingTAddress_clicked();
    void on_exportSendingTAddress_clicked();

    /** Set button states based on selection */
    void selectionReceivingZChanged();
    void selectionReceivingTChanged();
    void selectionSendingTChanged();

    /** Spawn contextual menu (right mouse menu) for address book entry */
    void contextualReceivingZMenu(const QPoint &point);
    void contextualReceivingTMenu(const QPoint &point);
    void contextualSendingTMenu(const QPoint &point);

    /** Copy label of currently selected address entry to clipboard (no button) */
    void onCopyLabelReceivingZAction();
    void onCopyLabelReceivingTAction();
    void onCopyLabelSendingTAction();

    /** Edit currently selected address entry (no button) */
    void onEditReceivingTAction();
    void onEditSendingTAction();

Q_SIGNALS:

};

#endif // BITCOIN_QT_ADDRESSBOOKDIALOG_H
