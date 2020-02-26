/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2020 by Admiral_Fish, bumba, and EzPzStreamz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include "Wild4.hpp"
#include "ui_Wild4.h"
#include <Core/Enum/Encounter.hpp>
#include <Core/Enum/Lead.hpp>
#include <Core/Enum/Method.hpp>
#include <Core/Gen4/Encounters4.hpp>
#include <Core/Gen4/Generators/WildGenerator4.hpp>
#include <Core/Gen4/ProfileLoader4.hpp>
#include <Core/Gen4/Searchers/WildSearcher4.hpp>
#include <Core/Parents/Filters/FrameFilter.hpp>
#include <Core/Parents/Frames/WildFrame.hpp>
#include <Core/Util/Nature.hpp>
#include <Core/Util/Translator.hpp>
#include <Forms/Gen4/Profile/ProfileManager4.hpp>
#include <Forms/Gen4/Tools/SeedtoTime4.hpp>
#include <Models/Gen4/WildModel4.hpp>
#include <QSettings>
#include <QThread>
#include <QTimer>

Wild4::Wild4(QWidget *parent) : QWidget(parent), ui(new Ui::Wild4)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_QuitOnClose, false);

    updateProfiles();
    setupModels();

    qRegisterMetaType<QVector<WildFrame>>("QVector<WildFrame>");
}

Wild4::~Wild4()
{
    QSettings setting;
    setting.beginGroup("wild4");
    setting.setValue("minDelay", ui->textBoxSearcherMinDelay->text());
    setting.setValue("maxDelay", ui->textBoxSearcherMaxDelay->text());
    setting.setValue("minFrame", ui->textBoxSearcherMinFrame->text());
    setting.setValue("maxFrame", ui->textBoxSearcherMaxFrame->text());
    setting.setValue("profile", ui->comboBoxProfiles->currentIndex());
    setting.setValue("geometry", this->saveGeometry());
    setting.endGroup();

    delete ui;
}

void Wild4::updateProfiles()
{
    connect(ui->comboBoxProfiles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::profilesIndexChanged);

    profiles = ProfileLoader4::getProfiles();
    profiles.insert(profiles.begin(), Profile4());

    ui->comboBoxProfiles->clear();

    for (const auto &profile : profiles)
    {
        ui->comboBoxProfiles->addItem(profile.getName());
    }

    QSettings setting;
    int val = setting.value("wild4/profile", 0).toInt();
    if (val < ui->comboBoxProfiles->count())
    {
        ui->comboBoxProfiles->setCurrentIndex(val);
    }
}

