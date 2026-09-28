// tests/p2/test_p2.cpp
//
// Test suite for ECE 309 Project 2

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Simple input source for testing the Harness
class TestInput : public InputSource {
public:
    explicit TestInput(const std::vector<std::string>& lines)
        : lines_(lines) {
    }

    std::string read_line() override {
        if (index_ >= lines_.size()) {
            eof_ = true;
            return "";
        }

        return lines_[index_++];
    }

    bool is_eof() const override {
        return eof_;
    }

private:
    std::vector<std::string> lines_;
    std::size_t index_ = 0;
    bool eof_ = false;
};

// Simple output sink for testing the Harness
class TestOutput : public OutputSink {
public:
    void write(std::string_view text) override {
        output += std::string(text);
    }

    std::string output;
};

int main() {

    // Test 1: Empty Conversation
    {
        Conversation conv;

        assert(conv.size() == 0);
        assert(conv.begin() == conv.end());
    }

    // Test 2: Append and access a message
    {
        Conversation conv;

        conv.append(Message(Role::User, "Hello"));

        assert(conv.size() == 1);
        assert(conv.at(0).role() == Role::User);
        assert(conv.at(0).content() == "Hello");
    }

    // Test 3: System message is first
    {
        Conversation conv;

        conv.append(Message(Role::System, "You are helpful."));
        conv.append(Message(Role::User, "Hello"));
        conv.append(Message(Role::Assistant, "Hi!"));

        assert(conv.size() == 3);
        assert(conv.at(0).role() == Role::System);
        assert(conv.at(1).role() == Role::User);
        assert(conv.at(2).role() == Role::Assistant);
    }

    // Test 4: Copy constructor
    {
        Conversation original;

        original.append(Message(Role::User, "Hello"));
        original.append(Message(Role::Assistant, "Hi!"));

        Conversation copy(original);

        assert(copy.size() == 2);
        assert(copy.at(0).content() == "Hello");
        assert(copy.at(1).content() == "Hi!");
    }

    // Test 5: Copy is independent
    {
        Conversation original;

        original.append(Message(Role::User, "Hello"));

        Conversation copy(original);

        copy.append(Message(Role::Assistant, "Hi!"));

        assert(original.size() == 1);
        assert(copy.size() == 2);
    }

    // Test 6: Copy assignment
    {
        Conversation original;

        original.append(Message(Role::User, "Hello"));
        original.append(Message(Role::Assistant, "Hi!"));

        Conversation copy;

        copy = original;

        assert(copy.size() == 2);
        assert(copy.at(0).content() == "Hello");
        assert(copy.at(1).content() == "Hi!");
    }

    // Test 7: Move constructor
    {
        Conversation original;

        original.append(Message(Role::User, "Hello"));
        original.append(Message(Role::Assistant, "Hi!"));

        Conversation moved(std::move(original));

        assert(moved.size() == 2);
        assert(moved.at(0).content() == "Hello");
        assert(moved.at(1).content() == "Hi!");

        assert(original.size() == 0);
        assert(original.begin() == original.end());
    }

    // Test 8: Move assignment
    {
        Conversation original;

        original.append(Message(Role::User, "Hello"));
        original.append(Message(Role::Assistant, "Hi!"));

        Conversation moved;

        moved = std::move(original);

        assert(moved.size() == 2);
        assert(moved.at(0).content() == "Hello");
        assert(moved.at(1).content() == "Hi!");

        assert(original.size() == 0);
        assert(original.begin() == original.end());
    }

    // Test 9: Growth behavior
    {
        Conversation conv;

        for (int i = 0; i < 100; i++) {
            conv.append(Message(Role::User, "message"));
        }

        assert(conv.size() == 100);

        for (int i = 0; i < 100; i++) {
            assert(conv.at(i).content() == "message");
        }
    }
        // Test 9a: Scanner clean text
    {
        SentinelScanner scanner("<|end_conversation|>");

        auto result = scanner.feed("Hello world");

        assert(result.sentinel_found == false);

        auto final_result = scanner.flush();

        assert(final_result.sentinel_found == false);
        assert(result.safe_text + final_result.safe_text == "Hello world");
    }

        // Test 9b: Scanner detects sentinel split at every boundary
    {
        std::string sentinel = "<|end_conversation|>";

        for (std::size_t split = 1; split < sentinel.size(); split++) {
            SentinelScanner scanner(sentinel);

            std::string first = "Hello " + sentinel.substr(0, split);
            std::string second = sentinel.substr(split);

            auto result1 = scanner.feed(first);

            assert(result1.sentinel_found == false);

            auto result2 = scanner.feed(second);

            assert(result2.sentinel_found == true);

            assert(result1.safe_text + result2.safe_text == "Hello ");
        }
    }
    // Test 9c: Scanner handles false alarms
    {
        SentinelScanner scanner("<|end_conversation|>");

        auto result = scanner.feed("Hello <|end_conversatio");

        assert(result.sentinel_found == false);

        auto final_result = scanner.flush();

        assert(final_result.sentinel_found == false);
        assert(result.safe_text + final_result.safe_text ==
               "Hello <|end_conversatio");
    }

    // Test 9d: Scanner keeps pending data bounded
    {
        SentinelScanner scanner("<|end_conversation|>");

        std::string long_text(1000, 'A');

        auto result = scanner.feed(long_text);

        assert(result.sentinel_found == false);
        assert(result.safe_text.size() > 900);

        auto final_result = scanner.flush();

        assert(final_result.sentinel_found == false);
        assert(result.safe_text.size() + final_result.safe_text.size() ==
               long_text.size());
    }
    // Test 10: Harness Turn Limit
    {
        const char* filename = "test_turn_limit.txt";

        std::ofstream file(filename);
        file << "hello\n";
        file << "---\n";
        file << "role: assistant\n";
        file << "Hi there!\n";
        file << "---\n";
        file.close();

        std::unique_ptr<ModelClient> model =
            std::make_unique<ReplayModelClient>(filename);

        HarnessConfig cfg;
        cfg.max_turns = 1;

        Harness harness(std::move(model), cfg);

        TestInput input({"hello"});
        TestOutput output;

        StopReason reason = harness.run(input, output);

        assert(reason.kind == StopReason::Kind::TurnLimit);
        assert(harness.conversation().size() == 2);

        std::remove(filename);
    }

    // Test 11: Harness Sentinel Halt
    {
        const char* filename = "test_sentinel.txt";

        std::ofstream file(filename);
        file << "role: assistant\n";
        file << "This is the answer <|end_conversation|> ignored text\n";
        file << "---\n";
        file.close();

        std::unique_ptr<ModelClient> model =
            std::make_unique<ReplayModelClient>(filename);

        HarnessConfig cfg;
        cfg.max_turns = 20;

        Harness harness(std::move(model), cfg);

        TestInput input({"hello"});
        TestOutput output;

        StopReason reason = harness.run(input, output);

        assert(reason.kind == StopReason::Kind::Sentinel);

        assert(output.output.find("<|end_conversation|>") == std::string::npos);

        assert(harness.conversation().size() == 2);
        assert(harness.conversation().at(1).content() ==
               "This is the answer <|end_conversation|>");

        std::remove(filename);
    }

    // Test 12: Transcript Round-Trip
    {
        const char* filename = "test_round_trip.txt";

        {
            std::ofstream file(filename);

            file << "role: system\n";
            file << "You are helpful.\n";
            file << "---\n";

            file << "role: user\n";
            file << "Hello\n";
            file << "---\n";

            file << "role: assistant\n";
            file << "Hi there!\n";
            file << "---\n";

            file << "role: user\n";
            file << "How are you?\n";
            file << "---\n";

            file << "role: assistant\n";
            file << "I am doing well. <|end_conversation|>\n";
            file << "---\n";
        }

        ReplayModelClient replay(filename);

        assert(replay.system_message() == "You are helpful.");

        Conversation conv;
        conv.append(Message(Role::System, replay.system_message()));
        conv.append(Message(Role::User, "Hello"));

        Message first_reply = replay.generate(conv);

        assert(first_reply.role() == Role::Assistant);
        assert(first_reply.content() == "Hi there!");

        conv.append(first_reply);
        conv.append(Message(Role::User, "How are you?"));

        Message second_reply = replay.generate(conv);

        assert(second_reply.role() == Role::Assistant);
        assert(second_reply.content() ==
               "I am doing well. <|end_conversation|>");

        std::remove(filename);
    }

    return 0;
}