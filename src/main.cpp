#include "app/App.hpp"
#include "logging/Logger.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        cweman::App app;
        app.run();
    } catch (const std::exception& e) {
        cweman::Logger::instance().critical(e.what());
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
