#include <array>
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
#include <vector>

#include <flecs.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_reload.hpp"

#include "levain/assets/image.hpp"
#include "levain/core/assert.hpp"
#include "levain/core/frame_time.hpp"
#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"
#include "levain/gpu/device.hpp"
#include "levain/input/bindings.hpp"
#include "levain/input/state.hpp"
#include "levain/platform/window.hpp"
#include "levain/render/camera.hpp"
#include "levain/render/gpu_timer.hpp"
#include "levain/render/mesh.hpp"
#include "levain/render/mesh_pass.hpp"
#include "levain/render/texture.hpp"
#include "levain/scene/camera_control.hpp"
#include "levain/scene/components.hpp"
#include "levain/scene/fixed_step.hpp"
#include "levain/scene/scene.hpp"
#include "levain/scene/transform.hpp"

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

/// Temps GPU moyen sur une période : somme et nombre des mesures reçues.
struct GpuTimeAverage
{
    double totalMs = 0.0;
    int samples = 0;
};

double averageOf(const GpuTimeAverage& average)
{
    return average.samples > 0 ? average.totalMs / average.samples : 0.0;
}

std::string describeFrameTimes(const levain::core::FrameTimeSummary& summary, double gpuMs)
{
    // Tirets ASCII : setWindowTitle refuse le reste (voir window.hpp). averageMs n'est jamais
    // nul, un résumé couvre au moins FrameTimePeriodSeconds.
    return std::format(
        "Levain - {:.3f} ms (min {:.3f}, max {:.3f}) - {:.0f} images/s - GPU {:.3f} ms",
        summary.averageMs, summary.minMs, summary.maxMs, 1000.0 / summary.averageMs, gpuMs);
}

/// Ce que le rendu dessine comme cube : un tag, vide, posé sur les entités de la grille. Demander
/// plutôt « les enfants de grid » coûterait 212 µs par frame au lieu de 8 : une requête
/// `(ChildOf, parent)` combinée à un composant ne se résout pas table par table quand la hiérarchie
/// est rangée dans `flecs::Parent` (manuel des hiérarchies de flecs, « Query performance »).
struct Cube
{
};

/// Le critère de M2.1 : 10 000 cubes instanciés, en une grille de 100 × 100.
constexpr int GridSide = 100;
constexpr float GridSpacing = 1.5f;

/// Le sol : 1000 unités de côté, pour qu'il file jusqu'à l'horizon, et une case du damier par unité
/// (la texture a 8 cases de côté).
constexpr float GroundSize = 1000.0f;
constexpr float GroundTextureRepeat = GroundSize / 8.0f;

/// Ce que dessine le sandbox en M2.2 : une grille de cubes texturés qui tournent, sur un sol qui
/// file jusqu'à l'horizon, où se voit le filtrage anisotrope.
struct DemoScene
{
    flecs::world world; ///< Les cubes, une entité chacun (M3.1), enfants de « grid » (M3.2).
    levain::scene::FixedStep fixedStep; ///< L'horloge de la simulation, 60 Hz (M3.3).
    flecs::query<const levain::scene::WorldTransform> cubes;
    std::vector<glm::vec3>
        cubePositions; ///< Relevées à chaque frame, gardées pour ne pas réallouer.
    levain::render::MeshPass meshPass;
    levain::render::Mesh cube;
    levain::render::Instances grid;
    levain::render::Mesh ground;
    levain::render::Instances groundInstance; ///< Une seule, sous les cubes.
    nvrhi::TextureHandle checker;
    nvrhi::BindingSetHandle material;
    flecs::entity cameraEntity; ///< Transform + FpsController : la caméra libre (M3.4).
    levain::render::Camera camera;
    levain::render::GpuTimer gpuTimer;
    nvrhi::TextureHandle depth; ///< Créé à la première frame, à la taille de l'image.
};

