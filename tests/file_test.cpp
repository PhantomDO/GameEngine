#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ostream>

#include <doctest/doctest.h>

#include "levain/core/file.hpp"

using levain::core::ErrorCode;
using levain::core::readFile;

TEST_CASE("readFile rend le contenu exact d'un fichier")
{
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "levain_file_test.bin";
    {
        std::ofstream file{path, std::ios::binary};
        file.write("\x00\x01\xFF", 3);
    }

    const auto bytes = readFile(path);
    std::filesystem::remove(path);

    REQUIRE(bytes.has_value());
    CHECK(bytes->size() == 3);
    CHECK(bytes->at(2) == std::byte{0xFF});
}

TEST_CASE("readFile signale un fichier absent comme FileNotFound")
{
    const auto bytes = readFile("/chemin/qui/n/existe/pas");

    REQUIRE_FALSE(bytes.has_value());
    CHECK(bytes.error().code == ErrorCode::FileNotFound);
}
