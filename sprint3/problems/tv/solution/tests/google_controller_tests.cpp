#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "../src/controller.h"

using namespace std::literals;

class ControllerWithTurnedOffTV : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_FALSE(tv_.IsTurnedOn());
    }

    void RunMenuCommand(std::string command) {
        input_.str(std::move(command));
        input_.clear();
        menu_.Run();
    }

    void ExpectExtraArgumentsErrorInOutput(std::string_view command) const {
        ExpectOutput(
            "Error: the "s.append(command).append(" command does not require any arguments\n"sv));
    }

    void ExpectErrorInOutput(std::string_view message) const {
        ExpectOutput(message);
    }

    void ExpectEmptyOutput() const {
        ExpectOutput({});
    }

    void ExpectOutput(std::string_view expected) const {
        // В g++ 10.3 не реализован метод ostringstream::view(), поэтому приходится
        // использовать метод str()
        EXPECT_EQ(output_.str(), std::string{expected});
    }

    TV tv_;
    std::istringstream input_;
    std::ostringstream output_;
    Menu menu_{input_, output_};
    Controller controller_{tv_, menu_};
};

TEST_F(ControllerWithTurnedOffTV, OnInfoCommandPrintsThatTVIsOff) {
    input_.str("Info"s);
    menu_.Run();
    ExpectOutput("TV is turned off\n"sv);
    EXPECT_FALSE(tv_.IsTurnedOn());
}
TEST_F(ControllerWithTurnedOffTV, OnInfoCommandPrintsErrorMessageIfCommandHasAnyArgs) {
    RunMenuCommand("Info some extra args"s);
    EXPECT_FALSE(tv_.IsTurnedOn());
    ExpectExtraArgumentsErrorInOutput("Info"sv);
}
TEST_F(ControllerWithTurnedOffTV, OnInfoCommandWithTrailingSpacesPrintsThatTVIsOff) {
    input_.str("Info  "s);
    menu_.Run();
    ExpectOutput("TV is turned off\n"sv);
}
TEST_F(ControllerWithTurnedOffTV, OnTurnOnCommandTurnsTVOn) {
    RunMenuCommand("TurnOn"s);
    EXPECT_TRUE(tv_.IsTurnedOn());
    ExpectEmptyOutput();
}
TEST_F(ControllerWithTurnedOffTV, OnTurnOnCommandPrintsErrorMessageIfCommandHasAnyArgs) {
    RunMenuCommand("TurnOn some extra args"s);
    EXPECT_FALSE(tv_.IsTurnedOn());
    ExpectExtraArgumentsErrorInOutput("TurnOn"sv);
}

TEST_F(ControllerWithTurnedOffTV, OnSelectChannelTurnedOffTv) {
    RunMenuCommand("SelectChannel 5"s);
    ExpectErrorInOutput("TV is turned off\n"s);
}

TEST_F(ControllerWithTurnedOffTV, OnPrevChannelTurnedOffTv) {
    RunMenuCommand("SelectPreviousChannel"s);
    ExpectErrorInOutput("TV is turned off\n"s);
}

/*
 * Протестируйте остальные аспекты поведения класса Controller, когда TV выключен
 */

//-----------------------------------------------------------------------------------

class ControllerWithTurnedOnTV : public ControllerWithTurnedOffTV {
protected:
    void SetUp() override {
        tv_.TurnOn();
    }
};

TEST_F(ControllerWithTurnedOnTV, OnTurnOffCommandTurnsTVOff) {
    RunMenuCommand("TurnOff"s);
    EXPECT_FALSE(tv_.IsTurnedOn());
    ExpectEmptyOutput();
}
TEST_F(ControllerWithTurnedOnTV, OnTurnOffCommandPrintsErrorMessageIfCommandHasAnyArgs) {
    RunMenuCommand("TurnOff some extra args"s);
    EXPECT_TRUE(tv_.IsTurnedOn());
    ExpectExtraArgumentsErrorInOutput("TurnOff"sv);
}
// Включите этот тест, после того, как реализуете метод TV::SelectChannel

TEST_F(ControllerWithTurnedOnTV, OnInfoPrintsCurrentChannel) {
    tv_.SelectChannel(42);
    RunMenuCommand("Info"s);
    ExpectOutput("TV is turned on\nChannel number is 42\n"sv);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelNoArgument) {
    RunMenuCommand("SelectChannel"s);
    ExpectErrorInOutput("Invalid channel\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelNotInt) {
    RunMenuCommand("SelectChannel gfh"s);
    ExpectErrorInOutput("Invalid channel\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelOutOfRangeZero) {
    RunMenuCommand("SelectChannel 0"s);
    ExpectErrorInOutput("Channel is out of range\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelOutOfRangeNegative) {
    RunMenuCommand("SelectChannel -1"s);
    ExpectErrorInOutput("Channel is out of range\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelOutOfRange100) {
    RunMenuCommand("SelectChannel 100"s);
    ExpectErrorInOutput("Channel is out of range\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectChannelOutOfRange200) {
    RunMenuCommand("SelectChannel 200"s);
    ExpectErrorInOutput("Channel is out of range\n"s);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectPreviousChannel1) {
    tv_.SelectChannel(42);
    RunMenuCommand("SelectPreviousChannel"s);
    RunMenuCommand("Info"s);
    ExpectOutput("TV is turned on\nChannel number is 1\n"sv);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectPreviousChannel2) {
    tv_.SelectChannel(42);
    tv_.SelectChannel(37);
    RunMenuCommand("SelectPreviousChannel"s);
    RunMenuCommand("Info"s);
    ExpectOutput("TV is turned on\nChannel number is 42\n"sv);
}

TEST_F(ControllerWithTurnedOnTV, OnSelectPreviousChannel3) {
    tv_.SelectChannel(42);
    tv_.SelectChannel(37);
    RunMenuCommand("SelectPreviousChannel"s);
    RunMenuCommand("SelectPreviousChannel"s);
    RunMenuCommand("Info"s);
    ExpectOutput("TV is turned on\nChannel number is 37\n"sv);
}

/*
 * Протестируйте остальные аспекты поведения класса Controller, когда TV включен
 */
