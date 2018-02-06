// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "resultsdialog.h"
#include "ui_resultsdialog.h"
#include "platformstyle.h"

ResultsDialog::ResultsDialog(const PlatformStyle *platformStyle, const QString& strTitle, const QString& strLabel_1, const QString& strLabel_2, const QString& strLabel_3, const QString& strLabel_4, const QString& strLabel_5, const QString& strLabel_6, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResultsDialog),
    strTitle(strTitle),
    strLabel_1(strLabel_1),
    strLabel_2(strLabel_2),
    strLabel_3(strLabel_3),
    strLabel_4(strLabel_4),
    strLabel_5(strLabel_5),
    strLabel_6(strLabel_6)
{
    ui->setupUi(this);

    setWindowTitle(strTitle);
    ui->label_1->setText(strLabel_1);
    ui->label_2->setText(strLabel_2);
    ui->label_3->setText(strLabel_3);
    ui->label_4->setText(strLabel_4);
    ui->label_5->setText(strLabel_5);
    ui->label_6->setText(strLabel_6);
}

ResultsDialog::~ResultsDialog()
{
    delete ui;
}

void ResultsDialog::on_buttonBox_clicked()
{
    this->close();
}