void Wild4::setupModels()
{
    generatorModel = new WildGeneratorModel4(ui->tableViewGenerator, Method::MethodJ);
    searcherModel = new WildSearcherModel4(ui->tableViewSearcher, Method::Method1);
    generatorMenu = new QMenu(ui->tableViewGenerator);
    searcherMenu = new QMenu(ui->tableViewSearcher);

    ui->tableViewGenerator->setModel(generatorModel);
    ui->tableViewSearcher->setModel(searcherModel);

    ui->textBoxGeneratorSeed->setValues(InputType::Seed32Bit);
    ui->textBoxGeneratorStartingFrame->setValues(InputType::Frame32Bit);
    ui->textBoxGeneratorMaxResults->setValues(InputType::Frame32Bit);
    ui->textBoxGeneratorDelay->setValues(InputType::Frame32Bit);

    ui->textBoxSearcherMinDelay->setValues(InputType::Delay);
    ui->textBoxSearcherMaxDelay->setValues(InputType::Delay);
    ui->textBoxSearcherMinFrame->setValues(InputType::Frame32Bit);
    ui->textBoxSearcherMaxFrame->setValues(InputType::Frame32Bit);

    ui->comboBoxGeneratorGender->setup({ 255, 0, 1 });
    ui->comboBoxSearcherGender->setup({ 255, 0, 1 });

    ui->comboBoxGeneratorAbility->setup({ 255, 0, 1 });
    ui->comboBoxSearcherAbility->setup({ 255, 0, 1 });

    ui->comboBoxGeneratorGenderRatio->setup({ 255, 127, 191, 63, 31, 0, 254 });
    ui->comboBoxSearcherGenderRatio->setup({ 255, 127, 191, 63, 31, 0, 254 });

    ui->comboBoxGeneratorLead->addItem(tr("None"));
    ui->comboBoxGeneratorLead->addItems(Translator::getNatures());

    ui->comboBoxGeneratorNature->setup(Translator::getNatures());
    ui->comboBoxSearcherNature->setup(Translator::getNatures());

    ui->comboBoxGeneratorHiddenPower->setup(Translator::getHiddenPowers());
    ui->comboBoxSearcherHiddenPower->setup(Translator::getHiddenPowers());

    QAction *outputTXTGenerator = generatorMenu->addAction(tr("Output Results to TXT"));
    QAction *outputCSVGenerator = generatorMenu->addAction(tr("Output Results to CSV"));
    connect(outputTXTGenerator, &QAction::triggered, [=] { ui->tableViewGenerator->outputModel(); });
    connect(outputCSVGenerator, &QAction::triggered, [=] { ui->tableViewGenerator->outputModel(true); });

    QAction *seedToTime = searcherMenu->addAction(tr("Generate times for seed"));
    QAction *outputTXTSearcher = searcherMenu->addAction(tr("Output Results to TXT"));
    QAction *outputCSVSearcher = searcherMenu->addAction(tr("Output Results to CSV"));
    connect(seedToTime, &QAction::triggered, this, &Wild4::seedToTime);
    connect(outputTXTSearcher, &QAction::triggered, [=] { ui->tableViewSearcher->outputModel(); });
    connect(outputCSVSearcher, &QAction::triggered, [=] { ui->tableViewSearcher->outputModel(true); });

    connect(ui->pushButtonGenerate, &QPushButton::clicked, this, &Wild4::generate);
    connect(ui->pushButtonSearch, &QPushButton::clicked, this, &Wild4::search);
    connect(ui->pushButtonGeneratorLead, &QPushButton::clicked, this, &Wild4::generatorLead);
    connect(ui->comboBoxGeneratorEncounter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Wild4::generatorEncounterIndexChanged);
    connect(ui->comboBoxSearcherEncounter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Wild4::searcherEncounterIndexChanged);
    connect(ui->comboBoxGeneratorLocation, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Wild4::generatorLocationIndexChanged);
    connect(ui->comboBoxSearcherLocation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::searcherLocationIndexChanged);
    connect(ui->comboBoxGeneratorPokemon, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::generatorPokemonIndexChanged);
    connect(ui->comboBoxSearcherPokemon, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::searcherPokemonIndexChanged);
    connect(ui->comboBoxGeneratorTime, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::generatorTimeIndexChanged);
    connect(ui->comboBoxSearcherTime, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Wild4::searcherTimeIndexChanged);
    connect(ui->tableViewGenerator, &QTableView::customContextMenuRequested, this, &Wild4::tableViewGeneratorContextMenu);
    connect(ui->tableViewSearcher, &QTableView::customContextMenuRequested, this, &Wild4::tableViewSearcherContextMenu);
    connect(ui->pushButtonProfileManager, &QPushButton::clicked, this, &Wild4::profileManager);

    generatorEncounterIndexChanged(0);
    searcherEncounterIndexChanged(0);

    QSettings setting;
    setting.beginGroup("wild4");
    if (setting.contains("minDelay"))
    {
        ui->textBoxSearcherMinDelay->setText(setting.value("minDelay").toString());
    }
    if (setting.contains("maxDelay"))
    {
        ui->textBoxSearcherMaxDelay->setText(setting.value("maxDelay").toString());
    }
    if (setting.contains("minFrame"))
    {
        ui->textBoxSearcherMinFrame->setText(setting.value("minFrame").toString());
    }
    if (setting.contains("maxFrame"))
    {
        ui->textBoxSearcherMaxFrame->setText(setting.value("maxFrame").toString());
    }
    if (setting.contains("geometry"))
    {
        this->restoreGeometry(setting.value("geometry").toByteArray());
    }
    setting.endGroup();
}

