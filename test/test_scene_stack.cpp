#include <gtest/gtest.h>
#include <openitup/scene/scene_stack.h>

using namespace openitup;

// Mock scene that logs all lifecycle calls
class MockScene : public Scene {
public:
    MockScene(std::vector<std::string>* log, const std::string& name)
        : log_(log), name_(name) {}

    void on_enter() override { log_->push_back(name_ + "::on_enter"); }
    void on_exit() override { log_->push_back(name_ + "::on_exit"); }
    void on_pause() override { log_->push_back(name_ + "::on_pause"); }
    void on_resume() override { log_->push_back(name_ + "::on_resume"); }
    void update(double dt) override { log_->push_back(name_ + "::update"); }
    void handle_input(const InputSnapshot& input) override {
        log_->push_back(name_ + "::handle_input");
    }
    void render() override { log_->push_back(name_ + "::render"); }

private:
    std::vector<std::string>* log_;
    std::string name_;
};

TEST(SceneStackTest, PushSceneCreatesOverlay) {
    // US-SCN-001 SC1
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.push(std::make_unique<MockScene>(&log, "B"));

    EXPECT_EQ(stack.size(), 2);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::on_pause");
    EXPECT_EQ(log[1], "B::on_enter");

    // Verify input routes to B
    log.clear();
    InputSnapshot input;
    stack.handle_input(input);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::handle_input");
}

TEST(SceneStackTest, PopSceneReturnsToPrevious) {
    // US-SCN-001 SC2
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();

    stack.pop();

    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "B::on_exit");
    EXPECT_EQ(log[1], "A::on_resume");

    // Verify input now routes to A
    log.clear();
    InputSnapshot input;
    stack.handle_input(input);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::handle_input");
}

TEST(SceneStackTest, ReplaceSceneTransitions) {
    // US-SCN-001 SC3
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.replace(std::make_unique<MockScene>(&log, "B"));

    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::on_exit");
    EXPECT_EQ(log[1], "B::on_enter");

    // No pause or resume calls (replace, not overlay)
    for (const auto& entry : log) {
        EXPECT_TRUE(entry.find("pause") == std::string::npos);
        EXPECT_TRUE(entry.find("resume") == std::string::npos);
    }
}

TEST(SceneStackTest, RenderOrderBottomToTop) {
    // US-SCN-001 SC4
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    stack.push(std::make_unique<MockScene>(&log, "C"));
    log.clear();

    stack.render();

    ASSERT_EQ(log.size(), 3);
    EXPECT_EQ(log[0], "A::render");
    EXPECT_EQ(log[1], "B::render");
    EXPECT_EQ(log[2], "C::render");
}

TEST(SceneStackTest, OnlyTopSceneReceivesUpdate) {
    // US-SCN-001 SC5
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();

    stack.update(1.0 / 60.0);

    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::update");
}

TEST(SceneStackTest, EmptyStackIsValid) {
    // US-SCN-001 SC6
    SceneStack stack;

    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);

    // All operations should be no-ops, no crashes
    stack.update(1.0 / 60.0);
    InputSnapshot input;
    stack.handle_input(input);
    stack.render();
    stack.pop();

    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, LifecycleOnEnterCalledOnPush) {
    // US-SCN-002 SC1
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));

    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_enter");
}

TEST(SceneStackTest, LifecycleOnExitCalledOnPop) {
    // US-SCN-002 SC2
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.pop();

    ASSERT_GE(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_exit");
    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, LifecycleOnPauseCalledOnOverlay) {
    // US-SCN-002 SC3
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.push(std::make_unique<MockScene>(&log, "B"));

    ASSERT_GE(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_pause");
}

TEST(SceneStackTest, LifecycleOnResumeCalledOnUncover) {
    // US-SCN-002 SC4
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();

    stack.pop();

    ASSERT_GE(log.size(), 2);
    EXPECT_EQ(log[1], "A::on_resume");
}

TEST(SceneStackTest, UpdateAndRenderCalledEachFrame) {
    // US-SCN-002 SC5
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.update(1.0 / 60.0);
    stack.render();

    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "A::update");
    EXPECT_EQ(log[1], "A::render");
}

TEST(SceneStackTest, InputRoutedToTopScene) {
    // US-SCN-002 SC6
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    stack.push(std::make_unique<MockScene>(&log, "B"));
    log.clear();

    InputSnapshot input;
    stack.handle_input(input);

    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "B::handle_input");
}

TEST(SceneStackTest, DestructorCallsOnExitOnRemainingScenes) {
    std::vector<std::string> log;

    {
        SceneStack stack;
        stack.push(std::make_unique<MockScene>(&log, "A"));
        stack.push(std::make_unique<MockScene>(&log, "B"));
        log.clear();
        // Destructor runs here
    }

    // Destructor pops all scenes, which calls on_exit on each
    // Pop B: B::on_exit, A::on_resume
    // Pop A: A::on_exit
    ASSERT_GE(log.size(), 3);
    EXPECT_EQ(log[0], "B::on_exit");
    EXPECT_EQ(log[1], "A::on_resume");
    EXPECT_EQ(log[2], "A::on_exit");
}

TEST(SceneStackTest, ReplaceOnEmptyStackEquivalentToPush) {
    std::vector<std::string> log;
    SceneStack stack;

    EXPECT_TRUE(stack.empty());

    stack.replace(std::make_unique<MockScene>(&log, "A"));

    EXPECT_EQ(stack.size(), 1);
    ASSERT_EQ(log.size(), 1);
    EXPECT_EQ(log[0], "A::on_enter");
}

TEST(SceneStackTest, PushNullptrIsNoOp) {
    SceneStack stack;
    stack.push(nullptr);

    EXPECT_TRUE(stack.empty());
}

TEST(SceneStackTest, ReplaceNullptrIsNoOp) {
    std::vector<std::string> log;
    SceneStack stack;

    stack.push(std::make_unique<MockScene>(&log, "A"));
    log.clear();

    stack.replace(nullptr);

    // Original scene should still be there
    EXPECT_EQ(stack.size(), 1);
    EXPECT_TRUE(log.empty());  // No lifecycle calls
}
