#include "levain/input/bindings.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <ranges>

#include "levain/core/file.hpp"

namespace levain::input
{

namespace
{

using core::ErrorCode;
using platform::InputDevice;

std::string_view trimmed(std::string_view text)
{
    const auto first = text.find_first_not_of(" \t\r");
    const auto last = text.find_last_not_of(" \t\r");
    return first == std::string_view::npos ? std::string_view{}
                                           : text.substr(first, last - first + 1);
}

/// La ligne sans son commentaire : tout ce qui suit « # » est pour le lecteur humain.
std::string_view withoutComment(std::string_view line)
{
    return trimmed(line.substr(0, line.find('#')));
}

std::unexpected<core::Error> lineError(int line, std::string_view what)
{
    return core::makeError(ErrorCode::InvalidData, std::format("ligne {} : {}", line, what));
}

std::optional<float> numberOf(std::string_view text)
{
    float value = 0.0f;
    const char* const first = text.data(); // la taille voyage avec, juste en dessous
    const char* const last = first + text.size();
    const auto parsed = std::from_chars(first, last, value);
    return parsed.ec == std::errc{} && parsed.ptr == last ? std::optional{value} : std::nullopt;
}

/// Le code d'un nom, et s'il désigne un axe. On essaie bouton **puis** axe : `pad:a` est un bouton,
/// `pad:leftx` un axe, et c'est SDL qui tranche (`platform/input.hpp`).
std::optional<Source> sourceOf(InputDevice device, const std::string& name)
{
    const auto button = device == InputDevice::Keyboard ? platform::keyCodeFromName(name)
                        : device == InputDevice::Mouse  ? platform::mouseButtonCodeFromName(name)
                                                        : platform::padButtonCodeFromName(name);
    if (button)
    {
        return Source{.device = device, .code = *button, .isAxis = false};
    }

    const auto axis = device == InputDevice::Mouse     ? platform::mouseAxisCodeFromName(name)
                      : device == InputDevice::Gamepad ? platform::padAxisCodeFromName(name)
                                                       : std::nullopt;
    if (axis)
    {
        return Source{.device = device, .code = *axis, .isAxis = true};
    }
    return std::nullopt;
}

/// L'indice d'une liaison dans sa liste, par son nom.
std::optional<int> indexOf(const std::vector<Binding>& bindings, std::string_view name)
{
    const auto found = std::ranges::find(bindings, name, &Binding::name);
    return found == bindings.end()
               ? std::nullopt
               : std::optional{static_cast<int>(std::distance(bindings.begin(), found))};
}

std::optional<InputDevice> deviceOf(std::string_view name)
{
    if (name == "key")
    {
        return InputDevice::Keyboard;
    }
    if (name == "mouse")
    {
        return InputDevice::Mouse;
    }
    if (name == "pad")
    {
        return InputDevice::Gamepad;
    }
    return std::nullopt;
}

/// Un jeton `appareil:nom[:échelle]`, par exemple `key:A:-1` ou `pad:leftx`.
core::Result<Source> parseSource(std::string_view token, int line)
{
    const auto firstColon = token.find(':');
    if (firstColon == std::string_view::npos)
    {
        return lineError(line, std::format("« {} » : il manque « appareil: »", token));
    }
    const auto device = deviceOf(token.substr(0, firstColon));
    if (!device)
    {
        return lineError(line, std::format("appareil inconnu « {} » (key, mouse ou pad)",
                                           token.substr(0, firstColon)));
    }

    std::string_view rest = token.substr(firstColon + 1);
    float scale = 1.0f;
    // Le nom d'une touche peut contenir des espaces (« Left Shift »), jamais deux-points : ce qui
    // suit le second deux-points est donc l'échelle.
    if (const auto secondColon = rest.find(':'); secondColon != std::string_view::npos)
    {
        const auto parsed = numberOf(trimmed(rest.substr(secondColon + 1)));
        if (!parsed)
        {
            return lineError(line, std::format("échelle illisible dans « {} »", token));
        }
        scale = *parsed;
        rest = rest.substr(0, secondColon);
    }

    auto source = sourceOf(*device, std::string{trimmed(rest)});
    if (!source)
    {
        return lineError(line, std::format("« {} » : nom inconnu de SDL", token));
    }
    source->scale = scale;
    return *source;
}

core::Result<Binding> parseBinding(std::string_view name, std::string_view sources, int line)
{
    Binding binding{.name = std::string{name}, .sources = {}};
    for (const auto part : std::views::split(sources, ','))
    {
        const std::string_view token = trimmed(std::string_view{part});
        if (token.empty())
        {
            return lineError(line, "une source vide entre deux virgules");
        }
        auto source = parseSource(token, line);
        if (!source)
        {
            return std::unexpected(source.error());
        }
        binding.sources.push_back(*source);
    }
    if (binding.sources.empty())
    {
        return lineError(line, std::format("« {} » n'est lié à rien", name));
    }
    return binding;
}

} // namespace

core::Result<Bindings> parseBindings(std::string_view text)
{
    Bindings bindings;
    int lineNumber = 0;
    for (const auto rawLine : std::views::split(text, '\n'))
    {
        ++lineNumber;
        const std::string_view line = withoutComment(std::string_view{rawLine});
        if (line.empty())
        {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string_view::npos)
        {
            return lineError(lineNumber, std::format("« {} » : il manque « = »", line));
        }
        const std::string_view left = trimmed(line.substr(0, equals));
        const std::string_view right = trimmed(line.substr(equals + 1));

        if (left == "deadzone")
        {
            const auto value = numberOf(right);
            if (!value || *value < 0.0f || *value >= 1.0f)
            {
                return lineError(lineNumber, "deadzone : un nombre entre 0 et 1 est attendu");
            }
            bindings.deadzone = *value;
            continue;
        }

        const auto space = left.find_first_of(" \t");
        if (space == std::string_view::npos)
        {
            return lineError(lineNumber, std::format("« {} » : « action <nom> », « axis <nom> » ou "
                                                     "« deadzone » est attendu",
                                                     left));
        }
        const std::string_view kind = left.substr(0, space);
        const std::string_view name = trimmed(left.substr(space + 1));
        if (kind != "action" && kind != "axis")
        {
            return lineError(lineNumber, std::format("« {} » inconnu (action ou axis)", kind));
        }

        auto binding = parseBinding(name, right, lineNumber);
        if (!binding)
        {
            return std::unexpected(binding.error());
        }
        (kind == "action" ? bindings.actions : bindings.axes).push_back(std::move(*binding));
    }
    return bindings;
}

core::Result<Bindings> loadBindings(const std::filesystem::path& path)
{
    auto bytes = core::readFile(path);
    if (!bytes)
    {
        return std::unexpected(bytes.error());
    }
    auto bindings = parseBindings(
        std::string_view{reinterpret_cast<const char*>(bytes->data()), bytes->size()});
    if (!bindings)
    {
        // Le nom du fichier n'est pas dans parseBindings, qui ne sait pas d'où vient son texte.
        return core::makeError(bindings.error().code,
                               std::format("{} : {}", path.string(), bindings.error().message));
    }
    return bindings;
}

std::optional<int> actionIndex(const Bindings& bindings, std::string_view name)
{
    return indexOf(bindings.actions, name);
}

std::optional<int> axisIndex(const Bindings& bindings, std::string_view name)
{
    return indexOf(bindings.axes, name);
}

} // namespace levain::input
