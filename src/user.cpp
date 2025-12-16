#include "user.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <algorithm>

User::User(const std::string& filepath) : path(filepath) {}
User::~User() {}

// write records -> binary file
bool User::write()
{
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "User::write - failed to open " << path << " for writing\n";
        return false;
    }

    // write persisted sign flag first
    ofs.write(reinterpret_cast<const char*>(&sign), sizeof(sign));

    uint64_t count = static_cast<uint64_t>(records.size());
    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& r : records) {
        if (!writeString(ofs, r.username)) return false;
        if (!writeString(ofs, r.password)) return false;
        ofs.write(reinterpret_cast<const char*>(&r.stage), sizeof(r.stage));
    }
    return ofs.good();
}

// read records <- binary file
bool User::read()
{
    records.clear();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        // no file -> nothing to read
        sign = 0;
        return false;
    }

    // read persisted sign flag first
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

// set current from first record or blank when no file
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

// login: clear sign (user is now active)
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

void User::logout()
{
    if (records.empty()) records.emplace_back();
    records.front().username = username;
    records.front().password = password;
    records.front().stage = stage;
    // mark explicit logout and persist so sign survives program exit
    sign = 1;
    write();
}

// signin ...
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

// accessors
std::string User::getUsername() const { return username; }
std::string User::getPassword() const { return password; }
char User::getStage() const { return stage; }
void User::setStage(char s) { stage = s; }

void User::setSign(bool s) { sign = s ? 1 : 0; }
bool User::getSign() const { return sign != 0; }

// helpers for string read/write (length + raw bytes)
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