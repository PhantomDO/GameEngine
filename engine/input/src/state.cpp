#include "levain/input/state.hpp"

#include <algorithm>
#include <cmath>

namespace levain::input
{

namespace
{

using platform::InputDevice;

bool buttonHeld(const RawInput& raw, const Source& source)
{
    switch (source.device)
    {
    case InputDevice::Keyboard:
        return raw.keys.test(source.code);
    case InputDevice::Mouse:
        return raw.mouseButtons.test(source.code);
    case InputDevice::Gamepad:
        return raw.padButtons.test(source.code);
    }
    return false;
}

/// La valeur d'un axe brut, avant son échelle. Un axe de manette est une position, dont on retire
/// la zone morte ; la souris est un déplacement, ramené à une vitesse.
float axisReading(const RawInput& raw, const Source& source, float deadzone, float frameSeconds)
{
    if (source.device == InputDevice::Gamepad)
    {
        return applyDeadzone(raw.padAxes.at(source.code), deadzone);
    }
    // Une image de durée nulle arrive au premier tour et dans les tests : pas de déplacement à
    // convertir, donc pas de division par zéro.
    return frameSeconds > 0.0f ? raw.motion.at(source.code) / frameSeconds : 0.0f;
}

float bindingValue(const RawInput& raw, const Binding& binding, float deadzone, float frameSeconds)
{
    float value = 0.0f;
    for (const Source& source : binding.sources)
    {
        value += sourceValue(raw, source, deadzone, frameSeconds);
    }
    return value;
}

bool bindingHeld(const RawInput& raw, const Binding& binding, float deadzone)
{
    // Une gâchette est un axe : elle compte comme un appui passé la moitié de sa course. Le temps
    // de l'image ne sert pas ici — un appui n'est pas une vitesse.
    return std::ranges::any_of(
        binding.sources,
        [&raw, deadzone](const Source& source)
        {
            return source.isAxis
                       ? std::abs(sourceValue(raw, source, deadzone, 1.0f)) >= ActionThreshold
                       : buttonHeld(raw, source);
        });
}

} // namespace

void applyEvent(RawInput& raw, const platform::InputEvent& event)
{
    const bool down = event.type == platform::InputEventType::ButtonDown;
    switch (event.type)
    {
    case platform::InputEventType::ButtonDown:
    case platform::InputEventType::ButtonUp:
        switch (event.device)
        {
        case InputDevice::Keyboard:
            raw.keys.set(event.code, down);
            break;
        case InputDevice::Mouse:
            raw.mouseButtons.set(event.code, down);
            break;
        case InputDevice::Gamepad:
            raw.padButtons.set(event.code, down);
            break;
        }
        break;

    case platform::InputEventType::AxisMotion:
        if (event.device == InputDevice::Gamepad)
        {
            raw.padAxes.at(event.code) = event.value;
        }
        else
        {
            // Les déplacements de la souris **s'ajoutent** : SDL en envoie plusieurs par image.
            raw.motion.at(event.code) += event.value;
        }
        break;
    }
}

InputState makeInputState(const Bindings& bindings)
{
    return InputState{.actionsHeld = std::vector<bool>(bindings.actions.size(), false),
                      .actionsHeldPreviously = std::vector<bool>(bindings.actions.size(), false),
                      .axes = std::vector<float>(bindings.axes.size(), 0.0f),
                      .raw = {}};
}

void updateInput(InputState& state, const Bindings& bindings,
                 std::span<const platform::InputEvent> events, float frameSeconds)
{
    state.actionsHeldPreviously = state.actionsHeld;
    state.raw.motion = {}; // le déplacement de la souris n'appartient qu'à l'image qui vient

    for (const platform::InputEvent& event : events)
    {
        applyEvent(state.raw, event);
    }

    for (std::size_t i = 0; i < bindings.actions.size(); ++i)
    {
        state.actionsHeld[i] = bindingHeld(state.raw, bindings.actions[i], bindings.deadzone);
    }
    for (std::size_t i = 0; i < bindings.axes.size(); ++i)
    {
        state.axes[i] = bindingValue(state.raw, bindings.axes[i], bindings.deadzone, frameSeconds);
    }
}

bool actionHeld(const InputState& state, int action)
{
    return state.actionsHeld.at(static_cast<std::size_t>(action));
}

bool actionPressed(const InputState& state, int action)
{
    const auto index = static_cast<std::size_t>(action);
    return state.actionsHeld.at(index) && !state.actionsHeldPreviously.at(index);
}

float axisValue(const InputState& state, int axis)
{
    return state.axes.at(static_cast<std::size_t>(axis));
}

float applyDeadzone(float value, float deadzone)
{
    const float magnitude = std::abs(value);
    if (magnitude <= deadzone)
    {
        return 0.0f;
    }
    const float scaled = (magnitude - deadzone) / (1.0f - deadzone);
    return std::copysign(std::min(scaled, 1.0f), value);
}

float sourceValue(const RawInput& raw, const Source& source, float deadzone, float frameSeconds)
{
    return source.isAxis ? axisReading(raw, source, deadzone, frameSeconds) * source.scale
           : buttonHeld(raw, source) ? source.scale
                                     : 0.0f;
}

} // namespace levain::input
