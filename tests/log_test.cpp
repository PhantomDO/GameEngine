#include <ostream>

#include <doctest/doctest.h>

#include "levain/core/log.hpp"

using levain::core::isLogEnabled;
using levain::core::LogLevel;
using levain::core::setLogLevel;

TEST_CASE("le niveau d'une catégorie filtre les messages moins importants")
{
    setLogLevel("test.filtre", LogLevel::Warning);

    CHECK_FALSE(isLogEnabled("test.filtre", LogLevel::Info));
    CHECK(isLogEnabled("test.filtre", LogLevel::Warning));
    CHECK(isLogEnabled("test.filtre", LogLevel::Error));
}

TEST_CASE("les catégories sont indépendantes les unes des autres")
{
    setLogLevel("test.bruyante", LogLevel::Trace);
    setLogLevel("test.silencieuse", LogLevel::Critical);

    CHECK(isLogEnabled("test.bruyante", LogLevel::Trace));
    CHECK_FALSE(isLogEnabled("test.silencieuse", LogLevel::Trace));
}
