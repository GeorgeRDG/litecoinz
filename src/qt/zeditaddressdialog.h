// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ZEDITADDRESSDIALOG_H
#define BITCOIN_QT_ZEDITADDRESSDIALOG_H

#include <QDialog>

class ZAddressTableModel;

namespace Ui {
    class ZEditAddressDialog;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an z-address and associated information.
 */
class ZEditAddressDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewReceivingAddress,
        NewSendingAddress,
        EditReceivingAddress,
        EditSendingAddress
    };

    explicit ZEditAddressDialog(Mode mode, QWidget *parent);
    ~ZEditAddressDialog();

    void setModel(ZAddressTableModel *model);
    void loadRow(int row);

    QString getAddress() const;
    void setAddress(const QString &address);

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();

    Ui::ZEditAddressDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    ZAddressTableModel *model;

    QString address;
};

#endif // BITCOIN_QT_ZEDITADDRESSDIALOG_H
