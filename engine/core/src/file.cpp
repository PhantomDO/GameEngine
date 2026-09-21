#include "levain/core/file.hpp"

#include <cstdint>
#include <format>
#include <fstream>
#include <system_error>

namespace levain::core
{

Result<std::vector<std::byte>> readFile(const std::filesystem::path& path)
{
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    std::ifstream file{path, std::ios::binary};
    if (error || !file)
    {
        return makeError(ErrorCode::FileNotFound,
                         std::format("{} : introuvable ou illisible", path.string()));
    }

    std::vector<std::byte> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
    {
        return makeError(ErrorCode::InvalidData,
                         std::format("{} : lecture incomplète", path.string()));
    }
    return bytes;
}

} // namespace levain::core
