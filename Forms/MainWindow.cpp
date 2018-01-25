/*
 * This file is part of PokéFinder
 * Copyright (C) 2017 by Admiral_Fish and bumba
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    SetupLanguage();
    SetupModels();

    s3.setAttribute(Qt::WA_QuitOnClose, false);
    w3.setAttribute(Qt::WA_QuitOnClose, false);
}

void MainWindow::SetupLanguage()
{
    QActionGroup* langGroup = new QActionGroup(ui->menuLanguage);
    connect(langGroup, SIGNAL (triggered(QAction *)), this, SLOT (slotLanguageChanged(QAction *)));
    langGroup->setExclusive(true);
    QStringList files = QDir(langPath).entryList(QStringList("PokeFinder_*.qm"));

    QString defaultLocale = QLocale::system().name();
    defaultLocale.truncate(defaultLocale.lastIndexOf('_'));

    for (int i = 0; i < files.size(); i++)
    {
        QString locale = files[i];
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);

        QAction *action = new QAction(this);

        action->setCheckable(true);
        action->setData(locale);

        if (defaultLocale == locale)
            action->setChecked(true);

        ui->menuLanguage->addAction(action);
        langGroup->addAction(action);
    }
    slotLanguageChanged(langGroup->checkedAction());
}

void MainWindow::SetupModels()
{
    QList<QAction*> actions = ui->menuLanguage->actions();
    QStringList langs = QStringList() << tr("German") << tr("English") << tr("Spanish") << tr("French") << tr("Italian") << tr("Japanese") << tr("Korean") << tr("Chinese");
    QMap<QString, int> keys;
    keys["de"] = 0;
    keys["en"] = 1;
    keys["es"] = 2;
    keys["fr"] = 3;
    keys["it"] = 4;
    keys["ja"] = 5;
    keys["ko"] = 6;
    keys["zh_Hans_CN"] = 7;

    for (QAction *action : actions)
        action->setText(langs[keys[action->data().toString()]]);
}

void MainWindow::slotLanguageChanged(QAction *action)
{
    if (action != NULL)
        LoadLanguage(action->data().toString());
}

void MainWindow::LoadLanguage(const QString &lang)
{
    if(currLang != lang)
    {
        currLang = lang;
        QLocale locale = QLocale(currLang);
        QLocale::setDefault(locale);
        QString languageName = QLocale::languageToString(locale.language());
        SwitchTranslator(translator, QString("PokeFinder_%1.qm").arg(currLang));
    }
}

void MainWindow::SwitchTranslator(QTranslator &translator, const QString &filename)
{
    qApp->removeTranslator(&translator);
    if(translator.load(langPath + filename))
        qApp->installTranslator(&translator);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event != NULL)
    {
        switch (event->type())
        {
            case QEvent::LanguageChange:
                ui->retranslateUi(this);
                break;
            case QEvent::LocaleChange:
            {
                QString locale = QLocale::system().name();
                locale.truncate(locale.lastIndexOf('_'));
                LoadLanguage(locale);
                break;
            }
            default:
                break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonStationary3_clicked()
{
    s3.show();
    s3.raise();
}

void MainWindow::on_actionResearcher_triggered()
{
    Researcher *r = new Researcher();
    r->setAttribute(Qt::WA_QuitOnClose, false);
    r->show();
}

void MainWindow::on_pushButtonWild3_clicked()
{
    w3.show();
    w3.raise();
}
