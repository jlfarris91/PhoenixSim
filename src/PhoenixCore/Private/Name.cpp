
#include "Name.h"

using namespace Phoenix;

const FName FName::None = FName();
const FName FName::Empty = ""_n;

FName FName::operator+(const FName& other) const
{
    FName result = *this;
    result += other;
    return result;
}

FName& FName::operator+=(const FName& other)
{
#if DEBUG
    if (IsNoneOrEmpty(*this))
    {
        memcpy(Debug, other.Debug, _countof(Debug));
    }
    else
    {
        (void)snprintf(Debug, _countof(Debug), "%s.%s", Debug, other.Debug);
    }
#endif
    Value = Hashing::FN1VA32Combine(Value, other.Value);
    return *this;
}
