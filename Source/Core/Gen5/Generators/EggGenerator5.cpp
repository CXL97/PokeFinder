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

#include "EggGenerator5.hpp"
#include <Core/Enum/Game.hpp>
#include <Core/Enum/Method.hpp>
#include <Core/Gen5/States/EggState5.hpp>
#include <Core/Parents/PersonalInfo.hpp>
#include <Core/Parents/PersonalLoader.hpp>
#include <Core/RNG/LCRNG64.hpp>
#include <Core/RNG/MTFast.hpp>
#include <Core/Util/Utilities.hpp>
#include <algorithm>

EggGenerator5::EggGenerator5(u32 initialAdvances, u32 maxAdvances, u32 offset, const Daycare &daycare, const Profile5 &profile,
                             const StateFilter &filter) :
    EggGenerator(initialAdvances, maxAdvances, offset, Method::None, 0, daycare, profile, filter),
    ditto(daycare.getDitto()),
    everstone(daycare.getEverstoneCount()),
    parentAbility(daycare.getParentAbility(1)),
    poweritem(daycare.getPowerItemCount()),
    rolls(((profile.getVersion() & Game::BW2) != Game::None && profile.getShinyCharm() ? 2 : 0) + (daycare.getMasuda() ? 5 : 0))
{
}

std::vector<EggState5> EggGenerator5::generate(u64 seed) const
{
    switch (profile.getVersion())
    {
    case Game::Black:
    case Game::White:
        return generateBW(seed);
    case Game::Black2:
    case Game::White2:
        return generateBW2(seed);
    default:
        return std::vector<EggState5>();
    }
}

std::vector<EggState5> EggGenerator5::generateBW(u64 seed) const
{
    const PersonalInfo *base = PersonalLoader::getPersonal(profile.getVersion(), daycare.getEggSpecie());
    const PersonalInfo *male;
    const PersonalInfo *female;
    if (daycare.getEggSpecie() == 29 || daycare.getEggSpecie() == 32)
    {
        male = PersonalLoader::getPersonal(profile.getVersion(), 32);
        female = PersonalLoader::getPersonal(profile.getVersion(), 29);
    }
    else if (daycare.getEggSpecie() == 313 || daycare.getEggSpecie() == 314)
    {
        male = PersonalLoader::getPersonal(profile.getVersion(), 313);
        female = PersonalLoader::getPersonal(profile.getVersion(), 314);
    }

    MTFast<13, true> mt(seed >> 32, 7);
    std::array<u8, 6> mtIVs;
    std::generate(mtIVs.begin(), mtIVs.end(), [&mt] { return mt.next(); });

    u32 advances = Utilities5::initialAdvances(seed, profile);
    BWRNG rng(seed, advances + initialAdvances);
    auto jump = rng.getJump(offset);

    std::vector<EggState5> states;
    for (u32 cnt = 0; cnt <= maxAdvances; cnt++)
    {
        BWRNG go(rng, jump);

        // Nidoran
        // Volbeat / Illumise
        const PersonalInfo *info = base;
        if (daycare.getEggSpecie() == 29 || daycare.getEggSpecie() == 32)
        {
            info = go.nextUInt(2) ? male : female;
        }
        else if (daycare.getEggSpecie() == 313 || daycare.getEggSpecie() == 314)
        {
            info = go.nextUInt(2) ? female : male;
        }

        u8 nature = go.nextUInt(25);
        // Everstone, first check for presence of item and proc
        if (everstone != 0 && go.nextUInt(2))
        {
            if (everstone == 2)
            {
                // 0->parent1 / 1->parent2
                nature = daycare.getParentNature(go.nextUInt(2));
            }
            else
            {
                u8 parent = daycare.getParentItem(0) == 1 ? 0 : 1;
                nature = daycare.getParentNature(parent);
            }
        }

        // Reroll ability to remove HA
        bool hiddenAbility = false;
        if (ditto)
        {
            go.advance(2);
        }
        else
        {
            hiddenAbility = go.nextUInt(100) >= 40 && parentAbility == 2;
        }

        // Power Items
        u8 inheritanceCount = 0;
        std::array<u8, 6> ivs = mtIVs;
        std::array<u8, 6> inheritance = { 0, 0, 0, 0, 0, 0 };
        if (poweritem != 0)
        {
            inheritanceCount = 1;
            if (poweritem == 2)
            {
                u8 parent = go.nextUInt(2);
                u8 item = daycare.getParentItem(parent);

                ivs[item - 2] = daycare.getParentIV(parent, item - 2);
                inheritance[item - 2] = parent + 1;
            }
            else
            {
                u8 parent = (daycare.getParentItem(0) >= 2 && daycare.getParentItem(0) <= 7) ? 0 : 1;
                u8 item = daycare.getParentItem(parent);

                ivs[item - 2] = daycare.getParentIV(parent, item - 2);
                inheritance[item - 2] = parent + 1;
            }
        }

        // IV Inheritance
        for (; inheritanceCount < 3;)
        {
            u8 index = go.nextUInt(6);
            u8 parent = go.nextUInt(2);

            // Assign stat inheritance
            if (inheritance[index] == 0)
            {
                inheritanceCount++;
                ivs[index] = daycare.getParentIV(parent, index);
                inheritance[index] = parent + 1;
            }
        }

        u32 pid = go.nextUInt(0xffffffff);
        for (u8 i = 0; i < rolls && !Utilities::isShiny<true>(pid, tsv); i++)
        {
            pid = go.nextUInt(0xffffffff);
        }

        u8 ability = hiddenAbility ? 2 : ((pid >> 16) & 1);

        EggState5 state(rng.nextUInt(0x1fff), advances + initialAdvances + cnt, pid, ivs, ability, Utilities::getGender(pid, info), nature,
                        Utilities::getShiny<true>(pid, tsv), inheritance, info);
        if (filter.compareState(static_cast<const State &>(state)))
        {
            states.emplace_back(state);
        }
    }

    return states;
}