void Wild4::updateLocationsGenerator()
{
    auto encounter = static_cast<Encounter>(ui->comboBoxGeneratorEncounter->currentData().toInt());
    int time = ui->comboBoxGeneratorTime->currentIndex();
    auto profile = profiles.at(ui->comboBoxProfiles->currentIndex());

    encounterGenerator = Encounters4::getEncounters(encounter, time, profile);

    QVector<u8> locs;
    for (const auto &area : encounterGenerator)
    {
        locs.append(area.getLocation());
    }

    QStringList locations = Translator::getLocations(locs, profile.getVersion());

    ui->comboBoxGeneratorLocation->clear();
    ui->comboBoxGeneratorLocation->addItems(locations);
}

void Wild4::updateLocationsSearcher()
{
    auto encounter = static_cast<Encounter>(ui->comboBoxSearcherEncounter->currentData().toInt());
    int time = ui->comboBoxSearcherTime->currentIndex();
    auto profile = profiles.at(ui->comboBoxProfiles->currentIndex());

    encounterSearcher = Encounters4::getEncounters(encounter, time, profile);

    QVector<u8> locs;
    for (const auto &area : encounterSearcher)
    {
        locs.append(area.getLocation());
    }

    QStringList locations = Translator::getLocations(locs, profile.getVersion());

    ui->comboBoxSearcherLocation->clear();
    ui->comboBoxSearcherLocation->addItems(locations);
}

void Wild4::updatePokemonGenerator()
{
    auto area = encounterGenerator.at(ui->comboBoxGeneratorLocation->currentIndex());
    QVector<u16> species = area.getUniqueSpecies();

    QStringList names = area.getSpecieNames();

    ui->comboBoxGeneratorPokemon->clear();
    ui->comboBoxGeneratorPokemon->addItem("-");
    for (auto i = 0; i < species.size(); i++)
    {
        ui->comboBoxGeneratorPokemon->addItem(names.at(i), species.at(i));
    }
}

void Wild4::updatePokemonSearcher()
{
    auto area = encounterSearcher.at(ui->comboBoxSearcherLocation->currentIndex());
    QVector<u16> species = area.getUniqueSpecies();

    QStringList names = area.getSpecieNames();

    ui->comboBoxSearcherPokemon->clear();
    ui->comboBoxSearcherPokemon->addItem("-");
    for (auto i = 0; i < species.size(); i++)
    {
        ui->comboBoxSearcherPokemon->addItem(names.at(i), species.at(i));
    }
}

void Wild4::updateProgress(const QVector<WildFrame> &frames, int progress)
{
    searcherModel->addItems(frames);
    ui->progressBar->setValue(progress);
}

void Wild4::generate()
{
    auto method = static_cast<Method>(ui->comboBoxGeneratorMethod->getCurrentInt());
    generatorModel->clearModel();
    generatorModel->setMethod(method);

    u32 seed = ui->textBoxGeneratorSeed->getUInt();
    u32 initialFrame = ui->textBoxGeneratorStartingFrame->getUInt();
    u32 maxResults = ui->textBoxGeneratorMaxResults->getUInt();
    u16 tid = currentProfile.getTID();
    u16 sid = currentProfile.getSID();
    u8 genderRatio = ui->comboBoxGeneratorGenderRatio->getCurrentByte();
    u32 offset = 0;
    if (ui->checkBoxGeneratorDelay->isChecked())
    {
        offset = ui->textBoxGeneratorDelay->getUInt();
    }

    FrameFilter filter(ui->comboBoxGeneratorGender->getCurrentByte(), ui->comboBoxGeneratorAbility->getCurrentByte(),
                       ui->checkBoxGeneratorShinyOnly->isChecked(), ui->checkBoxGeneratorDisableFilters->isChecked(),
                       ui->ivFilterGenerator->getLower(), ui->ivFilterGenerator->getUpper(), ui->comboBoxGeneratorNature->getChecked(),
                       ui->comboBoxGeneratorHiddenPower->getChecked(), ui->comboBoxGeneratorEncounterSlot->getChecked());

    WildGenerator4 generator(initialFrame, maxResults, tid, sid, genderRatio, method, filter);
    generator.setOffset(offset);
    generator.setEncounter(static_cast<Encounter>(ui->comboBoxGeneratorEncounter->getCurrentInt()));

    if (ui->pushButtonGeneratorLead->text() == tr("Cute Charm"))
    {
        generator.setLead((static_cast<Lead>(ui->comboBoxGeneratorLead->getCurrentInt())));
    }
    else if (ui->pushButtonGeneratorLead->text() == tr("Suction Cups"))
    {
        generator.setLead(Lead::SuctionCups);
    }
    else
    {
        int num = ui->comboBoxGeneratorLead->currentIndex();
        if (num == 0)
        {
            generator.setLead(Lead::None);
        }
        else
        {
            generator.setLead(Lead::Synchronize);
            generator.setSynchNature(static_cast<u8>(ui->comboBoxGeneratorLead->currentIndex() - 1));
        }
    }
    generator.setEncounterArea(encounterGenerator.at(ui->comboBoxGeneratorLocation->currentIndex()));

    auto frames = generator.generate(seed);
    generatorModel->addItems(frames);
}

