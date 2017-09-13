/* 
 *  This file is a part of DBS library.
 *
 *  Copyright (c) Pawe³ Kowal 2017
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "dbs/dbs.h"
#include "test_dbs.h"
#include "timer.h"

#include <vector>
#include <iostream>

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    {
        using namespace dbs_lib;

        dbs s1{1, size_t(-1)};
        dbs s2{2, size_t(-1)};

        dbs s3  = s1 | s2;
        dbs s4  = s3.set(55);

        std::cout << s3 << "\n";
        std::cout << s4 << "\n";
    };
    {
        dbs_lib::dbs f;
        for (size_t i = 63; i < 1000; ++i)
            f = f.set(i);
    }
    try
    {
        dbs_lib::testing::test_dbs tb;

        {
            size_t max_elem =  64*32*32*32*32;
            size_t n_elem   = 100;
            size_t n_rep    = 10000;

            dbs_lib::testing::tic();
            tb.test_pert_set(max_elem, n_elem, n_rep);
            double t = dbs_lib::testing::toc();

            std::cout << t << "\n";
        };

        bool ret = false;
        tb.test_perf_all(100000, ret);

        std::cout << ret << "\n";

        bool res = tb.test_all(1000);

        if (res == false)
            std::cout << "dbs failed\n";
        else
            std::cout << "dbs ok\n";
    }
    catch(std::exception& ex)
    {
        std::cout << ex.what();
    }
    catch(...)
    {
        std::cout << "unknown exception\n";
    }    

    return 0;
}