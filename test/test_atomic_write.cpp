#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <openitup/data/atomic_write.h>

namespace {

class AtomicWriteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "test_atomic_write";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
};

TEST_F(AtomicWriteTest, CreatesNewFile) {
    auto target = test_dir_ / "new_file.txt";
    std::string content = "Hello, World!";

    bool success = openitup::data::atomic_write_file(target, content);
    ASSERT_TRUE(success);

    // Verify file exists and has correct content
    EXPECT_TRUE(std::filesystem::exists(target));

    std::ifstream in(target);
    std::string actual_content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(content, actual_content);
}

TEST_F(AtomicWriteTest, OverwritesExistingFile) {
    auto target = test_dir_ / "existing_file.txt";

    // Create initial file with old content
    {
        std::ofstream out(target);
        out << "Old content";
    }

    // Overwrite with new content
    std::string new_content = "New content";
    bool success = openitup::data::atomic_write_file(target, new_content);
    ASSERT_TRUE(success);

    // Verify file has new content
    std::ifstream in(target);
    std::string actual_content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(new_content, actual_content);
}

TEST_F(AtomicWriteTest, NoTempFileLeftBehind) {
    auto target = test_dir_ / "file.txt";
    auto temp = test_dir_ / "file.txt.tmp";
    std::string content = "Test content";

    bool success = openitup::data::atomic_write_file(target, content);
    ASSERT_TRUE(success);

    // Verify temp file was cleaned up
    EXPECT_FALSE(std::filesystem::exists(temp));
}

TEST_F(AtomicWriteTest, HandlesEmptyContent) {
    auto target = test_dir_ / "empty_file.txt";
    std::string content = "";

    bool success = openitup::data::atomic_write_file(target, content);
    ASSERT_TRUE(success);

    EXPECT_TRUE(std::filesystem::exists(target));
    EXPECT_EQ(std::filesystem::file_size(target), 0);
}

TEST_F(AtomicWriteTest, HandlesLargeContent) {
    auto target = test_dir_ / "large_file.txt";
    std::string content(1024 * 1024, 'X');  // 1MB of 'X'

    bool success = openitup::data::atomic_write_file(target, content);
    ASSERT_TRUE(success);

    EXPECT_EQ(std::filesystem::file_size(target), content.size());
}

TEST_F(AtomicWriteTest, FailsOnInvalidPath) {
    // Try to write to a non-existent directory
    auto target = test_dir_ / "nonexistent_dir" / "file.txt";
    std::string content = "Test";

    bool success = openitup::data::atomic_write_file(target, content);
    EXPECT_FALSE(success);

    // File should not exist
    EXPECT_FALSE(std::filesystem::exists(target));
}

} // namespace
