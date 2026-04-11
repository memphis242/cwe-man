#include "TestUtils.hpp"
#include "app/State.hpp"
#include "data/Database.hpp"
#include "data/Repository.hpp"
#include "ui/Layout.hpp"
#include "ui/TreePane.hpp"

#include <ftxui/component/event.hpp>
#include <gtest/gtest.h>

#include <string>

namespace cweman {

TEST(LayoutEventTest, TreeQTriggersQuitCommandCallback) {
    auto temp_dir = test::make_temp_dir("cwe-man-layout");
    Database db(temp_dir / "layout.db");
    Repository repo(db);
    repo.initialize_schema();

    AppState state;
    state.mode = AppMode::Tree;

    std::string last_cmd;
    auto component = RootLayout(state, repo, [&](const std::string& cmd) { last_cmd = cmd; });

    EXPECT_TRUE(component->OnEvent(ftxui::Event::Character('q')));
    EXPECT_EQ(last_cmd, "q");
}

TEST(LayoutEventTest, ColonEntersCommandModeAndEnterDispatchesCommand) {
    auto temp_dir = test::make_temp_dir("cwe-man-layout-cmd");
    Database db(temp_dir / "layout.db");
    Repository repo(db);
    repo.initialize_schema();

    AppState state;
    state.mode = AppMode::Tree;

    std::string last_cmd;
    auto component = RootLayout(state, repo, [&](const std::string& cmd) { last_cmd = cmd; });

    EXPECT_TRUE(component->OnEvent(ftxui::Event::Character(':')));
    EXPECT_EQ(state.mode, AppMode::Command);

    EXPECT_TRUE(component->OnEvent(ftxui::Event::Character('q')));
    EXPECT_TRUE(component->OnEvent(ftxui::Event::Return));

    EXPECT_EQ(last_cmd, "q");
    EXPECT_EQ(state.mode, AppMode::Tree);
}

TEST(LayoutEventTest, FilterQReturnsToTreeAndClearsFilterState) {
    auto temp_dir = test::make_temp_dir("cwe-man-layout-filter");
    Database db(temp_dir / "layout.db");
    Repository repo(db);
    repo.initialize_schema();

    AppState state;
    state.mode = AppMode::Filter;
    state.filter_active = true;
    state.filter_navigation_active = true;
    state.filter_query = "xss";
    state.filter_matches = {1, 2, 3};

    auto component = RootLayout(state, repo, [&](const std::string&) {});

    EXPECT_TRUE(component->OnEvent(ftxui::Event::Character('q')));
    EXPECT_EQ(state.mode, AppMode::Tree);
    EXPECT_FALSE(state.filter_active);
    EXPECT_FALSE(state.filter_navigation_active);
    EXPECT_TRUE(state.filter_query.empty());
    EXPECT_TRUE(state.filter_matches.empty());
}

} // namespace cweman
