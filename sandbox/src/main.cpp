#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <expected>
#include <format>
#include <limits>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "levain/core/assert.hpp"
#include "levain/core/frame_time.hpp"
#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"
#include "levain/gpu/device.hpp"
#include "levain/platform/window.hpp"
#include "levain/render/camera.hpp"
#include "levain/render/mesh.hpp"
#include "levain/render/mesh_pass.hpp"

namespace
{

using Clock = std::chrono::steady_clock;

/// Durée sur laquelle le frame time du titre est résumé.
constexpr double FrameTimePeriodSeconds = 1.0;

/// Validation en Debug seulement (règle n°4) : elle coûte cher, et c'est là qu'on développe.
constexpr bool EnableValidation = LEVAIN_ASSERTIONS_ENABLED != 0;

struct LoopState
{
    bool isRunning = true;
    bool isVisible = true;
};

void applyWindowEvent(LoopState& state, const levain::platform::WindowEvent& event)
{
    using levain::platform::WindowEventType;

    switch (event.type)
    {
    case WindowEventType::CloseRequested:
        state.isRunning = false;
        break;
    // Les deux arrivent en double : sous Wayland, SDL renvoie EXPOSED après chaque
    // redimensionnement. On ne journalise donc que les changements d'état.
    case WindowEventType::Hidden:
        if (state.isVisible)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Info, "masquée : boucle en pause");
        }
        state.isVisible = false;
        break;
    case WindowEventType::Shown:
        if (!state.isVisible)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Info, "visible : boucle relancée");
        }
        state.isVisible = true;
        break;
    case WindowEventType::Resized:
        // M1.2 : c'est ici que la swapchain sera recréée à la nouvelle taille.
        levain::core::log("sandbox", levain::core::LogLevel::Info, "redimensionnée : {} × {} px",
                          event.pixelSize.width, event.pixelSize.height);
        break;
    }
}

double secondsBetween(Clock::time_point start, Clock::time_point end)
{
    return std::chrono::duration<double>(end - start).count();
}

std::string describeFrameTimes(const levain::core::FrameTimeSummary& summary)
{
    // Tirets ASCII : setWindowTitle refuse le reste (voir window.hpp). averageMs n'est jamais
    // nul, un résumé couvre au moins FrameTimePeriodSeconds.
    return std::format("Levain - {:.3f} ms (min {:.3f}, max {:.3f}) - {:.0f} images/s",
                       summary.averageMs, summary.minMs, summary.maxMs, 1000.0 / summary.averageMs);
}

/// Ce que dessine le sandbox en M2.1 : un cube qui tourne, vu par une caméra fixe.
struct DemoScene
{
    levain::render::MeshPass meshPass;
    levain::render::Mesh cube;
    levain::render::Camera camera;
    nvrhi::TextureHandle depth; ///< Créé à la première frame, à la taille de l'image.
};

/// Crée la passe des meshes et envoie le cube au GPU.
levain::core::Result<DemoScene> createDemoScene(levain::gpu::GpuDevice& gpu)
{
    auto meshPass = levain::render::createMeshPass(
        *gpu.nvrhi, nvrhi::FramebufferInfo()
                        .addColorFormat(levain::gpu::swapchainFormat(gpu))
                        .setDepthFormat(levain::render::DepthFormat));
    if (!meshPass)
    {
        return std::unexpected(meshPass.error());
    }

    const nvrhi::CommandListHandle upload = gpu.nvrhi->createCommandList();
    upload->open();
    levain::render::Mesh cube = levain::render::createCube(*gpu.nvrhi, *upload);
    upload->close();
    gpu.nvrhi->executeCommandList(upload);

    return DemoScene{
        .meshPass = std::move(*meshPass), .cube = std::move(cube), .camera = {}, .depth = {}};
}

/// Un tour toutes les 6 secondes, autour d'un axe incliné pour montrer trois faces à la fois.
glm::mat4 cubeRotation(double seconds)
{
    const float angle = static_cast<float>(seconds) * glm::two_pi<float>() / 6.0f;
    return glm::rotate(glm::mat4{1.0f}, angle, glm::vec3{1.0f, 1.0f, 0.0f});
}

/// Efface l'image de la swapchain et son depth buffer, y dessine le cube, et la présente.
void renderFrame(levain::gpu::GpuDevice& gpu, const levain::platform::Window& window,
                 DemoScene& scene, nvrhi::ICommandList& commandList, double seconds)
{
    nvrhi::ITexture* backBuffer = levain::gpu::beginFrame(gpu, window);
    if (backBuffer == nullptr)
    {
        return;
    }

    {
        // Le travail CPU d'une frame, hors attente de l'écran (critère de M1.3,
        // tools/tracy-capture.sh).
        LEVAIN_PROFILE_SCOPE_NAMED("commandes");

        const nvrhi::TextureDesc& target = backBuffer->getDesc();
        nvrhi::ITexture* depth = levain::render::ensureDepthTexture(*gpu.nvrhi, scene.depth,
                                                                    target.width, target.height);

        // ponytail: framebuffer recréé à chaque frame. C'est léger avec le rendu dynamique de
        // Vulkan 1.3 (NVRHI ne crée pas de VkFramebuffer) ; un cache par image si un profil le
        // montre.
        const nvrhi::FramebufferHandle framebuffer = gpu.nvrhi->createFramebuffer(
            nvrhi::FramebufferDesc().addColorAttachment(backBuffer).setDepthAttachment(depth));

        const levain::render::SceneConstants constants{
            .viewProjection = levain::render::viewProjectionOf(
                scene.camera, static_cast<float>(target.width) / static_cast<float>(target.height)),
            .model = cubeRotation(seconds),
        };

        // La couleur de fond : une croûte de levain. Locale et non globale : le constructeur de
        // nvrhi::Color n'est pas noexcept, et une exception levée à l'initialisation d'une
        // globale ne se rattrape pas.
        const nvrhi::Color clearColor{0.55f, 0.32f, 0.14f, 1.0f};

        commandList.open();
        commandList.clearTextureFloat(backBuffer, nvrhi::AllSubresources, clearColor);
        // 1 : la profondeur la plus lointaine, que tout ce qu'on dessine vient remplacer.
        commandList.clearDepthStencilTexture(depth, nvrhi::AllSubresources, true, 1.0f, false, 0);
        levain::render::drawMesh(commandList, scene.meshPass, *framebuffer, scene.cube, constants);
        commandList.close();
        gpu.nvrhi->executeCommandList(&commandList);
    }

    LEVAIN_PROFILE_SCOPE_NAMED("présentation");
    levain::gpu::presentFrame(gpu);
}

