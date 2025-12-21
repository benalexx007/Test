// Header providing a minimal user-profile abstraction. The `User`
// class encapsulates a list of persisted login records as well as
// a lightweight session model used by the application.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Start;
class Game;

class User {
public:
    // A persisted record representing a single user account.
    // The `stage` field stores the furthest completed stage for the user.
    struct Record {
        std::string username;
        std::string password;
        char stage = '0';
    };

    // Construct a User manager which stores data in the given file path.
    // The default path is `users.bin` in the working directory.
    User(const std::string& filepath = "users.bin");
    ~User();

    // Binary file I/O methods: write the `records` vector to disk and
    // restore it from disk. These functions encapsulate the storage
    // format (a sign flag plus a count followed by length-prefixed
    // strings) used by the application.
    bool write();                 // write current records -> file
    bool read();                  // read records <- file

    // Initialize in-memory session information from the stored file.
    // This method loads the first record as the current active user
    // unless the persisted `sign` flag indicates an explicit logout.
    void Init();

    // Authentication and session management.
    bool login(const std::string& user, const std::string& pass);
    void logout();                // persist the current user as first record
    bool signin(const std::string& user, const std::string& pass);

    // Accessors and mutators for the session fields.
    std::string getUsername() const;
    std::string getPassword() const;
    char getStage() const;
    void setStage(char s);
    bool updateStage(char newStage);

    // Persisted sign flag semantics: when `sign` equals 1 the user
    // explicitly logged out during the last session; a value of 0
    // indicates either an active session or no explicit logout.
    void setSign(bool s);
    bool getSign() const;
    bool isLoggedIn() const;

    // Return reference to internal records for external inspection.
    const std::vector<Record>& all() const { return records; }

private:
    // Storage path for the user database and the in-memory record list.
    std::string path;
    std::vector<Record> records;

    // Runtime session state copied from records.front() when a user
    // is active. These fields are not automatically written to disk
    // until `logout()` or explicit write operations occur.
    std::string username;
    std::string password;
    char stage = '0';

    // Persisted sign flag representation stored in the binary file.
    uint8_t sign = 0;

    // File helper methods that encode/decode length-prefixed strings
    // used by the custom binary format implemented in `write()`/`read()`.
    bool writeString(std::ofstream& ofs, const std::string& s);
    bool readString(std::ifstream& ifs, std::string& s);
};