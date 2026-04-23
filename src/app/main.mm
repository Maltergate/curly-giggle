// main.mm — application entry point (10 lines)
#include "gnc_viz/application.hpp"

int main()
{
    gnc_viz::Application app;
    if (!app.init()) return 1;
    app.run();
    return 0;
}
