#include "Singletons.h"

namespace chil::ioc
{
    Singletons& Sing()
    {
        static Singletons sing;
        return sing;
    }
}