void Wild4::search()
{
    auto method = static_cast<Method>(ui->comboBoxSearcherMethod->getCurrentInt());
    searcherModel->clearModel();
    searcherModel->setMethod(method);

    ui->pushButtonSearch->setEnabled(false);
    ui->pushButtonCancel->setEnabled(true);

    QVector<u8> min = ui->ivFilterSearcher->getLower();
    QVector<u8> max = ui->ivFilterSearcher->getUpper();

    FrameFilter filter(ui->comboBoxSearcherGender->getCurrentByte(), ui->comboBoxSearcherAbility->getCurrentByte(),
                       ui->checkBoxSearcherShinyOnly->isChecked(), false, min, max, ui->comboBoxSearcherNature->getChecked(),
                       ui->comboBoxSearcherHiddenPower->getChecked(), ui->comboBoxSearcherEncounterSlot->getChecked());

    u16 tid = currentProfile.getTID();
    u16 sid = currentProfile.getSID();
    u8 genderRatio = ui->comboBoxSearcherGenderRatio->getCurrentByte();

    auto *searcher = new WildSearcher4(tid, sid, genderRatio, method, filter);
    searcher->setDelay(ui->textBoxSearcherMinDelay->getUInt(), ui->textBoxSearcherMaxDelay->getUInt());
    searcher->setFrame(ui->textBoxSearcherMinFrame->getUInt(), ui->textBoxSearcherMaxFrame->getUInt());
    searcher->setEncounter(static_cast<Encounter>(ui->comboBoxSearcherEncounter->getCurrentInt()));
    searcher->setEncounterArea(encounterSearcher.at(ui->comboBoxSearcherLocation->currentIndex()));
    searcher->setLead(static_cast<Lead>(ui->comboBoxSearcherLead->getCurrentInt()));

    int maxProgress = 1;
    for (u8 i = 0; i < 6; i++)
    {
        maxProgress *= max.at(i) - min.at(i) + 1;
    }
    ui->progressBar->setRange(0, maxProgress);

    auto *thread = QThread::create([=] { searcher->startSearch(min, max); });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(ui->pushButtonCancel, &QPushButton::clicked, [searcher] { searcher->cancelSearch(); });

    auto *timer = new QTimer();
    connect(timer, &QTimer::timeout, [=] { updateProgress(searcher->getResults(), searcher->getProgress()); });
    connect(thread, &QThread::finished, timer, &QTimer::stop);
    connect(thread, &QThread::finished, timer, &QTimer::deleteLater);
    connect(timer, &QTimer::destroyed, [=] {
        ui->pushButtonSearch->setEnabled(true);
        ui->pushButtonCancel->setEnabled(false);
        updateProgress(searcher->getResults(), searcher->getProgress());
        delete searcher;
    });

    thread->start();
    timer->start(1000);
}

