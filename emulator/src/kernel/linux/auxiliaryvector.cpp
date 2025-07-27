#include "kernel/linux/auxiliaryvector.h"
#include "host/host.h"
#include "utils.h"
#include <cassert>
#include <vector>

namespace kernel {

    AuxiliaryVector& AuxiliaryVector::add(u64 type) {
        using namespace kernel;
        auto typeAndValue = Host::getauxval((Host::AUX_TYPE)type);
        if(!!typeAndValue) {
            data_.push_back(typeAndValue->type);
            data_.push_back(typeAndValue->value);
        }
        return *this;
    }

    AuxiliaryVector& AuxiliaryVector::add(u64 type, u64 value) {
        using namespace kernel;
        auto typeAndValue = Host::getauxval((Host::AUX_TYPE)type);
        if(!!typeAndValue) {
            data_.push_back(typeAndValue->type);
            data_.push_back(value);
        }
        return *this;
    }

    std::vector<u64> AuxiliaryVector::create() {
        using namespace kernel;
        std::vector<u64> res = data_;
        auto nil = Host::getauxval(Host::AUX_TYPE::NIL);
        assert(!!nil);
        res.push_back(nil->type);
        res.push_back(nil->value);
        return res;
    }

}