#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

enum class Role {
    System,
    User,
    Assistant
};

class Message {
public:
    Message();

    Message(Role role, std::string content);

    Role role() const noexcept;

    const std::string& content() const noexcept;

private:
    Role role_;
    std::string content_;
};

#endif