# DBS

DBS library implements a dynamic bitset, that can store unlimited number
of bits (restricted to number of values represented by size_t type). 

This library offers similar functionality to std :: bitset 
or boost :: dynamic_bitset, but is based on different design. Internally dbs is 
implemented as tree like structure, which allows for 
representing sparse bitsets with low memory overhead.

Bitset once created cannot be modified later, any operation on a bitset
creates an independent copy. However only modified blocks are copied.

## Use case

DBS library is intended to work with sparse bitsets, possibly large, in the functional
style, that is bitset operations should not destroy existing bitsets.

## Code example

Simple bitset operations:
```cpp
        using namespace dbs_lib;

        dbs s1{1, size_t(-1)};
        dbs s2{2, size_t(-1)};

        dbs s3  = s1 | s2;
        dbs s4  = s3.set(55);

        std::cout << s3 << "\n";
        std::cout << s4 << "\n";

        output:
        {1, 2, 18446744073709551615}
        {1, 2, 55, 18446744073709551615}
```            

## Status

DBS library is in release candidate stage. However tests were only
performed on Windows using Visual Studio 2015 compiler.

## Licence

Copyright (C) 2017  Paweł Kowal

This library is published under GPL licence.