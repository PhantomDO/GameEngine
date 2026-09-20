#include "levain/core/linear_allocator.hpp"

#include <cstdint>

#include "levain/core/assert.hpp"

namespace levain::core
{

bool isPowerOfTwo(std::size_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept
{
    LEVAIN_ASSERT(isPowerOfTwo(alignment), "l'alignement doit être une puissance de deux");

    return (value + alignment - 1) & ~(alignment - 1);
}

LinearAllocator::LinearAllocator(std::size_t capacity)
    : m_buffer{std::make_unique<std::byte[]>(capacity)}, m_capacity{capacity}
{
}

void* LinearAllocator::allocate(std::size_t size, std::size_t alignment)
{
    LEVAIN_ASSERT(isPowerOfTwo(alignment), "l'alignement doit être une puissance de deux");

    // L'alignement porte sur l'**adresse réelle**, pas sur l'offset dans le tampon :
    // `make_unique<std::byte[]>` ne garantit que l'alignement par défaut des nouvelles
    // allocations (16 octets ici). Aligner l'offset seul rendrait une adresse mal alignée
    // dès qu'on demande davantage — et silencieusement, puisque ça « marche » sur x86.
    const auto base = reinterpret_cast<std::uintptr_t>(m_buffer.get());
    const std::uintptr_t alignedAddress = alignUp(base + m_offset, alignment);
    const std::size_t alignedOffset = static_cast<std::size_t>(alignedAddress - base);

    // Le test porte sur la somme alignée, pas sur `size` seul : c'est le remplissage
    // d'alignement qui fait déborder une arène presque pleine, et l'oublier donne un
    // pointeur hors du tampon au lieu d'un nullptr.
    if (alignedOffset + size > m_capacity)
    {
        return nullptr;
    }

    m_offset = alignedOffset + size;

    return m_buffer.get() + alignedOffset;
}

void LinearAllocator::reset() noexcept
{
    m_offset = 0;
}

} // namespace levain::core