std::vector<EggState5> EggGenerator5::generateBW2(u64 seed) const
{
    std::vector<EggState5> states;

    MTFast<4> mt(seed >> 32, 2);

    u64 eggSeed = static_cast<u64>(mt.next()) << 32;
    eggSeed |= mt.next();

    const PersonalInfo *info = nullptr;
    EggState5 state = generateBW2Egg(eggSeed, &info);
    if (filter.compareAbility(state.getAbility()) && filter.compareNature(state.getNature()) && filter.compareIV(state.getIVs())
        && filter.compareHiddenPower(state.getHiddenPower()))
    {
        u32 advances = Utilities5::initialAdvances(seed, profile);
        BWRNG rng(seed, advances + initialAdvances);
        auto jump = rng.getJump(offset);

        for (u32 cnt = 0; cnt <= maxAdvances; cnt++)
        {
            BWRNG go(rng, jump);

            u32 pid = go.nextUInt();
            if (((pid >> 16) & 1) != state.getAbility())
            {
                pid ^= 0x10000;
            }

            for (u8 i = 0; i < rolls && !Utilities::isShiny<true>(pid, tsv); i++)
            {
                pid = go.nextUInt();
                if (((pid >> 16) & 1) != state.getAbility())
                {
                    pid ^= 0x10000;
                }
            }

            state.update(rng.nextUInt(0x1fff), advances + initialAdvances + cnt, pid, Utilities::getGender(pid, info),
                         Utilities::getShiny<true>(pid, tsv));
            if (filter.compareGender(state.getGender()) && filter.compareShiny(state.getShiny()))
            {
                states.emplace_back(state);
            }
        }
    }

    return states;
}

EggState5 EggGenerator5::generateBW2Egg(u64 seed, const PersonalInfo **info) const
{
    BWRNG rng(seed);

    // Nidoran
    // Volbeat / Illumise
    if (daycare.getEggSpecie() == 29 || daycare.getEggSpecie() == 32)
    {
        if (rng.nextUInt(2))
        {
            *info = PersonalLoader::getPersonal(profile.getVersion(), 32);
        }
        else
        {
            *info = PersonalLoader::getPersonal(profile.getVersion(), 29);
        }
    }
    else if (daycare.getEggSpecie() == 313 || daycare.getEggSpecie() == 314)
    {
        if (rng.nextUInt(2))
        {
            *info = PersonalLoader::getPersonal(profile.getVersion(), 314);
        }
        else
        {
            *info = PersonalLoader::getPersonal(profile.getVersion(), 313);
        }
    }
    else
    {
        *info = PersonalLoader::getPersonal(profile.getVersion(), daycare.getEggSpecie());
    }

    u8 nature = rng.nextUInt(25);
    // Everstone
    if (everstone == 2)
    {
        // 0->parent1 / 1->parent2
        nature = daycare.getParentNature(rng.nextUInt(2));
    }
    else if (everstone == 1)
    {
        u8 parent = daycare.getParentItem(0) == 1 ? 0 : 1;
        nature = daycare.getParentNature(parent);
    }

    u8 ability;
    if (!ditto)
    {
        u8 rand = rng.nextUInt(100);
        if (parentAbility == 0)
        {
            ability = rand < 80 ? 0 : 1;
        }
        else if (parentAbility == 1)
        {
            ability = rand < 20 ? 0 : 1;
        }
        else
        {
            ability = rand < 20 ? 0 : rand < 40 ? 1 : 2;
        }
    }
    else
    {
        rng.advance(1);
        ability = rng.nextUInt(2);
    }

    // Power Items
    u8 inheritanceCount = 0;
    std::array<u8, 6> inheritance = { 0, 0, 0, 0, 0, 0 };
    std::array<u8, 6> ivs;
    if (poweritem != 0)
    {
        inheritanceCount = 1;
        if (poweritem == 2)
        {
            u8 parent = rng.nextUInt(2);
            u8 item = daycare.getParentItem(parent);

            ivs[item - 2] = daycare.getParentIV(parent, item - 2);
            inheritance[item - 2] = parent + 1;
        }
        else
        {
            u8 parent = (daycare.getParentItem(0) >= 2 && daycare.getParentItem(0) <= 7) ? 0 : 1;
            u8 item = daycare.getParentItem(parent);

            ivs[item - 2] = daycare.getParentIV(parent, item - 2);
            inheritance[item - 2] = parent + 1;
        }
    }

    // IV Inheritance
    for (; inheritanceCount < 3;)
    {
        u8 index = rng.nextUInt(6);
        u8 parent = rng.nextUInt(2);

        if (inheritance[index] == 0)
        {
            ivs[index] = daycare.getParentIV(parent, index);
            inheritance[index] = parent + 1;
            inheritanceCount++;
        }
    }

    for (u8 i = 0; i < 6; i++)
    {
        if (inheritance[i] == 0)
        {
            ivs[i] = rng.nextUInt(32);
        }
    }

    return EggState5(ivs, ability, nature, inheritance, *info);
}
