#pragma once
#include <string>
#include <vector>

class Game; // forward

class User {
public:
    struct Record {
        std::string username;
        std::string password;
        int stage = -1;
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
    int getStage() const;
    void setStage(int s);

    // all records (useful for external tools)
    const std::vector<Record>& all() const { return records; }

private:
    std::string path;
    std::vector<Record> records;

    // currently active user info (copied from records.front() when logged in)
    std::string username;
    std::string password;
    int stage = -1;

    // helpers
    static bool writeString(std::ofstream& ofs, const std::string& s);
    static bool readString(std::ifstream& ifs, std::string& s);
};