#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Start;
class Game;

class User {
public:
    struct Record {
        std::string username;
        std::string password;
        char stage = '0';
    };

    User(const std::string& filepath = "users.bin");
    ~User();

    // file IO
    bool write();                 // write current records -> file
    bool read();                  // read records <- file

    // initialize the in-memory current user from file (first record)
    void Init();

    // authentication / session management
    bool login(const std::string& user, const std::string& pass);
    void logout();                // save current user into first record (persist)
    bool signin(const std::string& user, const std::string& pass);

    // accessors
    std::string getUsername() const;
    std::string getPassword() const;
    char getStage() const;
    void setStage(char s);

    // persisted sign flag: 0 = last session not explicitly logged out, 1 = explicitly logged out
    void setSign(bool s);
    bool getSign() const;

    // all records (useful for external tools)
    const std::vector<Record>& all() const { return records; }

private:
    std::string path;
    std::vector<Record> records;

    // currently active user info (copied from records.front() when logged in)
    std::string username;
    std::string password;
    char stage = '0';

    uint8_t sign = 0; // persisted flag stored in the binary file

    // helpers
    bool writeString(std::ofstream& ofs, const std::string& s);
    bool readString(std::ifstream& ifs, std::string& s);
};