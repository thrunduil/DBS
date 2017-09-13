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

#include "test_dbs.h"
#include "dbs/dbs.h"
#include "timer.h"
#include "rand.h"

#include <set>
#include <iostream>
#include <algorithm>

#pragma warning(disable :4146)  // unary minus operator applied to unsigned type, result still unsigned

namespace dbs_lib { namespace testing
{

bool test_dbs::test_all(size_t n_rep)
{
    bool ret        = true;               

    ret             &= test_set_all(n_rep);    
    ret             &= test_constructor_all(n_rep);    
    ret             &= test_find_all(n_rep);
    ret             &= test_reset_all(n_rep);    
    ret             &= test_not_all(n_rep); 

    ret             &= test_eq_all(n_rep);
    ret             &= test_neq_all(n_rep);

    ret             &= test_and_all(n_rep);
    ret             &= test_or_all(n_rep);
    ret             &= test_xor_all(n_rep);

    return ret;
};

void test_dbs::test_perf_all(size_t n_rep, bool& ret)
{
    {
        size_t max_elem = 64*32;
        double t1       = test_perf_find_set(max_elem, n_rep, ret);    
        double t2       = test_perf_find_dbs(max_elem, n_rep, ret);    

        std::cout << "find - " << max_elem << ": set " << t1 << ", dbs " << t2 << ", ratio " << t1/t2 << "\n";
    };

    {
        size_t max_elem = 64*32*32*32*32;
        double t1       = test_perf_find_set(max_elem, n_rep, ret);    
        double t2       = test_perf_find_dbs(max_elem, n_rep, ret);    

        std::cout << "find - " << max_elem << ": set " << t1 << ", dbs " << t2 << ", ratio " << t1/t2 << "\n";
    };

    {
        size_t max_elem = -size_t(1);
        double t1       = test_perf_find_set(max_elem, n_rep, ret);    
        double t2       = test_perf_find_dbs(max_elem, n_rep, ret);    

        std::cout << "find - " << max_elem << ": set " << t1 << ", dbs " << t2 << ", ratio " << t1/t2 << "\n";
    };
};

double test_dbs::test_perf_find_set(size_t max_elem, size_t n_rep, bool& ret)
{
    double t = 0.;

    ret |= test_perf_find_set(max_elem, 10, n_rep, t);
    ret |= test_perf_find_set(max_elem, 1000, n_rep, t);
    ret |= test_perf_find_set(max_elem, 100000, n_rep, t);
    
    return t;
};    

double test_dbs::test_perf_find_dbs(size_t max_elem, size_t n_rep, bool& ret)
{
    double t = 0.;

    ret &= test_perf_find_dbs(max_elem, 10, n_rep, t);
    ret &= test_perf_find_dbs(max_elem, 1000, n_rep, t);
    ret &= test_perf_find_dbs(max_elem, 100000, n_rep, t);
    
    return t;
}; 

bool test_dbs::test_perf_find_set(size_t max_elem, size_t n_items, size_t n_rep, double& t)
{
    std::set<size_t> s = this->rand_set(max_elem, n_items);

    std::vector<size_t> v;
    v.reserve(n_rep);

    double tol = 0.5;

    for(size_t i = 0; i < n_rep; ++i)
    {
        size_t elem = rand_elem(max_elem);

        if (testing::genrand_real1() < tol)
        {
            v.push_back(elem);
        }
        else
        {
            v.push_back(elem);
            s.insert(elem);
        };
    };

    bool val = false;
    tic();

    for(size_t i = 0; i < n_rep; ++i)
        val |= (s.find(v[i]) != s.end());

    t += toc();

    return val;
};  

bool test_dbs::test_perf_find_dbs(size_t max_elem, size_t n_items, size_t n_rep, double& t)
{
    std::set<size_t> s      = this->rand_set(max_elem, n_items);
    std::vector<size_t> sv  = to_vector(s);

    dbs bs(sv.size(), sv.data());

    std::vector<size_t> v;
    v.reserve(n_rep);

    double tol = 0.5;

    for(size_t i = 0; i < n_rep; ++i)
    {
        size_t elem = rand_elem(max_elem);

        if (genrand_real1() < tol)
        {
            v.push_back(elem);
        }
        else
        {
            v.push_back(elem);
            bs = bs.set(elem);
        };
    };

    tic();

    bool val = false;
    
    for(size_t i = 0; i < n_rep; ++i)
        val |= bs.test(v[i]);

    t += toc();

    return val;
}; 

bool test_dbs::test_set_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_set(64*32, 10);
        ret         &= test_set(64*32, 100);
        //ret       &= test_set(64*32, 1000);

