#include <limits>

#include <doctest/doctest.h>

#include "levain/render/texture.hpp"

using levain::render::clampAnisotropy;

TEST_CASE("clampAnisotropy ramène le niveau dans [1, 16]")
{
    CHECK(clampAnisotropy(1.0f) == 1.0f);
    CHECK(clampAnisotropy(8.0f) == 8.0f);
    CHECK(clampAnisotropy(64.0f) == 16.0f); // le maximum de Direct3D 12
    CHECK(clampAnisotropy(0.0f) == 1.0f);
    CHECK(clampAnisotropy(-4.0f) == 1.0f);
}

TEST_CASE("clampAnisotropy ne laisse pas passer un NaN jusqu'au pilote")
{
    CHECK(clampAnisotropy(std::numeric_limits<float>::quiet_NaN()) == 1.0f);
}
