#pragma once

#include "app/State.hpp"
#include "data/Repository.hpp"

#include <functional>
#include <string>

#include <ftxui/component/component.hpp>

namespace cweman {

using CommandCallback = std::function<void(const std::string&)>;

// Creates the root layout component that wires together all panes.
ftxui::Component RootLayout(AppState& state, Repository& repo,
                            CommandCallback on_command);

} // namespace cweman
