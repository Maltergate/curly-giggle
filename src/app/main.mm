// main.mm — application entry point (10 lines)
#include "fastscope/application.hpp"

int main()
{
    fastscope::Application app;
    if (!app.init()) return 1;
    app.run();
    return 0;
}
