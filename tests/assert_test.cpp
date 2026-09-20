#include <ostream>

#include <doctest/doctest.h>

#include "levain/core/assert.hpp"

// LEVAIN_ASSERT déclenche un arrêt dans le débogueur : impossible de le tester en le
// faisant échouer. Ce qui se teste, et qui est le vrai piège, c'est que LEVAIN_VERIFY
// évalue toujours son expression — y compris en Release, où LEVAIN_ASSERT disparaît.
// Ce test tourne dans les deux configurations en CI, donc il attrape une régression qui
// ne serait visible qu'en Release.

TEST_CASE("LEVAIN_VERIFY évalue son expression dans toutes les configurations")
{
    int calls = 0;
    const auto incrementAndSucceed = [&calls]
    {
        ++calls;
        return true;
    };

    LEVAIN_VERIFY(incrementAndSucceed(), "doit être évaluée même hors Debug");

    CHECK(calls == 1);
}

TEST_CASE("LEVAIN_ASSERT n'exécute rien quand la condition tient")
{
    int calls = 0;
    const auto incrementAndSucceed = [&calls]
    {
        ++calls;
        return true;
    };

    LEVAIN_ASSERT(incrementAndSucceed(), "condition vraie");

#if LEVAIN_ASSERTIONS_ENABLED
    CHECK(calls == 1);
#else
    CHECK(calls == 0); // compilée hors du binaire : l'expression n'est pas évaluée
#endif
}
