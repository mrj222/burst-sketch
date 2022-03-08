#ifndef _ABSTRACT_H_
#define _ABSTRACT_H_

#include <vector>
#include "Burst.h"

template<typename ID_TYPE>
class Abstract {
public:
    Abstract() {}
    virtual ~Abstract() {}

    virtual void insert(ID_TYPE id, uint64_t window) = 0;
    virtual std::vector<Burst<ID_TYPE>> query() = 0;
};

#endif