void Wild4::profilesIndexChanged(int index)
{
    if (index >= 0)
    {
        currentProfile = profiles.at(index);

        ui->labelProfileTIDValue->setText(QString::number(currentProfile.getTID()));
        ui->labelProfileSIDValue->setText(QString::number(currentProfile.getSID()));
        ui->labelProfileGameValue->setText(currentProfile.getVersionString());
        ui->labelProfileDualSlotValue->setText(currentProfile.getDualSlotString());
        ui->labelProfileRadioValue->setText(currentProfile.getRadioString());
        ui->labelProfilePokeRadarValue->setText(currentProfile.getRadar() ? tr("Yes") : tr("No"));
        ui->labelProfileSwarmValue->setText(currentProfile.getSwarm() ? tr("Yes") : tr("No"));

        bool flag = currentProfile.getVersion() & Game::HGSS;

        ui->comboBoxGeneratorMethod->clear();
        ui->comboBoxGeneratorMethod->addItem(flag ? tr("Method K") : tr("Method J"), flag ? Method::MethodK : Method::MethodJ);
        ui->comboBoxGeneratorMethod->addItem(tr("Chained Shiny"), Method::ChainedShiny);

        ui->comboBoxSearcherMethod->clear();
        ui->comboBoxSearcherMethod->addItem(flag ? tr("Method K") : tr("Method J"), flag ? Method::MethodK : Method::MethodJ);
        ui->comboBoxSearcherMethod->addItem(tr("Chained Shiny"), Method::ChainedShiny);

        ui->comboBoxGeneratorEncounter->clear();
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        if (flag)
        {
            ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        }
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);

        ui->comboBoxSearcherEncounter->clear();
        ui->comboBoxSearcherEncounter->addItem(tr("Grass"), Encounter::Grass);
        if (flag)
        {
            ui->comboBoxSearcherEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        }
        ui->comboBoxSearcherEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxSearcherEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxSearcherEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxSearcherEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);

        ui->comboBoxSearcherLead->clear();
        ui->comboBoxSearcherLead->addItem(tr("Any"), Lead::Search);
        ui->comboBoxSearcherLead->addItem(tr("Synchronize"), Lead::Synchronize);
        ui->comboBoxSearcherLead->addItem(tr("Cute Charm"), Lead::CuteCharm);
        if (flag)
        {
            ui->comboBoxSearcherLead->addItem("Suction Cups", Lead::SuctionCups);
        }
        ui->comboBoxSearcherLead->addItem("None", Lead::None);

        ui->pushButtonGeneratorLead->setText(tr("Synchronize"));
        ui->comboBoxGeneratorLead->addItem("None");
        ui->comboBoxGeneratorLead->addItems(Translator::getNatures());

        updateLocationsSearcher();
        updateLocationsGenerator();
    }
}

void Wild4::generatorLead()
{
    ui->comboBoxGeneratorLead->clear();
    QString text = ui->pushButtonGeneratorLead->text();
    if (text == tr("Synchronize"))
    {
        auto profile = profiles.at(ui->comboBoxProfiles->currentIndex());
        bool flag = profile.getVersion() & Game::HGSS;
        if (flag)
        {
            ui->pushButtonGeneratorLead->setText(tr("Suction Cups"));
            ui->comboBoxGeneratorLead->setEnabled(false);
        }
        else
        {
            ui->pushButtonGeneratorLead->setText(tr("Cute Charm"));
            ui->comboBoxGeneratorLead->setEnabled(true);

            ui->comboBoxGeneratorLead->addItem(tr("♂ Lead"), Lead::CuteCharmFemale);
            ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (50% ♂ Target)"), Lead::CuteCharm50M);
            ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (75% ♂ Target)"), Lead::CuteCharm75M);
            ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (25% ♂ Target)"), Lead::CuteCharm25M);
            ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (87.5% ♂ Target)"), Lead::CuteCharm875M);
        }
    }
    else if (text == tr("Suction Cups"))
    {
        ui->pushButtonGeneratorLead->setText(tr("Cute Charm"));
        ui->comboBoxGeneratorLead->setEnabled(true);

        ui->comboBoxGeneratorLead->addItem(tr("♂ Lead"), Lead::CuteCharmFemale);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (50% ♂ Target)"), Lead::CuteCharm50M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (75% ♂ Target)"), Lead::CuteCharm75M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (25% ♂ Target)"), Lead::CuteCharm25M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (87.5% ♂ Target)"), Lead::CuteCharm875M);
    }
    else
    {
        ui->pushButtonGeneratorLead->setText(tr("Synchronize"));

        ui->comboBoxGeneratorLead->addItem("None");
        ui->comboBoxGeneratorLead->addItems(Translator::getNatures());
    }
}

