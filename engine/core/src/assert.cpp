#include "levain/core/assert.hpp"

#include "levain/core/log.hpp"

namespace levain::core
{

void reportFailedAssert(std::string_view expression, std::string_view message,
                        const std::source_location& where)
{
    log("assert", LogLevel::Critical, "assertion violée : {}\n  message  : {}\n  {}:{} ({})",
        expression, message, where.file_name(), where.line(), where.function_name());
}

} // namespace levain::core