/// Les cubes de la grille : une entité chacun, nommée par sa colonne et sa rangée
/// (« grid.cube_50_50 » au centre) pour la retrouver dans l'explorer, et **enfant** d'une entité
/// « grid » qu'on peut déplacer d'un bloc (M3.2). Leur position est donc dans le repère de la
/// grille, et leur vitesse, nulle au départ, est celle que l'explorer peut changer.
///
/// La hiérarchie passe par le composant `flecs::Parent` et jamais par `child_of` : une entité ne
/// peut pas avoir les deux, et le système des matrices monde ne voit que le premier (ADR-0015).
void spawnCubeGrid(flecs::world& world)
{
    const flecs::entity grid = world.entity("grid").set(levain::scene::Transform{});
    const float half = static_cast<float>(GridSide - 1) * GridSpacing / 2.0f;
    for (int z = 0; z < GridSide; ++z)
    {
        for (int x = 0; x < GridSide; ++x)
        {
            const glm::vec3 position{static_cast<float>(x) * GridSpacing - half, 0.0f,
                                     static_cast<float>(z) * GridSpacing - half};
            world.entity(flecs::Parent{grid}, std::format("cube_{}_{}", x, z).c_str())
                .set(levain::scene::Transform{.position = position})
                .set(levain::scene::Velocity{})
                .add<Cube>();
        }
    }
}

/// La caméra du rendu, relue sur l'entité : sa **matrice monde** porte la position et le regard
/// déjà interpolés entre deux pas de simulation (ADR-0016). Lire le `Transform` ferait saccader le
/// regard dès que le rendu va plus vite que la simulation.
void updateRenderCamera(levain::render::Camera& camera, const flecs::entity& cameraEntity)
{
    const glm::mat4& world = cameraEntity.get<levain::scene::WorldTransform>().matrix;
    camera.position = glm::vec3(world[3]);
    camera.target = camera.position + glm::vec3(glm::mat3(world) * glm::vec3{0.0f, 0.0f, -1.0f});
}

/// Les intentions du joueur, lues dans les axes et les actions du fichier de liaisons. Le sandbox
/// est le seul à connaître les deux côtés : `engine/scene` ignore l'existence de l'input, et
/// `engine/input` ne sait rien des caméras (SPECS §7).
struct CameraActions
{
    int moveRight = 0;
    int moveForward = 0;
    int moveUp = 0;
    int lookRight = 0;
    int lookUp = 0;
    int lookEnable = 0;
    int sprint = 0;
};

/// Les indices des actions et des axes dont la caméra a besoin, résolus **une fois**. Un nom absent
/// du fichier est une erreur de chargement : sans ça, la caméra ne répondrait jamais à cette
/// commande, sans que rien ne le dise (règle n°7).
[[nodiscard]] levain::core::Result<CameraActions>
cameraActionsOf(const levain::input::Bindings& bindings)
{
    CameraActions actions;
    const std::array<std::pair<const char*, int*>, 5> axes{{{"move_right", &actions.moveRight},
                                                            {"move_forward", &actions.moveForward},
                                                            {"move_up", &actions.moveUp},
                                                            {"look_right", &actions.lookRight},
                                                            {"look_up", &actions.lookUp}}};
    for (const auto& [name, index] : axes)
    {
        const auto found = levain::input::axisIndex(bindings, name);
        if (!found)
        {
            return levain::core::makeError(levain::core::ErrorCode::InvalidData,
                                           std::format("axe « {} » absent de input.cfg", name));
        }
        *index = found.value_or(-1);
    }
    const std::array<std::pair<const char*, int*>, 2> buttons{
        {{"look_enable", &actions.lookEnable}, {"sprint", &actions.sprint}}};
    for (const auto& [name, index] : buttons)
    {
        const auto found = levain::input::actionIndex(bindings, name);
        if (!found)
        {
            return levain::core::makeError(levain::core::ErrorCode::InvalidData,
                                           std::format("action « {} » absente de input.cfg", name));
        }
        *index = found.value_or(-1);
    }
    return actions;
}

levain::scene::FpsInput fpsInputFrom(const levain::input::InputState& input,
                                     const CameraActions& actions)
{
    // Le regard à la souris ne compte que si son bouton est tenu ; à la manette, le stick suffit.
    const bool looking = levain::input::actionHeld(input, actions.lookEnable);
    const float lookRight = levain::input::axisValue(input, actions.lookRight);
    const float lookUp = levain::input::axisValue(input, actions.lookUp);
    return {.move = {levain::input::axisValue(input, actions.moveRight),
                     levain::input::axisValue(input, actions.moveForward)},
            .up = levain::input::axisValue(input, actions.moveUp),
            .look = looking ? glm::vec2{lookRight, lookUp} : glm::vec2{0.0f},
            .sprint = levain::input::actionHeld(input, actions.sprint)};
}

