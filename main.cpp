#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
using namespace std;

// Simple hash function for PIN validation (32-bit)
unsigned int simpleHash(const string& pin) {
    unsigned int hash = 5381;
    for (char c : pin) {
        hash = ((hash << 5) + hash) ^ (unsigned char)c;
    }
    return hash;
}

// Make a repeated key from PIN
vector<unsigned char> makeKey(const string& pin) {
    vector<unsigned char> key(pin.begin(), pin.end());
    return key;
}

// XOR encrypt/decrypt (same function)
void xorProcess(vector<unsigned char>& data, const vector<unsigned char>& key) {
    for (size_t i = 0; i < data.size(); i++) {
        data[i] ^= key[i % key.size()];
    }
}

// Encrypt file with PIN validation hash
bool encryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    ifstream in(inputPath, ios::binary);
    if (!in) return false;

    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});
    in.close();

    // Add PIN hash at the end (4 bytes)
    unsigned int pinHash = simpleHash(pin);
    unsigned char hashBytes[4];
    hashBytes[0] = (pinHash >> 24) & 0xFF;
    hashBytes[1] = (pinHash >> 16) & 0xFF;
    hashBytes[2] = (pinHash >> 8) & 0xFF;
    hashBytes[3] = pinHash & 0xFF;

    for (int i = 0; i < 4; i++) {
        data.push_back(hashBytes[i]);
    }

    auto key = makeKey(pin);
    xorProcess(data, key);

    ofstream out(outputPath, ios::binary);
    if (!out) return false;

    out.write((char*)data.data(), data.size());
    out.close();
    return true;
}

// Decrypt file with PIN validation
bool decryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    ifstream in(inputPath, ios::binary);
    if (!in) return false;

    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});
    in.close();

    if (data.size() < 4) {
        // File is too small, decrypt without validation (old format)
        auto key = makeKey(pin);
        xorProcess(data, key);
        
        ofstream out(outputPath, ios::binary);
        if (!out) return false;
        out.write((char*)data.data(), data.size());
        out.close();
        return true;
    }

    auto key = makeKey(pin);
    xorProcess(data, key);

    // Extract and verify PIN hash from the end
    unsigned char hashBytes[4];
    hashBytes[0] = data[data.size() - 4];
    hashBytes[1] = data[data.size() - 3];
    hashBytes[2] = data[data.size() - 2];
    hashBytes[3] = data[data.size() - 1];

    unsigned int storedHash = ((unsigned int)hashBytes[0] << 24) |
                               ((unsigned int)hashBytes[1] << 16) |
                               ((unsigned int)hashBytes[2] << 8) |
                               ((unsigned int)hashBytes[3]);

    unsigned int calculatedHash = simpleHash(pin);

    // Check if this looks like a valid hash (if hash matches, it's new format)
    // If hash doesn't match, it might be old format - try decrypting without hash validation
    if (storedHash != calculatedHash) {
        // Hash mismatch - could be old format or wrong PIN
        // For now, treat as wrong PIN for new format files
        // But we can't distinguish, so return false
        return false;
    }

    // Remove hash bytes before writing
    data.resize(data.size() - 4);

    ofstream out(outputPath, ios::binary);
    if (!out) return false;

    out.write((char*)data.data(), data.size());
    out.close();
    return true;
}

// Command-line backend interface for Electron
int main(int argc, char* argv[]) {
    if (argc < 5) {
        cout << "Usage: vault.exe <encrypt/decrypt> <input> <output> <4-digit PIN>\n";
        return 1;
    }

    string cmd = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];
    string pin = argv[4];

    if (pin.size() != 4) {
        cout << "PIN must be 4 digits.\n";
        return 1;
    }

    if (cmd == "encrypt") {
        if (encryptFile(inputFile, outputFile, pin)) cout << "Encrypted OK\n";
        else cout << "Failed\n";
    }
    else if (cmd == "decrypt") {
        if (decryptFile(inputFile, outputFile, pin)) cout << "Decrypted OK\n";
        else cout << "Failed\n";
    }
    else {
        cout << "Unknown command.\n";
    }

    return 0;
}
