#include "user.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <algorithm>
User::User(const std::string& filepath) : path(filepath) {}
User::~User() {}

// Write all in-memory records to a binary file using a compact custom
// format: [sign flag][count][string len][string bytes]...[stage]. The
// function returns true if the underlying ofstream reports a good
// state upon completion.
bool User::write()
{
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "User::write - failed to open " << path << " for writing\n";
        return false;
    }

    // Persist the sign flag first so that Init() can detect explicit logout.
    ofs.write(reinterpret_cast<const char*>(&sign), sizeof(sign));

    // Write the number of records followed by each record's fields.
    uint64_t count = static_cast<uint64_t>(records.size());
    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& r : records) {
        if (!writeString(ofs, r.username)) return false;
        if (!writeString(ofs, r.password)) return false;
        ofs.write(reinterpret_cast<const char*>(&r.stage), sizeof(r.stage));
    }
    return ofs.good();
}

// Read persisted records from the binary file. On success the method
// repopulates `records` and `sign`. If the file does not exist the
// method returns false and leaves `records` empty.
bool User::read()
{
    records.clear();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        // No file present: treat as first-run state with no persisted sign.
        sign = 0;
        return false;
    }

    // Read persisted sign flag first for session semantics.
    ifs.read(reinterpret_cast<char*>(&sign), sizeof(sign));
    if (!ifs) { sign = 0; return false; }

    uint64_t count = 0;
    ifs.read(reinterpret_cast<char*>(&count), sizeof(count));
    for (uint64_t i = 0; i < count; ++i) {
        Record r;
        if (!readString(ifs, r.username)) return false;
        if (!readString(ifs, r.password)) return false;
        ifs.read(reinterpret_cast<char*>(&r.stage), sizeof(r.stage));
        if (!ifs) return false;
        records.push_back(std::move(r));
    }
    return true;
}

// Initialize session information from the stored records. If the
// persisted `sign` flag indicates an explicit logout (sign==1), the
// method will not auto-load credentials into the session.
void User::Init()
{
    if (!read() || records.empty()) {
        username.clear();
        password.clear();
        stage = '0';
        sign = 0;
        if (records.empty()) records.emplace_back();
        return;
    }

    // if sign==1 it means last session explicitly logged out -> do not auto-load credentials
    if (sign == 1) {
        username.clear();
        password.clear();
        stage = '0';
    } else {
        username = records.front().username;
        password = records.front().password;
        stage = records.front().stage;
    }
}

// Authenticate a user by comparing provided credentials against the
// stored records. On successful authentication the matching record is
// moved to the front of the list to represent the current session and
// the sign flag is cleared to indicate an active session.
bool User::login(const std::string& user, const std::string& pass)
{
    read();

    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].username == user && records[i].password == pass) {
            username = records[i].username;
            password = records[i].password;
            stage = records[i].stage;
            if (i != 0) std::swap(records[i], records[0]);
            sign = 0;
            write();
            return true;
        }
    }
    std::cout << "USERNAME OR PASSWORD IS INCORRECT\n";
    return false;
}

// Persist the current session into the first record and mark the
// persisted sign flag to indicate an explicit logout. This ensures
// that consecutive application launches will not automatically log
// the user in.
void User::logout()
{
    if (records.empty()) records.emplace_back();
    records.front().username = username;
    records.front().password = password;
    records.front().stage = stage;
    sign = 1;
    write();
}

// Register a new user account by appending a record and making it the
// active session. The function immediately persists the updated
// records to disk.
bool User::signin(const std::string& user, const std::string& pass)
{
    read();
    Record r;
    r.username = user;
    r.password = pass;
    r.stage = '0';
    records.push_back(r);
    if (records.size() > 1) std::swap(records.back(), records.front());
    username = records.front().username;
    password = records.front().password;
    stage = records.front().stage;
    sign = 0;
    return write();
}

// Accessors and mutators
std::string User::getUsername() const { return username; }
std::string User::getPassword() const { return password; }
char User::getStage() const { return stage; }
void User::setStage(char s) { stage = s; }

// Update the stored stage value in both memory and on disk. Valid
// stage characters are '0' through '3'.
bool User::updateStage(char newStage)
{
    if (newStage < '0' || newStage > '3') {
        std::cerr << "User::updateStage - invalid stage: " << newStage << "\n";
        return false;
    }
    stage = newStage;
    read();
    if (!records.empty()) {
        records.front().stage = newStage;
    } else {
        Record r;
        r.username = username;
        r.password = password;
        r.stage = newStage;
        records.push_back(r);
    }
    bool ok = write();
    if (ok) {
        std::cerr << "User::updateStage - updated stage to: " << newStage << "\n";
    } else {
        std::cerr << "User::updateStage - failed to write to file\n";
    }
    return ok;
}
void User::setSign(bool s) { sign = s ? 1 : 0; }
bool User::getSign() const { return sign != 0; }

// A user is considered logged in when the `username` field is not
// empty and the persisted sign flag is zero (which means the last
// session did not explicitly log out).
bool User::isLoggedIn() const
{
    return !username.empty() && sign == 0;
}

// Helper functions for writing and reading length-prefixed strings
// to and from binary streams. The read function enforces a maximum
// string length to prevent unbounded memory allocation when reading
// malformed or corrupted files.
bool User::writeString(std::ofstream& ofs, const std::string& s)
{
    uint64_t n = static_cast<uint64_t>(s.size());
    ofs.write(reinterpret_cast<const char*>(&n), sizeof(n));
    if (n) ofs.write(s.data(), static_cast<std::streamsize>(n));
    return ofs.good();
}

bool User::readString(std::ifstream& ifs, std::string& s)
{
    uint64_t n = 0;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(n));
    if (!ifs) return false;

    const uint64_t MAX_STRING_LEN = 1 << 20; // 1 MiB max per string
    if (n > MAX_STRING_LEN) {
        std::cerr << "User::readString - string length too large: " << n << "\n";
        return false;
    }

    if (n == 0) {
        s.clear();
        return true;
    }
    s.assign(static_cast<size_t>(n), '\0');
    ifs.read(&s[0], static_cast<std::streamsize>(n));
    return static_cast<bool>(ifs);
}