/// Les positions des cubes **dans le monde**, dans `positions`, que le rendu envoie ensuite au GPU.
/// La glu entre scene et render, qui ne se connaissent pas (SPECS §7). Lire `WorldTransform` et non
/// `Transform` : c'est ce qui fait suivre les cubes quand la grille bouge.
void gatherCubePositions(const flecs::query<const levain::scene::WorldTransform>& cubes,
                         std::vector<glm::vec3>& positions)
{
    positions.clear();
    cubes.each([&positions](const levain::scene::WorldTransform& transform)
               { positions.push_back(levain::scene::worldPosition(transform)); });
}

#ifdef LEVAIN_ENABLE_EXPLORER
/// L'explorer web de flecs (https://www.flecs.dev/explorer) lit et modifie le monde par l'addon
/// REST, sur le port 27750. Debug seulement, et **sur la boucle locale** : par défaut, flecs écoute
/// sur toutes les interfaces, et son API distante sait aussi supprimer des entités et exécuter des
/// scripts (https://www.flecs.dev/flecs/FlecsRemoteApi.html).
void enableExplorerOnLoopback(flecs::world& world)
{
    world.import<flecs::stats>(); // les statistiques de l'onglet « Stats » de l'explorer
    // ipaddr doit venir de l'allocateur de flecs : EcsRest en prend la propriété et le libère à la
    // destruction du monde (src/addons/rest.c, ECS_DTOR(EcsRest)). Une chaîne statique finissait en
    // « double free » à la sortie du sandbox.
    world.set<flecs::Rest>(
        {.port = ECS_REST_DEFAULT_PORT, .ipaddr = ecs_os_strdup("127.0.0.1"), .impl = nullptr});
    levain::core::log("sandbox", levain::core::LogLevel::Info,
                      "explorer : https://www.flecs.dev/explorer (REST sur 127.0.0.1:{})",
                      ECS_REST_DEFAULT_PORT);
}
#endif

/// Les niveaux de mip dans le format qu'attend render. Ils pointent dans `mips`, qui doit leur
/// survivre jusqu'à l'envoi.
std::vector<levain::render::TextureLevel>
textureLevelsOf(const std::vector<levain::assets::Image>& mips)
{
    std::vector<levain::render::TextureLevel> levels;
    levels.reserve(mips.size());
    for (const levain::assets::Image& mip : mips)
    {
        levels.push_back({.width = mip.width, .height = mip.height, .rgba = mip.rgba});
    }
    return levels;
}

/// Le format des images où dessine la passe des meshes : la swapchain et le depth buffer.
nvrhi::FramebufferInfo sceneTargetOf(levain::gpu::GpuDevice& gpu)
{
    return nvrhi::FramebufferInfo()
        .addColorFormat(levain::gpu::swapchainFormat(gpu))
        .setDepthFormat(levain::render::DepthFormat);
}

