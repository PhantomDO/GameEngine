#pragma once

#include <chrono>

#include <nvrhi/nvrhi.h>

#include "levain/core/file.hpp"
#include "levain/render/mesh_pass.hpp"

/// Le hot-reload des shaders (ADR-0014) : les sources de `shaders/`, et le prochain moment où les
/// regarder.
struct ShaderReload
{
    levain::core::FileWatch sources;
    std::chrono::steady_clock::time_point nextCheck;
};

/// Commence à surveiller `shaders/` dans le dépôt.
[[nodiscard]] ShaderReload startShaderReload();

/// Si une source a changé : relance le build des shaders, puis recrée le pipeline des passes qui
/// l'utilisent. En cas d'échec, les erreurs vont dans le log et le rendu continue avec les shaders
/// en place. Ne regarde les fichiers qu'une fois par `ShaderCheckPeriod`.
void reloadChangedShaders(ShaderReload& reload, nvrhi::IDevice& device,
                          const nvrhi::FramebufferInfo& target, levain::render::MeshPass& meshPass);

/// L'intervalle de surveillance, qui s'ajoute au délai du hot-reload.
inline constexpr std::chrono::milliseconds ShaderCheckPeriod{100};
