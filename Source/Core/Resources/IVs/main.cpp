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

#include "MTFast.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <mutex>

class IVTableSearcher
{
public:
    IVTableSearcher()
    {
        std::generate(entralinkSeeds.begin(), entralinkSeeds.end(), [] { return new u32[16384]; });
        std::generate(normalSeeds.begin(), normalSeeds.end(), [] { return new u32[16384]; });
        std::generate(roamerSeeds.begin(), roamerSeeds.end(), [] { return new u32[8192]; });

        entralinkSizes.fill(0);
        normalSizes.fill(0);
        roamerSizes.fill(0);
    }

    ~IVTableSearcher()
    {
        for (auto &seeds : entralinkSeeds)
        {
            delete[] seeds;
        }

        for (auto &seeds : normalSeeds)
        {
            delete[] seeds;
        }

        for (auto &seeds : roamerSeeds)
        {
            delete[] seeds;
        }
    }

    void startSearch(int threads)
    {
        auto *threadContainer = new std::thread[threads];

        u32 split = 0x100000000 / threads;
        u32 start = 0;

        for (int i = 0; i < threads; i++)
        {
            if (i == threads - 1)
            {
                threadContainer[i] = std::thread([=] { search(start, 0xffffffff); });
            }
            else
            {
                threadContainer[i] = std::thread([=] { search(start, start + split); });
            }
            start += split;
        }

        for (int i = 0; i < threads; i++)
        {
            threadContainer[i].join();
        }

        for (int i = 0; i < entralinkSeeds.size(); i++)
        {
            std::sort(&entralinkSeeds[i][0], &entralinkSeeds[i][entralinkSizes[i]]);
        }

        for (int i = 0; i < normalSeeds.size(); i++)
        {
            std::sort(&normalSeeds[i][0], &normalSeeds[i][normalSizes[i]]);
        }

        for (int i = 0; i < roamerSeeds.size(); i++)
        {
            std::sort(&roamerSeeds[i][0], &roamerSeeds[i][roamerSizes[i]]);
        }

        // Save results
        std::ofstream file("ivs.bin", std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        if (file.is_open())
        {
            file.write(reinterpret_cast<const char *>(entralinkSizes.data()), sizeof(entralinkSizes));
            file.write(reinterpret_cast<const char *>(normalSizes.data()), sizeof(normalSizes));
            file.write(reinterpret_cast<const char *>(roamerSizes.data()), sizeof(roamerSizes));

            for (int i = 0; i < entralinkSeeds.size(); i++)
            {
                file.write(reinterpret_cast<const char *>(entralinkSeeds[i]), sizeof(u32) * entralinkSizes[i]);
            }

            for (int i = 0; i < normalSeeds.size(); i++)
            {
                file.write(reinterpret_cast<const char *>(normalSeeds[i]), sizeof(u32) * normalSizes[i]);
            }

            for (int i = 0; i < roamerSeeds.size(); i++)
            {
                file.write(reinterpret_cast<const char *>(roamerSeeds[i]), sizeof(u32) * roamerSizes[i]);
            }

            file.close();
        }

        delete[] threadContainer;
    }

private:
    std::mutex mutex;
    std::array<u32 *, 10> entralinkSeeds;
    std::array<u32 *, 8> normalSeeds;
    std::array<u32 *, 6> roamerSeeds;
    std::array<u16, 10> entralinkSizes;
    std::array<u16, 8> normalSizes;
    std::array<u16, 6> roamerSizes;

    void search(u32 start, u32 end)
    {
        for (u32 seed = start;; seed++)
        {
            MTFast<37, true> mt(seed);

            std::array<u8, 37> ivs;
            std::generate(ivs.begin(), ivs.end(), [&mt] { return mt.next(); });

            for (u8 i = 0; i < 10; i++)
            {
                // Entralink
                u8 hp = ivs[i + 22];
                u8 atk = ivs[i + 23];
                u8 def = ivs[i + 24];
                u8 spa = ivs[i + 25];
                u8 spd = ivs[i + 26];
                u8 spe = ivs[i + 27];
                if (hp >= 30 && def >= 30 && spd >= 30 && (atk >= 30 || spa >= 30) && (spe <= 1 || spe >= 30))
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    entralinkSeeds[i][entralinkSizes[i]++] = seed;
                }

                // Normal
                if (i < 8)
                {
                    hp = ivs[i];
                    atk = ivs[i + 1];
                    def = ivs[i + 2];
                    spa = ivs[i + 3];
                    spd = ivs[i + 4];
                    spe = ivs[i + 5];

                    if (hp >= 30 && def >= 30 && spd >= 30 && (atk >= 30 || spa >= 30) && (spe <= 1 || spe >= 30))
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        normalSeeds[i][normalSizes[i]++] = seed;
                    }
                }

                // Roamer
                if (i < 6)
                {
                    hp = ivs[i + 1];
                    atk = ivs[i + 2];
                    def = ivs[i + 3];
                    spa = ivs[i + 6];
                    spd = ivs[i + 4];
                    spe = ivs[i + 5];

                    if (hp >= 30 && def >= 30 && spd >= 30 && (atk >= 30 || spa >= 30) && spe >= 30)
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        roamerSeeds[i][roamerSizes[i]++] = seed;
                    }
                }
            }

            if (seed == end)
            {
                break;
            }
        }
    }
};

int main()
{
    IVTableSearcher searcher;
    searcher.startSearch(4);
}
