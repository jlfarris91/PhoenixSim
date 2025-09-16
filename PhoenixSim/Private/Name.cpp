
#include "Name.h"

using namespace Phoenix;

PHOENIXSIM_API const FName FName::None = FName();
PHOENIXSIM_API const FName FName::Empty = ""_n;

FName::operator unsigned int() const
{
    return Hash;
}

bool FName::operator==(const FName& other) const
{
    return Hash == other.Hash;
}

bool FName::operator!=(const FName& other) const
{
    return Hash != other.Hash;
}
