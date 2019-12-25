
#include "eptg/in.hpp"

namespace eptg {

bool in(const char * values, const char v)
{
    while(*values)
        if (v == *values++)
            return true;
    return false;
}

} // namespace
