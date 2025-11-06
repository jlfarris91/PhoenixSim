
#include "Name.h"

using namespace Phoenix;

const FName FName::None = FName();
const FName FName::Empty = ""_n;

FName::operator hash32_t() const
{
    return Value;
}

bool FName::operator==(const FName& other) const
{
    return Value == other.Value;
}

bool FName::operator!=(const FName& other) const
{
    return Value != other.Value;
}

std::strong_ordering FName::operator<=>(const FName& other) const
{
    return Value <=> other.Value;
}

FName FName::operator+(const FName& other) const
{
    FName result = *this;
    result += other;
    return result;
}

FName& FName::operator+=(const FName& other)
{
    Value = Hashing::FN1VA32Combine(Value, other.Value);
#if DEBUG
    (void)snprintf(Debug, _countof(Debug), "%s.%s", Debug, other.Debug);
#endif
    return *this;
}
