// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZADDRESSBOOKPAGE_H
#define BITCOIN_QT_ZADDRESSBOOKPAGE_H

#include <QDialog>

class ZAddressTableModel;
class OptionsModel;
class PlatformStyle;

namespace Ui {
    class ZAddressBookPage;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
class QMenu;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
QT_END_NAMESPACE

/** Widget that shows a list of sending or receiving z-addresses.
  */
class ZAddressBookPage : public QDialog
{
    Q_OBJECT

public:
    enum Tabs {
        SendingTab = 0,
        ReceivingTab = 1
    };

    enum Mode {
        ForSelection, /**< Open z-address book to pick z-address */
        ForEditing  /**< Open z-address book for editing */
    };

    explicit ZAddressBookPage(const PlatformStyle *platformStyle, Mode mode, Tabs tab, QWidget *parent);
    ~ZAddressBookPage();

    void setModel(ZAddressTableModel *model);
    const QString &getReturnValue() const { return returnValue; }

public Q_SLOTS:
    void done(int retval);

private:
    Ui::ZAddressBookPage *ui;
    ZAddressTableModel *model;
    Mode mode;
    Tabs tab;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newAddressToSelect;

private Q_SLOTS:
    /** Delete currently selected z-address entry */
    void on_deleteAddress_clicked();
    /** Create a new z-address for receiving coins and / or add a new z-address book entry */
    void on_newAddress_clicked();
    /** Copy z-address of currently selected z-address entry to clipboard */
    void on_copyAddress_clicked();
    /** Copy label of currently selected z-address entry to clipboard (no button) */
    void onCopyLabelAction();
    /** Edit currently selected z-address entry (no button) */
    void onEditAction();
    /** Export button clicked */
    void on_exportButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for z-address book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to z-address table */
    void selectNewAddress(const QModelIndex &parent, int begin, int /*end*/);

Q_SIGNALS:
    void sendCoins(QString addr);
};

#endif // BITCOIN_QT_ZADDRESSBOOKPAGE_H