        ret         &= test_set(64*32*32*32*32, 10);
        ret         &= test_set(64*32*32*32*32, 100);
        //ret       &= test_set(64*32*32*32*32, 1000);

        ret         &= test_set(-size_t(1), 10);
        ret         &= test_set(-size_t(1), 100);
        //ret       &= test_set(-size_t(1), 1000);
    };

    std::cout << "test_set: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};
bool test_dbs::test_constructor_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_constructor(64*32, 10);
        ret         &= test_constructor(64*32, 100);
        ret         &= test_constructor(64*32, 1000);

        ret         &= test_constructor(64*32*32*32*32, 10);
        ret         &= test_constructor(64*32*32*32*32, 100);
        ret         &= test_constructor(64*32*32*32*32, 1000);

        ret         &= test_constructor(-size_t(1), 10);
        ret         &= test_constructor(-size_t(1), 100);
        ret         &= test_constructor(-size_t(1), 1000);
    };

    std::cout << "test_constructor: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_find_all(size_t n_rep)
{
    bool ret    = true;

    size_t n_search = 100;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_find(64*32, 10, n_search);
        ret         &= test_find(64*32, 100, n_search);
        ret         &= test_find(64*32, 1000, n_search);

        ret         &= test_find(64*32*32*32*32, 10, n_search);
        ret         &= test_find(64*32*32*32*32, 100, n_search);
        ret         &= test_find(64*32*32*32*32, 1000, n_search);

        ret         &= test_find(-size_t(1), 10, n_search);
        ret         &= test_find(-size_t(1), 100, n_search);
        ret         &= test_find(-size_t(1), 1000, n_search);
    };

    std::cout << "test_find: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_reset_all(size_t n_rep)
{
    bool ret    = true;

    size_t n_search = 100;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_reset(64*32, 10, n_search);
        ret         &= test_reset(64*32, 100, n_search);
        ret         &= test_reset(64*32, 1000, n_search);

        ret         &= test_reset(64*32*32*32*32, 10, n_search);
        ret         &= test_reset(64*32*32*32*32, 100, n_search);
        ret         &= test_reset(64*32*32*32*32, 1000, n_search);

        ret         &= test_reset(-size_t(1), 10, n_search);
        ret         &= test_reset(-size_t(1), 100, n_search);
        ret         &= test_reset(-size_t(1), 1000, n_search);
    };

    std::cout << "test_reset: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_not_all(size_t n_rep)
{
    bool ret    = true;

    size_t n_search = 100;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_not(64*32, 10, n_search);
        ret         &= test_not(64*32, 100, n_search);
        ret         &= test_not(64*32, 1000, n_search);

        ret         &= test_not(64*32*32*32*32, 10, n_search);
        ret         &= test_not(64*32*32*32*32, 100, n_search);
        ret         &= test_not(64*32*32*32*32, 1000, n_search);

        ret         &= test_not(-size_t(1), 10, n_search);
        ret         &= test_not(-size_t(1), 100, n_search);
        ret         &= test_not(-size_t(1), 1000, n_search);
    };

    std::cout << "test_not: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_eq_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_eq(64*32, 10);
        ret         &= test_eq(64*32, 100);
        ret         &= test_eq(64*32, 1000);

        ret         &= test_eq(64*32*32*32*32, 10);
        ret         &= test_eq(64*32*32*32*32, 100);
        ret         &= test_eq(64*32*32*32*32, 1000);

        ret         &= test_eq(-size_t(1), 10);
        ret         &= test_eq(-size_t(1), 100);
        ret         &= test_eq(-size_t(1), 1000);
    };

    std::cout << "test_eq: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_neq_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_eq(64*32, 10);
        ret         &= test_eq(64*32, 100);
        ret         &= test_eq(64*32, 1000);

        ret         &= test_eq(64*32*32*32*32, 10);
        ret         &= test_eq(64*32*32*32*32, 100);
        ret         &= test_eq(64*32*32*32*32, 1000);

        ret         &= test_eq(-size_t(1), 10);
        ret         &= test_eq(-size_t(1), 100);
        ret         &= test_eq(-size_t(1), 1000);
    };

    std::cout << "test_neq: " << (ret? "OK" : "FAILED") << "\n";
    return ret;

};
bool test_dbs::test_and_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_and(64*32, 10);
        ret         &= test_and(64*32, 100);
        ret         &= test_and(64*32, 1000);

        ret         &= test_and(64*32*32*32*32, 10);
        ret         &= test_and(64*32*32*32*32, 100);
        ret         &= test_and(64*32*32*32*32, 1000);

        ret         &= test_and(-size_t(1), 10);
        ret         &= test_and(-size_t(1), 100);
        ret         &= test_and(-size_t(1), 1000);
    };

    std::cout << "test_and: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_or_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_or(64*32, 10);
        ret         &= test_or(64*32, 100);
        ret         &= test_or(64*32, 1000);

        ret         &= test_or(64*32*32*32*32, 10);
        ret         &= test_or(64*32*32*32*32, 100);
        ret         &= test_or(64*32*32*32*32, 1000);

        ret         &= test_or(-size_t(1), 10);
        ret         &= test_or(-size_t(1), 100);
        ret         &= test_or(-size_t(1), 1000);
    };

    std::cout << "test_or: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_xor_all(size_t n_rep)
{
    bool ret    = true;

    for (size_t i = 0; i < n_rep; ++i)
    {
        ret         &= test_xor(64*32, 10);
        ret         &= test_xor(64*32, 100);
        ret         &= test_xor(64*32, 1000);

        ret         &= test_xor(64*32*32*32*32, 10);
        ret         &= test_xor(64*32*32*32*32, 100);
        ret         &= test_xor(64*32*32*32*32, 1000);

        ret         &= test_xor(-size_t(1), 10);
        ret         &= test_xor(-size_t(1), 100);
        ret         &= test_xor(-size_t(1), 1000);
    };

    std::cout << "test_xor: " << (ret? "OK" : "FAILED") << "\n";
    return ret;
};

