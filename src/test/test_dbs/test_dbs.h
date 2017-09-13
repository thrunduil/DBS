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

#pragma once

#include <set>
#include <vector>

namespace dbs_lib { namespace testing
{

struct test_dbs
{
    public:
        bool                test_set(size_t max_elem, size_t n_items);
        bool                test_constructor(size_t max_elem, size_t n_items);
        bool                test_find(size_t max_elem, size_t n_items, size_t n_search);
        bool                test_reset(size_t max_elem, size_t n_items, size_t n_search);
        bool                test_not(size_t max_elem, size_t n_items, size_t n_search);

        bool                test_eq(size_t max_elem, size_t n_items);
        bool                test_neq(size_t max_elem, size_t n_items);

        bool                test_and(size_t max_elem, size_t n_items);
        bool                test_or(size_t max_elem, size_t n_items);
        bool                test_xor(size_t max_elem, size_t n_items);        

        bool                test_set_all(size_t n_rep);
        bool                test_constructor_all(size_t n_rep);
        bool                test_find_all(size_t n_rep);
        bool                test_reset_all(size_t n_rep);
        bool                test_not_all(size_t n_rep);

        bool                test_eq_all(size_t n_rep);
        bool                test_neq_all(size_t n_rep);

        bool                test_and_all(size_t n_rep);
        bool                test_or_all(size_t n_rep);
        bool                test_xor_all(size_t n_rep);

        bool                test_perf_find_set(size_t max_elem, size_t n_items, size_t n_rep, double& t);    
        bool                test_perf_find_dbs(size_t max_elem, size_t n_items, size_t n_rep, double& t); 

        double              test_perf_find_set(size_t max_elem, size_t n_rep, bool& ret);    
        double              test_perf_find_dbs(size_t max_elem, size_t n_rep, bool& ret); 

        bool                test_all(size_t n_rep);
        void                test_perf_all(size_t n_rep, bool& ret);
        void                test_pert_set(size_t max_elem, size_t n_items, size_t n_rep);

    private:
        std::set<size_t>    rand_set(size_t max_elem, size_t n_items);
        std::vector<size_t> to_vector(const std::set<size_t>& );
        size_t              rand_elem(size_t max_elem);
};

}}