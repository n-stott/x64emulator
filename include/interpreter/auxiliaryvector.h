#ifndef AUXILIARYVECTOR_H
#define AUXILIARYVECTOR_H

#include "utils/utils.h"
#include <vector>

namespace kernel {

    class AuxiliaryVector {
    public:
        AuxiliaryVector& add(u64 type);
        AuxiliaryVector& add(u64 type, u64 value);
        std::vector<u64> create();

    private:
        std::vector<u64> data_;
    };

}

#endif