/// Crée la passe des meshes et envoie au GPU le cube, la grille, le sol et la texture du damier.
levain::core::Result<DemoScene> createDemoScene(levain::gpu::GpuDevice& gpu,
                                                const levain::render::SamplerSettings& sampler)
{
    auto image = levain::assets::loadImage(LEVAIN_DATA_DIR "/textures/checker.png");
    if (!image)
    {
        return std::unexpected(image.error());
    }
    const std::vector<levain::assets::Image> mips =
        levain::assets::buildMipChain(std::move(*image));

    auto meshPass = levain::render::createMeshPass(*gpu.nvrhi, sceneTargetOf(gpu));
    if (!meshPass)
    {
        return std::unexpected(meshPass.error());
    }

    flecs::world world;
    world.import<levain::scene::SceneModule>();
#ifdef LEVAIN_ENABLE_EXPLORER
    enableExplorerOnLoopback(world);
#endif
    spawnCubeGrid(world);
    // La caméra est une entité comme les autres : basse, sur le côté de la grille, et visant loin
    // devant. Elle porte son état précédent pour que le rendu l'interpole entre deux pas de
    // simulation (ADR-0016) — sans quoi le regard avancerait par saccades de 16 ms.
    const flecs::entity cameraEntity =
        world.entity("camera")
            .set(levain::scene::Transform{.position = {100.0f, 8.0f, 120.0f}})
            // 10,3° de lacet et 10,1° sous l'horizon : exactement le regard des milestones
            // précédents, qui visait le point {92, 0, 76}.
            .set(levain::scene::FpsController{.yawDegrees = 10.3f, .pitchDegrees = -10.1f})
            .add<levain::scene::PreviousTransform>();
    // Les cubes, et rien d'autre : ni la grille, qui n'est qu'un point d'accroche, ni le sol.
    flecs::query<const levain::scene::WorldTransform> cubes =
        world.query_builder<const levain::scene::WorldTransform>("cubes").with<Cube>().build();
    levain::scene::FixedStep fixedStep;
    levain::scene::advanceWorld(world, fixedStep,
                                0.0f); // les matrices monde, avant le premier envoi
    std::vector<glm::vec3> cubePositions;
    gatherCubePositions(cubes, cubePositions);

    const nvrhi::CommandListHandle upload = gpu.nvrhi->createCommandList();
    upload->open();
    levain::render::Mesh cube = levain::render::createCube(*gpu.nvrhi, *upload);
    levain::render::Instances grid =
        levain::render::createInstances(*gpu.nvrhi, *upload, cubePositions);
    levain::render::Mesh ground =
        levain::render::createPlane(*gpu.nvrhi, *upload, GroundSize, GroundTextureRepeat);
    // Juste sous les cubes, qui tournent sur eux-mêmes : leur demi-diagonale fait 0,87.
    const std::array<glm::vec3, 1> groundOffset{glm::vec3{0.0f, -1.0f, 0.0f}};
    levain::render::Instances groundInstance =
        levain::render::createInstances(*gpu.nvrhi, *upload, groundOffset);
    nvrhi::TextureHandle checker =
        levain::render::createTexture(*gpu.nvrhi, *upload, textureLevelsOf(mips), "checker");
    upload->close();
    gpu.nvrhi->executeCommandList(upload);
    const nvrhi::SamplerHandle samplerHandle = levain::render::createSampler(*gpu.nvrhi, sampler);
    nvrhi::BindingSetHandle material =
        levain::render::createMaterialBindings(*gpu.nvrhi, *meshPass, *checker, *samplerHandle);

    // La grille occupe la gauche de l'image, le sol file jusqu'à l'horizon à droite, de plus en
    // plus de biais : c'est là que le filtrage trilinéaire seul le rend flou. Position et regard
    // sont ceux de l'entité, et le joueur peut les changer.
    const levain::render::Camera camera{.position = {},
                                        .target = {},
                                        .verticalFovRadians = glm::radians(60.0f),
                                        .nearPlane = 0.5f,
                                        .farPlane = 1000.0f};
    return DemoScene{.world = std::move(world),
                     .fixedStep = fixedStep,
                     .cubes = std::move(cubes),
                     .cubePositions = std::move(cubePositions),
                     .meshPass = std::move(*meshPass),
                     .cube = std::move(cube),
                     .grid = std::move(grid),
                     .ground = std::move(ground),
                     .groundInstance = std::move(groundInstance),
                     .checker = std::move(checker),
                     .material = std::move(material),
                     .cameraEntity = cameraEntity,
                     .camera = camera,
                     .gpuTimer = levain::render::createGpuTimer(*gpu.nvrhi),
                     .depth = {}};
}

/// Un tour toutes les 6 secondes, autour d'un axe incliné pour montrer trois faces à la fois.
glm::mat4 cubeRotation(double seconds)
{
    const float angle = static_cast<float>(seconds) * glm::two_pi<float>() / 6.0f;
    return glm::rotate(glm::mat4{1.0f}, angle, glm::vec3{1.0f, 1.0f, 0.0f});
}

/// Efface l'image de la swapchain et son depth buffer, y dessine la grille, et la présente. Rend le
/// temps GPU d'une frame précédente, dès qu'il est lisible.
std::optional<double> renderFrame(levain::gpu::GpuDevice& gpu,
                                  const levain::platform::Window& window, DemoScene& scene,
                                  nvrhi::ICommandList& commandList, double seconds)
{
    nvrhi::ITexture* backBuffer = levain::gpu::beginFrame(gpu, window);
    if (backBuffer == nullptr)
    {
        return std::nullopt;
    }

    std::optional<double> gpuMs;

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
        gpuMs = levain::render::beginGpuTimer(*gpu.nvrhi, commandList, scene.gpuTimer);
        commandList.clearTextureFloat(backBuffer, nvrhi::AllSubresources, clearColor);
        // 1 : la profondeur la plus lointaine, que tout ce qu'on dessine vient remplacer.
        commandList.clearDepthStencilTexture(depth, nvrhi::AllSubresources, true, 1.0f, false, 0);
        // Le renderer dessine ce que contient le monde : les positions du tour qui vient de finir.
        gatherCubePositions(scene.cubes, scene.cubePositions);
        levain::render::updateInstances(commandList, scene.grid, scene.cubePositions);
        levain::render::drawMesh(commandList, scene.meshPass, *framebuffer, scene.cube, scene.grid,
                                 *scene.material, constants);
        levain::render::drawMesh(
            commandList, scene.meshPass, *framebuffer, scene.ground, scene.groundInstance,
            *scene.material,
            {.viewProjection = constants.viewProjection, .model = glm::mat4{1.0f}});
        levain::render::endGpuTimer(commandList, scene.gpuTimer);
        commandList.close();
        gpu.nvrhi->executeCommandList(&commandList);
    }

    LEVAIN_PROFILE_SCOPE_NAMED("présentation");
    levain::gpu::presentFrame(gpu);
    return gpuMs;
}