bool test_dbs::test_set(size_t max_elem, size_t n_items)
{        
    dbs bs;

    std::vector<size_t> dbs_elems;
    std::set<size_t>    true_elems;

    bool ret = true;

    for (size_t i = 0; i < n_items; ++i)
    {
        dbs_elems.clear();

        size_t t    = static_cast<size_t>(genrand_int32());
        t           = t % max_elem;

        true_elems.insert(t);

        bs          = bs.set(t);
        bs.get_elements(dbs_elems);            

        auto it1    = true_elems.begin();
        auto it2    = dbs_elems.begin();

        int pos     = 0;
        for (; it1 != true_elems.end() && it2 != dbs_elems.end(); ++it1, ++it2, ++pos)
        {
            if (*it1 != *it2)
            {
                ret = false;
                //std::cout << pos << " " << *it1 << " " << *it2 << "\n";
            };
        };
    };

    return ret;
};

bool test_dbs::test_constructor(size_t max_elem, size_t n_items)
{        
    std::set<size_t> s          = rand_set(max_elem, n_items);
    std::vector<size_t> v1      = to_vector(s);
    std::vector<size_t> v2;

    dbs bs(v1.size(), v1.data());
    bs.get_elements(v2);

    auto it1    = v1.begin();
    auto it2    = v2.begin();

    bool ret    = true;

    for (; it1 != v1.end() && it2 != v2.end(); ++it1, ++it2)
    {
        if (*it1 != *it2)
            ret = false;
    };

    return ret;
};

bool test_dbs::test_find(size_t max_elem, size_t n_items, size_t n_search)
{        
    std::set<size_t> s          = rand_set(max_elem, n_items);
    std::vector<size_t> vec     = to_vector(s);

    dbs bs(vec.size(), vec.data());

    bool ret    = true;

    for(size_t i = 0; i < n_search; ++i)
    {
        bool exist = (genrand_real1() < 0.5);

        if (exist == true)
        {
            size_t elem     = this->rand_elem(max_elem);
            s.insert(elem);
            bs = bs.set(elem);

            bool val        = bs.test(elem);
            
            if (val == false)
                ret         = false;
        }
        else
        {
            size_t elem     = this->rand_elem(max_elem);
            bool v1         = (s.find(elem) != s.end());
            bool v2         = bs.test(elem);

            if (v1 != v2)
                ret         = false;
        };
    };

    return ret;
};

