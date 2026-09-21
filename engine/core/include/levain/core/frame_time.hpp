#pragma once

#include <limits>
#include <optional>

namespace levain::core
{

/// Frame time sur une période, en millisecondes.
///
/// Le maximum compte autant que la moyenne : une saccade de 50 ms au milieu de 999 frames à
/// 1 ms ne déplace la moyenne que de 0,05 ms, mais elle se voit à l'écran. Le maximum, lui, la
/// montre tout de suite.
struct FrameTimeSummary
{
    double averageMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
    int frameCount = 0;
};

/// Frames accumulées depuis le dernier résumé.
struct FrameTimeAccumulator
{
    double elapsedSeconds = 0.0;
    /// Infini au départ, pour que la première frame devienne le minimum. Un départ à zéro
    /// afficherait un minimum de 0 ms pour toujours.
    double minSeconds = std::numeric_limits<double>::infinity();
    double maxSeconds = 0.0;
    int frameCount = 0;
};

/// Ajoute une frame. Dès que les frames accumulées couvrent `periodSeconds`, renvoie leur
/// résumé et repart de zéro ; sinon ne renvoie rien.
///
/// Une période fixe plutôt qu'une moyenne glissante : un chiffre qui change une fois par
/// seconde se lit, un chiffre qui change à chaque frame ne se lit pas.
[[nodiscard]] std::optional<FrameTimeSummary>
recordFrame(FrameTimeAccumulator& accumulator, double frameSeconds, double periodSeconds);

} // namespace levain::core