struct SandboxOptions
{
    /// La durée de la boucle, sans limite par défaut. Comptée depuis le premier tour de boucle, pas
    /// depuis le lancement : en CI, le démarrage varie de 1 à plus de 10 s selon la charge du
    /// runner (lavapipe), et un délai extérieur tombait parfois avant la première frame.
    double loopSeconds = std::numeric_limits<double>::infinity();
    /// Le filtrage anisotrope du damier ; 1 le désactive (trilinéaire seul).
    float maxAnisotropy = 16.0f;
};

/// Un nombre strictement positif, écrit en entier. Vide sinon, NaN compris.
std::optional<double> parsePositive(std::string_view text)
{
    double value = 0.0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !(value > 0.0))
    {
        return std::nullopt;
    }
    return value;
}

/// `[--seconds N] [--anisotropy N]`, dans n'importe quel ordre. Vide si les arguments sont
/// invalides.
std::optional<SandboxOptions> parseOptions(std::span<char* const> arguments)
{
    SandboxOptions options;
    for (std::size_t i = 1; i < arguments.size(); i += 2)
    {
        const std::optional<double> value =
            i + 1 < arguments.size() ? parsePositive(arguments[i + 1]) : std::nullopt;
        const std::string_view name{arguments[i]};
        if (!value)
        {
            return std::nullopt;
        }
        if (name == "--seconds")
        {
            options.loopSeconds = *value;
        }
        else if (name == "--anisotropy")
        {
            options.maxAnisotropy = static_cast<float>(*value);
        }
        else
        {
            return std::nullopt;
        }
    }
    return options;
}

