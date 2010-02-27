/*
  This file is part of Qween.
  Copyright (C) 2009-2010 NOSE Takafumi <ahya365@gmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  In addition, as a special exception, NOSE Takafumi
  gives permission to link the code of its release of Qween with the
  OpenSSL project's "OpenSSL" library (or with modified versions of it
  that use the same license as the "OpenSSL" library), and distribute
  the linked executables.  You must obey the GNU General Public License
  in all respects for all of the code used other than "OpenSSL".  If you
  modify this file, you may extend this exception to your version of the
  file, but you are not obligated to do so.  If you do not wish to do
  so, delete this exception statement from your version.
*/

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "const.h"
#include <QFile>
/**/
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->textBrowser_about->append("<p align=\"center\"><img src=\":/res/qween_icon.png\" width=\"96\" height=\"96\" /></p>");
    ui->textBrowser_about->append(
            QString("<p align=\"center\"><span style=\" font-size:xx-large; font-weight:600;\">Qween v%1.%2.%3</span></p>")
            .arg(QWEEN_VERSION_MAJOR).arg(QWEEN_VERSION_MINOR).arg(QWEEN_VERSION_REV));

    QFile commitInfo("commit.txt");
    if(commitInfo.exists()){
        QString aboutText = "<p align=\"left\">Commit: %1</p>";
        commitInfo.open(QIODevice::ReadOnly|QIODevice::Text);
        QString line(commitInfo.readLine());
        line.replace("commit ","");
        m_commitId = line;
        ui->textBrowser_about->append(aboutText.arg(line));
        commitInfo.close();
    }

    ui->textBrowser_about->append("<p align=\"left\">Copyright (C) 2009-2010 NOSE Takafumi &lt;ahya365@gmail.com&gt;</p>");

    ui->textEdit_debug->appendPlainText(QString("Commit ID: %1").arg(m_commitId));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