bool test_dbs::test_reset(size_t max_elem, size_t n_items, size_t n_search)
{        
    std::set<size_t> s          = rand_set(max_elem, n_items);
    std::vector<size_t> v1      = to_vector(s);

    dbs bs(v1.size(), v1.data());

    bool ret    = true;

    for(size_t i = 0; i < n_search; ++i)
    {
        bool exist = (genrand_real1() < 0.5);

        if (exist == true)
        {
            size_t elem     = this->rand_elem(max_elem);
            s.erase(elem);

            bs = bs.set(elem);
            bs = bs.reset(elem);

            bool val        = bs.test(elem);
            
            if (val == true)
                ret         = false;
        }
        else
        {
            size_t elem     = this->rand_elem(max_elem);
            s.erase(elem);

            bs              = bs.reset(elem);
            bool v2         = bs.test(elem);

            if (v2 == true)
                ret         = false;
        };
    };

    for(auto it = s.begin(); it != s.end(); ++it)
        bs = bs.reset(*it);

    if (bs.size() != 0)
        ret = false;

    return ret;
};

bool test_dbs::test_not(size_t max_elem, size_t n_items, size_t n_search)
{        
    std::set<size_t> s          = rand_set(max_elem, n_items);
    std::vector<size_t> vec     = to_vector(s);

    dbs bs(vec.size(), vec.data());

    bool ret    = true;

    for(size_t i = 0; i < n_search; ++i)
    {
        bool exist = (genrand_real1() < 0.5);

        if (exist == true)
        {
            size_t elem     = this->rand_elem(max_elem);
            s.erase(elem);

            bs = bs.set(elem);
            bs = bs.flip(elem);

            bool val        = bs.test(elem);
            
            if (val == true)
                ret         = false;
        }
        else
        {
            size_t elem     = this->rand_elem(max_elem);
            bool v1         = s.find(elem) != s.end();

            if (v1)
                s.erase(elem);
            else
                s.insert(elem);

            bs              = bs.flip(elem);
            bool v2         = bs.test(elem);
            v1              = !v1;

            if (v2 != v1)
                ret         = false;
        };
    };

    for(auto it = s.begin(); it != s.end(); ++it)
        bs = bs.reset(*it);

    if (bs.size() != 0)
        ret = false;

    return ret;
};

bool test_dbs::test_eq(size_t max_elem, size_t n_item)
{        
    std::set<size_t> s          = rand_set(max_elem, n_item);
    std::vector<size_t> vec     = to_vector(s);

    dbs bs(vec.size(), vec.data());

    bool exist = (genrand_real1() < 0.5);

    if (exist == true)
    {
        bool ret        = (bs == bs);

        if (ret == false)
            return false;
        else
            return true;
    }
    else
    {
        size_t elem     = this->rand_elem(max_elem);
        bool v1         = s.find(elem) != s.end();

        s.insert(elem);

        dbs bs2         = bs.set(elem);
        bool v2         = (bs == bs2);

        if (v2 != v1)
            return false;
        else
            return true;
    };
};

