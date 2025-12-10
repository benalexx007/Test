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
        // no file is not an error for read() — caller's Init will handle
        return false;
    }

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
        username = std::string(1, '\0');
        password = std::string(1, '\0');
        stage = -1;
        // ensure records vector has at least one slot to store future logout
        if (records.empty()) records.emplace_back();
        return;
    }
    username = records.front().username;
    password = records.front().password;
    stage = records.front().stage;
}

bool User::login(const std::string& user, const std::string& pass)
{
    // ensure we have current list
    read();

    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].username == user && records[i].password == pass) {
            // assign current and move record to front
            username = records[i].username;
            password = records[i].password;
            stage = records[i].stage;
            if (i != 0) std::swap(records[i], records[0]);
            // persist order
            write();
            return true;
        }
    }
    std::cout << "USERNAME OR PASSWORD IS INCORRECT\n";
    return false;
}

void User::logout()
{
    // ensure vector exists, put current user into first slot, persist
    if (records.empty()) records.emplace_back();
    records.front().username = username;
    records.front().password = password;
    records.front().stage = stage;
    write();
}

bool User::signin(const std::string& user, const std::string& pass)
{
    // read existing list
    read();

    // create new record
    Record r;
    r.username = user;
    r.password = pass;
    r.stage = -1;

    // add at end and swap with first, make it current
    records.push_back(r);
    if (records.size() > 1) std::swap(records.back(), records.front());
    username = records.front().username;
    password = records.front().password;
    stage = records.front().stage;
    return write();
}

// accessors
std::string User::getUsername() const { return username; }
std::string User::getPassword() const { return password; }
int User::getStage() const { return stage; }
void User::setStage(int s) { stage = s; }

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
    if (n == 0) {
        s.clear();
        return true;
    }
    s.assign(n, '\0');
    ifs.read(&s[0], static_cast<std::streamsize>(n));
    return static_cast<bool>(ifs);
}