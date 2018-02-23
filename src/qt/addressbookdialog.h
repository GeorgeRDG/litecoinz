// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ADDRESSBOOKDIALOG_H
#define BITCOIN_QT_ADDRESSBOOKDIALOG_H

#include <QDialog>

class AddressTableModel;
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

/** Widget that shows a list of sending or receiving addresses.
  */
class AddressBookDialog : public QDialog
{
    Q_OBJECT

public:
    enum Tabs {
        SendingTab = 0,
        ReceivingTab = 1
    };

    enum Mode {
        ForTSelection,
        ForZSelection
    };

    explicit AddressBookDialog(const PlatformStyle *platformStyle, Mode mode, Tabs tab, QWidget *parent);
    ~AddressBookDialog();

    void setModel(AddressTableModel *model);
    const QString &getReturnValue() const { return returnValue; }

public Q_SLOTS:
    void done(int retval);

private:
    Ui::AddressBookDialog *ui;
    AddressTableModel *model;
    Mode mode;
    Tabs tab;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;

private Q_SLOTS:
    void onCopyAddressAction();
    void onCopyLabelAction();
    void contextualMenu(const QPoint &point);

};

#endif // BITCOIN_QT_ADDRESSBOOKDIALOG_H
