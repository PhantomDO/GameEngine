#pragma once

#include <cstddef>
#include <memory>

namespace levain::core
{

/// Arène qui n'avance qu'en montant, et qu'on vide d'un coup.
///
/// Rendre la mémoire bloc par bloc est impossible : le seul moyen de libérer est `reset()`,
/// qui rend tout d'un seul coup et en temps constant. C'est exactement ce qu'il faut pour
/// les données qui ne vivent qu'une frame — listes de rendu, résultats de culling,
/// tampons intermédiaires — et c'est pour ça que les moteurs n'utilisent pas `malloc` dans
/// la boucle de jeu : `malloc` cherche un bloc libre, tient des métadonnées par allocation
/// et se fragmente. Ici, allouer, c'est ajouter à un entier.
///
/// Équivalent : `FMemStack` d'Unreal.
class LinearAllocator
{
public:
    explicit LinearAllocator(std::size_t capacity);

    /// Réserve `size` octets alignés sur `alignment`.
    ///
    /// Renvoie **`nullptr`** si la capacité restante ne suffit pas. C'est une entorse
    /// assumée à l'ADR-0008, qui voudrait un `Result` : construire le message d'erreur
    /// allouerait, dans un allocateur, sur le chemin d'échec. `nullptr` est de surcroît le
    /// contrat que tout le monde connaît, celui de `malloc` et de `operator new(nothrow)`.
    ///
    /// `alignment` doit être une puissance de deux : c'est un bug si ce n'est pas le cas.
    [[nodiscard]] void* allocate(std::size_t size,
                                 std::size_t alignment = alignof(std::max_align_t));

    /// Rend toute la mémoire d'un coup. Les pointeurs déjà distribués deviennent invalides.
    void reset() noexcept;

    [[nodiscard]] std::size_t usedBytes() const noexcept { return m_offset; }

    [[nodiscard]] std::size_t capacity() const noexcept { return m_capacity; }

    [[nodiscard]] std::size_t remainingBytes() const noexcept { return m_capacity - m_offset; }

private:
    std::unique_ptr<std::byte[]> m_buffer;
    std::size_t m_capacity;
    std::size_t m_offset = 0;
};

/// Arrondit `value` au multiple supérieur de `alignment`, qui doit être une puissance de
/// deux. Isolée et nommée parce que c'est le calcul que tout le monde écrit de travers.
[[nodiscard]] std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept;

/// Vrai si `value` est une puissance de deux non nulle.
[[nodiscard]] bool isPowerOfTwo(std::size_t value) noexcept;

} // namespace levain::core
