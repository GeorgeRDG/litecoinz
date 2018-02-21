// Copyright (c) 2017-2018 The LitecoinZ developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "resultsdialog.h"
#include "ui_resultsdialog.h"
#include "platformstyle.h"

#include <univalue.h>
#include "rpc/server.h"

#include <QTimer>
#include <QMessageBox>

ResultsDialog::ResultsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResultsDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowTitleHint);

    ui->buttonBox->hide();
    ui->progressBar->hide();
}

int ResultsDialog::exec()
{
    counter = 0;
    skipEsc = true;

    ui->progressBar->setValue(counter);
    ui->progressBar->setMinimum(0);
    ui->progressBar->show();

    connect(&timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    timer.start(1000);
    QDialog::exec();
}

void ResultsDialog::reject()
{
    if(!skipEsc)
        QDialog::reject();
}

void ResultsDialog::setOperationId(QString opid)
{
    strOperationId = opid;
    ui->labelOperationId->setText("opid: " + strOperationId);
}

void ResultsDialog::setLabels(QString label1, QString label2, QString label3, QString label4, QString label5)
{
    ui->label_1->setText(label1);
    ui->label_2->setText(label2);
    ui->label_3->setText(label3);
    ui->label_4->setText(label4);
    ui->label_5->setText(label5);
}

void ResultsDialog::updateProgressBar()
{
    QString strStatus;
    try {
        /* Check and display the operation status */
        UniValue obj(UniValue::VARR);
        UniValue params(UniValue::VARR);
        obj.push_back(strOperationId.toStdString());
        params.push_back(obj);
        UniValue ret = z_getoperationstatus(params, false);

        UniValue status = find_value(ret[0], "status");
        strStatus = QString::fromStdString(status.get_str());
    } catch (std::exception &e) {
        strStatus = "Ops... an internal error occurred!";
        qDebug("Error %s ", e.what());
        QMessageBox msgBox("", e.what(), QMessageBox::Critical, 0, 0, 0, this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
        msgBox.exec();
    } catch (...) {
        strStatus = "Ops... an internal error occurred!";
        qDebug("Error <unknown>");
        QMessageBox msgBox("", "Error <unknown>", QMessageBox::Critical, 0, 0, 0, this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
        msgBox.exec();
    }

    ui->labelResult->setText("Status: " + strStatus);

    if(strStatus == "executing")
    {
        counter++;
        ui->progressBar->setValue(counter);
    }
    else
    {
        timer.stop();
        ui->progressBar->setValue(100);
        ui->progressBar->hide();
        ui->buttonBox->show();
        skipEsc = false;
    }
}

ResultsDialog::~ResultsDialog()
{
    delete ui;
}

void ResultsDialog::on_buttonBox_clicked()
{
    QDialog::done(QDialog::Accepted);
}
