#include "levain/core/error.hpp"

#include <utility>

namespace levain::core
{

std::unexpected<Error> makeError(ErrorCode code, std::string message)
{
    return std::unexpected{Error{code, std::move(message)}};
}

std::string_view describe(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::FileNotFound:
        return "fichier introuvable";
    case ErrorCode::InvalidData:
        return "données invalides";
    case ErrorCode::OutOfMemory:
        return "mémoire insuffisante";
    case ErrorCode::Unsupported:
        return "non pris en charge";
    }

    return "code d'erreur inconnu";
}

} // namespace levain::core
