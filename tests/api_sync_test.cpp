#include "TestUtils.hpp"
#include "api/Sync.hpp"

#include <gtest/gtest.h>

#include <fstream>

namespace cweman {

TEST(SyncTest, WriteAndReadTimestampRoundTrip) {
    auto temp_home = test::make_temp_dir("cwe-man-sync-home");
    test::ScopedHome scoped_home(temp_home);

    EXPECT_TRUE(Sync::read_sync_timestamp().empty());
    Sync::write_sync_timestamp();

    auto ts = Sync::read_sync_timestamp();
    EXPECT_FALSE(ts.empty());
    EXPECT_NE(ts.find("CDT"), std::string::npos);
}

TEST(SyncTest, SyncNeededDetectsRecentAndOldTimestamps) {
    auto temp_home = test::make_temp_dir("cwe-man-sync-needed");
    test::ScopedHome scoped_home(temp_home);

    std::filesystem::create_directories(temp_home / ".cwe-man");
    {
        std::ofstream f(temp_home / ".cwe-man" / "sync_state.txt");
        f << "2099-01-01 00:00:00 CDT\n";
    }
    EXPECT_FALSE(Sync::sync_needed(30));

    {
        std::ofstream f(temp_home / ".cwe-man" / "sync_state.txt");
        f << "2000-01-01 00:00:00 CDT\n";
    }
    EXPECT_TRUE(Sync::sync_needed(30));
}

TEST(SyncTest, InvalidTimestampFormatFallsBackToNeeded) {
    auto temp_home = test::make_temp_dir("cwe-man-sync-invalid");
    test::ScopedHome scoped_home(temp_home);

    std::filesystem::create_directories(temp_home / ".cwe-man");
    {
        std::ofstream f(temp_home / ".cwe-man" / "sync_state.txt");
        f << "not-a-timestamp\n";
    }
    EXPECT_TRUE(Sync::sync_needed(30));
}

} // namespace cweman
