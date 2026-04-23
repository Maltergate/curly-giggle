// Placeholder main — will be replaced by the `hello-world-imgui` todo.
// This file intentionally avoids any UI/Metal initialisation so that the
// cmake-infra todo can be validated with a simple compile + run.

#include <cstdio>
#include "gnc_viz/version.hpp"

int main()
{
    std::printf("GNC Viz v%s — cmake-infra OK\n", gnc_viz::version());
    return 0;
}