/// La durée de la boucle : `--seconds N`, ou sans limite. Vide si les arguments sont invalides.
///
/// Comptée depuis le premier tour de boucle, pas depuis le lancement : en CI, le démarrage varie de
/// 1 à plus de 10 s selon la charge du runner (lavapipe), et un délai extérieur tombait parfois
/// avant la première frame.
std::optional<double> parseLoopSeconds(std::span<char* const> arguments)
{
    if (arguments.size() == 1)
    {
        return std::numeric_limits<double>::infinity();
    }
    if (arguments.size() != 3 || std::string_view{arguments[1]} != "--seconds")
    {
        return std::nullopt;
    }

    const std::string_view value{arguments[2]};
    double seconds = 0.0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), seconds);
    if (error != std::errc{} || end != value.data() + value.size() || seconds <= 0.0)
    {
        return std::nullopt;
    }
    return seconds;
}

void runMainLoop(levain::platform::Window& window, levain::gpu::GpuDevice& gpu, DemoScene& scene,
                 double loopSeconds)
{
    const nvrhi::CommandListHandle commandList = gpu.nvrhi->createCommandList();
    LoopState state;
    levain::core::FrameTimeAccumulator frameTimes;
    const Clock::time_point loopStart = Clock::now();
    Clock::time_point previousFrameEnd = loopStart;
    int frameCount = 0;

    while (state.isRunning && secondsBetween(loopStart, Clock::now()) < loopSeconds)
    {
        if (!state.isVisible)
        {
            for (const auto& event : levain::platform::waitEvents(window))
            {
                applyWindowEvent(state, event);
            }

            // Le temps passé masquée n'est pas une frame. Sans cette remise à l'heure, la
            // première frame après la restauration durerait toute la minimisation, et le
            // maximum affiché serait de plusieurs secondes.
            previousFrameEnd = Clock::now();
            continue;
        }

        {
            LEVAIN_PROFILE_SCOPE_NAMED("événements");

            for (const auto& event : levain::platform::pollEvents(window))
            {
                applyWindowEvent(state, event);
            }
        }

        {
            LEVAIN_PROFILE_SCOPE_NAMED("rendu");
            renderFrame(gpu, window, scene, *commandList, secondsBetween(loopStart, Clock::now()));
        }

        const Clock::time_point frameEnd = Clock::now();
        const double frameSeconds = secondsBetween(previousFrameEnd, frameEnd);
        previousFrameEnd = frameEnd;

        if (const auto summary =
                levain::core::recordFrame(frameTimes, frameSeconds, FrameTimePeriodSeconds))
        {
            LEVAIN_PROFILE_SCOPE_NAMED("titre");
            levain::platform::setWindowTitle(window, describeFrameTimes(*summary));
        }

        ++frameCount;
        LEVAIN_PROFILE_FRAME();
    }

    // Lu par la CI, qui échoue si la boucle a tourné moins d'une seconde : un démarrage lent
    // (lavapipe, validation, sanitizers) peut sinon manger tout le délai sans que rien ne rougisse.
    levain::core::log("sandbox", levain::core::LogLevel::Info,
                      "boucle arrêtée après {:.1f} s et {} frames",
                      secondsBetween(loopStart, Clock::now()), frameCount);
}

} // namespace

int main(int argc, char** argv)
{
    // std::print et std::format peuvent lever : format_error sur une chaîne de format
    // invalide, system_error si l'écriture échoue. On rattrape au sommet (ADR-0008).
    try
    {
        const std::optional<double> loopSeconds =
            parseLoopSeconds(std::span{argv, static_cast<std::size_t>(argc)});
        if (!loopSeconds)
        {
            std::println(stderr, "usage : levain_sandbox [--seconds N]");
            return 2;
        }

        std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
                   levain::core::toolchain(), __cplusplus);

        auto window = levain::platform::createWindow("Levain", 1280, 720);
        if (!window)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              window.error().message);
            return 1;
        }

        // Déclaré après window, gpu sera détruit avant elle : la surface Vulkan doit disparaître
        // avant la fenêtre SDL qui la porte.
        const Clock::time_point deviceStart = Clock::now();
        auto gpu = levain::gpu::createGpuDevice(*window, {.enableValidation = EnableValidation});
        if (!gpu)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              gpu.error().message);
            return 1;
        }
        levain::core::log("sandbox", levain::core::LogLevel::Info, "device créé en {:.1f} ms",
                          secondsBetween(deviceStart, Clock::now()) * 1000.0);

        auto scene = createDemoScene(*gpu);
        if (!scene)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              scene.error().message);
            return 1;
        }

        runMainLoop(*window, *gpu, *scene, *loopSeconds);
        levain::core::log("sandbox", levain::core::LogLevel::Info, "fenêtre fermée");
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }

    return 0;
}
