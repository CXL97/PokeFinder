/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2024 by Admiral_Fish, bumba, and EzPzStreamz
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

#ifndef EGG3MODEL_HPP
#define EGG3MODEL_HPP

#include <Core/Gen3/States/EggState3.hpp>
#include <Model/TableModel.hpp>

/**
 * @brief Provides a table model implementation to show egg encounter information for Gen 3
 */
class EggModel3 : public TableModel<EggState3>
{
    Q_OBJECT
public:
    /**
     * @brief Construct a new EggGeneratorModel3 object
     *
     * @param parent Parent object, which takes memory ownership
     * @param emerald Whether the version is Emerald or not
     */
    EggModel3(QObject *parent, bool emerald);

    /**
     * @brief Returns the number of columns in the model
     *
     * @param parent Unused parent index
     *
     * @return Number of columns
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Returns data at the \p index with \p role
     *
     * @param index Row/column index
     * @param role Model data role
     *
     * @return Data at index
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Returns header text at the \p section, \p orientation, and \p role
     *
     * @param section Column index
     * @param orientation Header position
     * @param role Model data role
     *
     * @return Header text at column
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    /**
     * @brief Sets flag that controls whether the model display inheritance or stats/IVs
     *
     * @param flag Whether to show inheritance or not
     */
    void setShowInheritance(bool flag);

    /**
     * @brief Sets flag that controls whether the model display stats or IVs
     *
     * @param flag Whether to show stats or not
     */
    void setShowStats(bool flag);

private:
    QStringList header = { tr("Held Advances"), tr("Pickup Advances"),
                           tr("Redraws"),       tr("PID"),
                           tr("Shiny"),         tr("Nature"),
                           tr("Ability"),       tr("HP"),
                           tr("Atk"),           tr("Def"),
                           tr("SpA"),           tr("SpD"),
                           tr("Spe"),           tr("Hidden"),
                           tr("Power"),         tr("Gender") };
    bool emerald;
    bool showInheritance;
    bool showStats;

    int getColumn(int column) const;
};

#endif // EGG3MODEL_HPP
