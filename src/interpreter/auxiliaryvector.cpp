#include "interpreter/auxiliaryvector.h"
#include "host/host.h"
#include <cassert>

namespace x64 {

    AuxiliaryVector& AuxiliaryVector::add(u64 type) {
        auto typeAndValue = Host::getauxval((Host::AUX_TYPE)type);
        if(!!typeAndValue) {
            data_.push_back(typeAndValue->type);
            data_.push_back(typeAndValue->value);
        }
        return *this;
    }

    AuxiliaryVector& AuxiliaryVector::add(u64 type, u64 value) {
        auto typeAndValue = Host::getauxval((Host::AUX_TYPE)type);
        if(!!typeAndValue) {
            data_.push_back(typeAndValue->type);
            data_.push_back(value);
        }
        return *this;
    }

    std::vector<u64> AuxiliaryVector::create() {
        std::vector<u64> res = data_;
        auto nil = Host::getauxval(Host::AUX_TYPE::NIL);
        assert(!!nil);
        res.push_back(nil->type);
        res.push_back(nil->value);
        return res;
    }

}