void runMainLoop(levain::platform::Window& window, levain::gpu::GpuDevice& gpu, DemoScene& scene,
                 double loopSeconds, const levain::input::Bindings& bindings,
                 const CameraActions& actions)
{
    const nvrhi::CommandListHandle commandList = gpu.nvrhi->createCommandList();
    LoopState state;
    levain::core::FrameTimeAccumulator frameTimes;
    GpuTimeAverage periodGpu; ///< Depuis la dernière mise à jour du titre.
    GpuTimeAverage totalGpu;  ///< Depuis le début de la boucle, journalisé à la fin.
    const Clock::time_point loopStart = Clock::now();
    Clock::time_point previousFrameEnd = loopStart;
    int frameCount = 0;
    ShaderReload shaderReload = startShaderReload();
    const nvrhi::FramebufferInfo sceneTarget = sceneTargetOf(gpu);

    levain::input::InputState input = levain::input::makeInputState(bindings);
    bool mouseCaptured = false;
    // La première image n'a pas d'image précédente : un pas de simulation, pour démarrer.
    double lastFrameSeconds = scene.fixedStep.stepSeconds;

    while (state.isRunning && secondsBetween(loopStart, Clock::now()) < loopSeconds)
    {
        if (!state.isVisible)
        {
            for (const auto& event : levain::platform::waitEvents(window).window)
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

            const levain::platform::Events events = levain::platform::pollEvents(window);
            for (const auto& event : events.window)
            {
                applyWindowEvent(state, event);
            }
            levain::input::updateInput(input, bindings, events.input,
                                       static_cast<float>(lastFrameSeconds));

            // La souris ne se capture que pendant le regard : sinon on ne pourrait plus rien
            // faire d'autre de la fenêtre.
            const bool looking = levain::input::actionHeld(input, actions.lookEnable);
            if (looking != mouseCaptured)
            {
                levain::platform::setMouseCaptured(window, looking);
                mouseCaptured = looking;
            }
            // Ce que le joueur demande, posé pour le prochain pas de simulation.
            scene.world.set<levain::scene::FpsInput>(fpsInputFrom(input, actions));
        }

        reloadChangedShaders(shaderReload, *gpu.nvrhi, sceneTarget, scene.meshPass);

        {
            // Un tour du monde : les pas de simulation que la dernière image a mérités, puis une
            // passe de rendu qui interpole et compose les matrices monde (ADR-0016). La durée
            // passée est celle de l'image précédente : celle-ci n'est pas encore finie.
            LEVAIN_PROFILE_SCOPE_NAMED("monde");
            levain::scene::advanceWorld(scene.world, scene.fixedStep,
                                        static_cast<float>(lastFrameSeconds));
            updateRenderCamera(scene.camera, scene.cameraEntity);
        }

        {
            LEVAIN_PROFILE_SCOPE_NAMED("rendu");
            if (const auto gpuMs = renderFrame(gpu, window, scene, *commandList,
                                               secondsBetween(loopStart, Clock::now())))
            {
                periodGpu.totalMs += *gpuMs;
                ++periodGpu.samples;
                totalGpu.totalMs += *gpuMs;
                ++totalGpu.samples;
            }
        }

        const Clock::time_point frameEnd = Clock::now();
        const double frameSeconds = secondsBetween(previousFrameEnd, frameEnd);
        previousFrameEnd = frameEnd;
        lastFrameSeconds = frameSeconds;

        if (const auto summary =
                levain::core::recordFrame(frameTimes, frameSeconds, FrameTimePeriodSeconds))
        {
            LEVAIN_PROFILE_SCOPE_NAMED("titre");
            levain::platform::setWindowTitle(window,
                                             describeFrameTimes(*summary, averageOf(periodGpu)));
            periodGpu = {};
        }

        ++frameCount;
        LEVAIN_PROFILE_FRAME();
    }

    // Lu par la CI, qui échoue si la boucle a tourné moins d'une seconde : un démarrage lent
    // (lavapipe, validation, sanitizers) peut sinon manger tout le délai sans que rien ne rougisse.
    levain::core::log(
        "sandbox", levain::core::LogLevel::Info,
        "boucle arrêtée après {:.1f} s et {} frames ; GPU : {:.3f} ms en moyenne sur {} mesures",
        secondsBetween(loopStart, Clock::now()), frameCount, averageOf(totalGpu), totalGpu.samples);
}

} // namespace

int main(int argc, char** argv)
{
    // std::print et std::format peuvent lever : format_error sur une chaîne de format
    // invalide, system_error si l'écriture échoue. On rattrape au sommet (ADR-0008).
    try
    {
        const std::optional<SandboxOptions> options =
            parseOptions(std::span{argv, static_cast<std::size_t>(argc)});
        if (!options)
        {
            std::println(stderr, "usage : levain_sandbox [--seconds N] [--anisotropy N]");
            return 2;
        }

        std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
                   levain::core::toolchain(), __cplusplus);

        // 1920 × 1080 : la résolution du critère de M2.1. En points ; un point vaut un pixel sur la
        // machine de référence, et le log « redimensionnée » donne les pixels réels.
        auto window = levain::platform::createWindow("Levain", 1920, 1080);
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

        const levain::render::SamplerSettings sampler{.maxAnisotropy = options->maxAnisotropy};
        levain::core::log("sandbox", levain::core::LogLevel::Info, "filtrage anisotrope : {}",
                          levain::render::clampAnisotropy(sampler.maxAnisotropy));
        auto scene = createDemoScene(*gpu, sampler);
        if (!scene)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              scene.error().message);
            return 1;
        }

        // Les liaisons d'entrée : changer une touche dans data/input.cfg ne demande aucune
        // recompilation (ADR-0017). Un nom inconnu échoue ici, avec son numéro de ligne.
        auto bindings = levain::input::loadBindings(LEVAIN_DATA_DIR "/input.cfg");
        if (!bindings)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              bindings.error().message);
            return 1;
        }
        const auto actions = cameraActionsOf(*bindings);
        if (!actions)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              actions.error().message);
            return 1;
        }

        levain::core::log("sandbox", levain::core::LogLevel::Info,
                          "liaisons : {} actions et {} axes (data/input.cfg) ; clic droit pour "
                          "regarder, ZQSD ou WASD pour avancer",
                          bindings->actions.size(), bindings->axes.size());

        runMainLoop(*window, *gpu, *scene, options->loopSeconds, *bindings, *actions);
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
