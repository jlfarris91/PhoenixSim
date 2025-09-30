
#include "Name.h"

using namespace Phoenix;

const FName FName::None = FName();
const FName FName::Empty = ""_n;

FName::operator size_t() const
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