bool test_dbs::test_neq(size_t max_elem, size_t n_item)
{        
    std::set<size_t> s          = rand_set(max_elem, n_item);
    std::vector<size_t> vec     = to_vector(s);

    dbs bs(vec.size(), vec.data());

    bool exist = (genrand_real1() < 0.5);

    if (exist == true)
    {
        bool ret        = ((bs != bs) == false);

        if (ret == false)
            return false;
        else
            return true;
    }
    else
    {
        size_t elem     = this->rand_elem(max_elem);
        bool v1         = s.find(elem) != s.end();

        s.insert(elem);

        dbs bs2         = bs.set(elem);
        bool v2         = (bs != bs2);
        v1              = !v1;

        if (v2 != v1)
            return false;
        else
            return true;
    };
};
bool test_dbs::test_and(size_t max_elem, size_t n_item)
{        
    std::set<size_t> s1         = rand_set(max_elem, n_item);
    std::set<size_t> s2         = rand_set(max_elem, n_item);

    size_t common               = this->rand_elem(max_elem);
    s1.insert(common);
    s2.insert(common);

    std::vector<size_t> v3;
    v3.resize(s1.size() + s2.size());
    auto it = std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), v3.begin());
    v3.resize(it-v3.begin());

    std::vector<size_t> v1      = to_vector(s1);
    std::vector<size_t> v2      = to_vector(s2);

    dbs bs1(v1.size(), v1.data());
    dbs bs2(v2.size(), v2.data());
    dbs bs3 = bs1 & bs2;

    std::vector<size_t> vret;
    bs3.get_elements(vret);

    auto it1 = v3.begin();
    auto it2 = vret.begin();

    bool ret = true;
    for (; it1 != v3.end() && it2 != vret.end(); ++it1, ++it2)
    {
        if (*it1 != *it2)
            ret = false;
    };

    return ret;
};

bool test_dbs::test_or(size_t max_elem, size_t n_item)
{        
    std::set<size_t> s1         = rand_set(max_elem, n_item);
    std::set<size_t> s2         = rand_set(max_elem, n_item);

    std::vector<size_t> v3;
    v3.resize(s1.size() + s2.size());
    auto it = std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), v3.begin());
    v3.resize(it-v3.begin());

    std::vector<size_t> v1      = to_vector(s1);
    std::vector<size_t> v2      = to_vector(s2);

    dbs bs1(v1.size(), v1.data());
    dbs bs2(v2.size(), v2.data());
    dbs bs3 = bs1 | bs2;

    std::vector<size_t> vret;
    bs3.get_elements(vret);

    auto it1 = v3.begin();
    auto it2 = vret.begin();

    bool ret = true;
    for (; it1 != v3.end() && it2 != vret.end(); ++it1, ++it2)
    {
        if (*it1 != *it2)
            ret = false;
    };

    return ret;
};

bool test_dbs::test_xor(size_t max_elem, size_t n_item)
{        
    std::set<size_t> s1         = rand_set(max_elem, n_item);
    std::set<size_t> s2         = rand_set(max_elem, n_item);

    size_t common               = this->rand_elem(max_elem);
    s1.insert(common);
    s2.insert(common);

    std::vector<size_t> v3;
    v3.resize(s1.size() + s2.size());
    auto it = std::set_symmetric_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), v3.begin());
    v3.resize(it-v3.begin());

    std::vector<size_t> v1      = to_vector(s1);
    std::vector<size_t> v2      = to_vector(s2);

    dbs bs1(v1.size(), v1.data());
    dbs bs2(v2.size(), v2.data());
    dbs bs3 = bs1 ^ bs2;

    std::vector<size_t> vret;
    bs3.get_elements(vret);

    auto it1 = v3.begin();
    auto it2 = vret.begin();

    bool ret = true;
    for (; it1 != v3.end() && it2 != vret.end(); ++it1, ++it2)
    {
        if (*it1 != *it2)
        {
            std::cout << "error\n";
            dbs bs4 = bs1 ^ bs2;
            ret = false;
        };
    };

    return ret;
};

std::set<size_t> test_dbs::rand_set(size_t max_elem, size_t n_items)
{
    std::set<size_t> ret;
    for (size_t i = 0; i < n_items; ++i)
        ret.insert(rand_elem(max_elem));

    return ret;
};

size_t test_dbs::rand_elem(size_t max_elem)
{
    size_t t    = static_cast<size_t>(genrand_int32());
    t           = t % max_elem;

    return t;
};

std::vector<size_t> test_dbs::to_vector(const std::set<size_t>& s)
{
    std::vector<size_t> ret;
    ret.reserve(s.size());

    for(auto it = s.begin(); it != s.end(); ++it)
    {
        ret.push_back(*it);
    };

    return ret;
};

void test_dbs::test_pert_set(size_t max_elem, size_t n_items, size_t n_rep)
{    
    //size_t max_elem = 64*32*32*32*32;

    for (size_t j = 0; j < n_rep; ++ j)
    {
        dbs bs;

        for (size_t i = 0; i < n_items; ++i)
        {

            size_t t    = rand_elem(max_elem);
            bs          = bs.set(t);
        };
    };
};

}};