void Wild4::generatorEncounterIndexChanged(int index)
{
    if (index >= 0)
    {
        QStringList t;
        auto encounter = static_cast<Encounter>(ui->comboBoxGeneratorEncounter->currentData().toInt());

        switch (encounter)
        {
        case Encounter::Grass:
            t = QStringList({ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" });
            break;
        case Encounter::Surfing:
        case Encounter::OldRod:
        case Encounter::GoodRod:
        case Encounter::SuperRod:
            t = QStringList({ "0", "1", "2", "3", "4" });
            break;
        case Encounter::RockSmash:
            t = QStringList({ "0", "1" });
            break;
        default:
            break;
        }

        ui->comboBoxGeneratorEncounterSlot->setup(t);
        updateLocationsGenerator();
    }
}

void Wild4::searcherEncounterIndexChanged(int index)
{
    if (index >= 0)
    {
        QStringList t;
        auto encounter = static_cast<Encounter>(ui->comboBoxSearcherEncounter->currentData().toInt());

        switch (encounter)
        {
        case Encounter::Grass:
            t = QStringList({ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" });
            break;
        case Encounter::Surfing:
        case Encounter::OldRod:
        case Encounter::GoodRod:
        case Encounter::SuperRod:
            t = QStringList({ "0", "1", "2", "3", "4" });
            break;
        case Encounter::RockSmash:
            t = QStringList({ "0", "1" });
            break;
        default:
            break;
        }

        ui->comboBoxSearcherEncounterSlot->setup(t);
        updateLocationsSearcher();
    }
}

void Wild4::generatorLocationIndexChanged(int index)
{
    if (index >= 0)
    {
        updatePokemonGenerator();
    }
}

void Wild4::searcherLocationIndexChanged(int index)
{
    if (index >= 0)
    {
        updatePokemonSearcher();
    }
}

void Wild4::generatorPokemonIndexChanged(int index)
{
    if (index <= 0)
    {
        ui->comboBoxGeneratorEncounterSlot->resetChecks();
    }
    else
    {
        u16 num = ui->comboBoxGeneratorPokemon->getCurrentUShort();
        QVector<bool> flags = encounterGenerator.at(ui->comboBoxGeneratorLocation->currentIndex()).getSlots(num);

        ui->comboBoxGeneratorEncounterSlot->setChecks(flags);
    }
}

void Wild4::searcherPokemonIndexChanged(int index)
{
    if (index <= 0)
    {
        ui->comboBoxSearcherEncounterSlot->resetChecks();
    }
    else
    {
        u16 num = ui->comboBoxSearcherPokemon->getCurrentUShort();
        QVector<bool> flags = encounterSearcher.at(ui->comboBoxSearcherLocation->currentIndex()).getSlots(num);

        ui->comboBoxSearcherEncounterSlot->setChecks(flags);
    }
}

void Wild4::generatorTimeIndexChanged(int index)
{
    if (index >= 0)
    {
        int position = ui->comboBoxGeneratorLocation->currentIndex();
        updateLocationsGenerator();
        ui->comboBoxGeneratorLocation->setCurrentIndex(position);
    }
}

void Wild4::searcherTimeIndexChanged(int index)
{
    if (index >= 0)
    {
        int position = ui->comboBoxSearcherLocation->currentIndex();
        updateLocationsSearcher();
        ui->comboBoxSearcherLocation->setCurrentIndex(position);
    }
}

void Wild4::seedToTime()
{
    QModelIndex index = ui->tableViewSearcher->currentIndex();
    auto *time = new SeedtoTime4(searcherModel->data(searcherModel->index(index.row(), 0)).toString(),
                                 profiles.at(ui->comboBoxProfiles->currentIndex()));
    time->show();
    time->raise();
}

void Wild4::tableViewGeneratorContextMenu(QPoint pos)
{
    if (generatorModel->rowCount() > 0)
    {
        generatorMenu->popup(ui->tableViewGenerator->viewport()->mapToGlobal(pos));
    }
}

void Wild4::tableViewSearcherContextMenu(QPoint pos)
{
    if (searcherModel->rowCount() > 0)
    {
        searcherMenu->popup(ui->tableViewSearcher->viewport()->mapToGlobal(pos));
    }
}

void Wild4::profileManager()
{
    auto *manager = new ProfileManager4();
    connect(manager, &ProfileManager4::updateProfiles, this, [=] { emit alertProfiles(4); });
    manager->show();
}
