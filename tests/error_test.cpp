#include <ostream>

#include <doctest/doctest.h>

#include "levain/core/error.hpp"

using levain::core::ErrorCode;
using levain::core::makeError;
using levain::core::Result;

namespace
{

/// Fonction d'exemple : échoue pour une raison qui n'est pas un bug.
Result<int> parsePositive(int value)
{
    if (value <= 0)
    {
        return makeError(ErrorCode::InvalidData, "attendu strictement positif");
    }

    return value;
}

} // namespace

TEST_CASE("un Result porte la valeur en cas de succès")
{
    const auto result = parsePositive(42);

    REQUIRE(result.has_value());
    CHECK(*result == 42);
}

TEST_CASE("un Result porte le code et le message en cas d'échec")
{
    const auto result = parsePositive(-1);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidData);
    CHECK(result.error().message == "attendu strictement positif");
}

TEST_CASE("chaque code d'erreur a une description non vide")
{
    for (const auto code : {ErrorCode::FileNotFound, ErrorCode::InvalidData, ErrorCode::OutOfMemory,
                            ErrorCode::Unsupported})
    {
        CHECK_FALSE(levain::core::describe(code).empty());
    }
}
