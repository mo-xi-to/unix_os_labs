#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <iomanip>
#include <sstream>

using namespace std;
namespace fs = filesystem;

struct SHA1 {
    uint32_t digest[5];
    unsigned char buffer[64];
    uint64_t size;
    uint32_t buffer_idx;

    SHA1() { reset(); }

    void reset() {
        digest[0] = 0x67452301; digest[1] = 0xEFCDAB89;
        digest[2] = 0x98BADCFE; digest[3] = 0x10325476;
        digest[4] = 0xC3D2E1F0;
        size = 0; buffer_idx = 0;
    }

    uint32_t rol(uint32_t value, size_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    void process_block(const unsigned char* block) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | (block[i*4+3]);
        for (int i = 16; i < 80; i++)
            w[i] = rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

        uint32_t a = digest[0], b = digest[1], c = digest[2], d = digest[3], e = digest[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            
            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }
        digest[0] += a; digest[1] += b; digest[2] += c; digest[3] += d; digest[4] += e;
        buffer_idx = 0;
    }

    void add_byte(unsigned char byte) {
        buffer[buffer_idx++] = byte;
        size++;
        if (buffer_idx == 64) process_block(buffer);
    }

    void update(istream& is) {
        char chunk[4096];
        while (is.read(chunk, sizeof(chunk)) || is.gcount()) {
            size_t count = is.gcount();
            for (size_t i = 0; i < count; ++i) add_byte(chunk[i]);
        }
    }

    string finalize() {
        unsigned char temp[64];
        copy(buffer, buffer + buffer_idx, temp);
        size_t idx = buffer_idx;
        
        temp[idx++] = 0x80;
        if (idx > 56) {
            fill(temp + idx, temp + 64, 0);
            process_block(temp);
            idx = 0;
        }
        fill(temp + idx, temp + 56, 0);
        uint64_t bits = size * 8;
        for (int i = 0; i < 8; i++) temp[63 - i] = (bits >> (i * 8)) & 0xFF;
        process_block(temp);

        stringstream ss;
        ss << hex << setfill('0');
        for (int i = 0; i < 5; ++i) ss << setw(8) << digest[i];
        return ss.str();
    }
};

string calculate_file_hash(const fs::path& p) {
    ifstream f(p, ios::binary);
    if (!f.is_open()) return "";
    SHA1 hasher;
    hasher.update(f);
    return hasher.finalize();
}

int main(int argc, char* argv[]) {
    string target_dir_str;

    if (argc > 1) {
        target_dir_str = argv[1];
    } else {
        cout << "Enter target directory: ";
        getline(cin, target_dir_str);
    }

    fs::path target_path(target_dir_str);

    if (!fs::exists(target_path) || !fs::is_directory(target_path)) {
        cerr << "Error: Invalid path" << endl;
        return 1;
    }

    cout << "Scanning directory structure..." << endl;
    
    map<uintmax_t, list<fs::path>> files_by_size;

    for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
        if (entry.is_regular_file() && !entry.is_symlink()) {
            try {
                files_by_size[entry.file_size()].push_back(entry.path());
            } catch (...) {
                continue; 
            }
        }
    }

    int dup_count = 0;

    cout << "Analyzing files content..." << endl;

    for (auto& [size, files] : files_by_size) {
        if (files.size() < 2) continue;

        map<string, fs::path> unique_hashes;

        for (const auto& current_file : files) {
            string hash = calculate_file_hash(current_file);
            if (hash.empty()) continue;

            if (unique_hashes.count(hash)) {
                fs::path original = unique_hashes[hash];

                if (fs::equivalent(current_file, original)) continue;

                cout << "Duplicate found: " << current_file.filename() << endl;
                
                try {
                    fs::remove(current_file);
                    fs::create_hard_link(original, current_file);
                    cout << "Linked " << original.filename() << endl;
                    dup_count++;
                } catch (const exception& e) {
                    cerr << "Error: " << e.what() << endl;
                }
            } else {
                unique_hashes[hash] = current_file;
            }
        }
    }

    cout << "Operation completed. Files replaced: " << dup_count << endl;

    return 0;
}