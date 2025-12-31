/*PF - FINAL PROJECT
Group 10. Members: Waqas Ameer, Meerub Yaseen, M. Saaib
_________________________________________________________
Encrypto is a lightweight file-vault tool written in C++. 
It lets you lock and unlock any file using a user-defined PIN.
The program loads your file into memory, encrypts it using XOR processing,
and saves it as a protected .enc file.
Decrypting uses the same PIN to restore the original content.*/

#include <iostream>      // For printing messages to console (cout, cerr)
#include <fstream>       // For reading and writing files (ifstream, ofstream)
#include <string>        // For storing text and file paths as strings
#include <vector>        // For storing file data as a list of bytes
#include <cstring>       // For character and string manipulation functions
using namespace std;

// HASH FUNCTION - Converts PIN to a number
unsigned int simpleHash(const string& pin) {
    unsigned int hash = 5381;  // Magic number from DJB2 algorithm to avoid hash collisions
    
    for (int i = 0; i < (int)pin.size(); i++) {  // Go through each character in the PIN
        char character = pin[i];
        hash = hash * 33;  // Multiply hash by 33 using bit shift (32*hash + hash)
        hash = hash ^ (unsigned char)character;  // Mix in the character using XOR
    }
    
    return hash;  // Return the final hash number
}

// ============================================
// CREATE KEY - Convert PIN into encryption key
// ============================================
vector<unsigned char> makeKey(const string& pin) {
    vector<unsigned char> key;  // Create empty vector to store key bytes
    for (int i = 0; i < (int)pin.size(); i++) {  // Go through each character in PIN
        unsigned char byte = (unsigned char)pin[i];  // Convert character to byte
        key.push_back(byte);  // Add it to the key
    }
    return key;
}

// ============================================
// XOR PROCESS - Main encryption/decryption
// ============================================
void xorProcess(vector<unsigned char>& data, const vector<unsigned char>& key) {
    for (int i = 0; i < (int)data.size(); i++) {  // Go through each byte in file
        data[i] ^= key[i % key.size()];  // XOR with corresponding key byte (repeat key if needed)
    }
}

// ============================================
// ENCRYPT FILE - Lock a file with your PIN
// ============================================
bool encryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    ifstream in(inputPath, ios::binary);  // Try to open input file
    if (!in) {
        cerr << "ERROR: Could not open input file: " << inputPath << endl;
        return false;
    }

    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});  // Read all file data into memory
    in.close();
    cout << "Read " << data.size() << " bytes from file" << endl;

    unsigned int pinHash = simpleHash(pin);  // Create PIN verification code
    
    unsigned char hashBytes[4];  // Convert hash number into 4 bytes
    hashBytes[0] = (pinHash >> 24) & 0xFF;  // First byte
    hashBytes[1] = (pinHash >> 16) & 0xFF;  // Second byte
    hashBytes[2] = (pinHash >> 8) & 0xFF;   // Third byte
    hashBytes[3] = pinHash & 0xFF;          // Fourth byte

    for (int i = 0; i < 4; i++) {  // Add these 4 bytes to end of file
        data.push_back(hashBytes[i]);
    }

    auto key = makeKey(pin);  // Encrypt the data using PIN
    xorProcess(data, key);

    ofstream out(outputPath, ios::binary);  // Save encrypted data to output file
    if (!out) {
        cerr << "ERROR: Could not create output file: " << outputPath << endl;
        return false;
    }

    char* buffer = (char*)data.data();  // Get pointer to encrypted data
    out.write(buffer, data.size());  // Write all bytes to file
    out.close();
    cout << "Wrote " << data.size() << " bytes to encrypted file" << endl;
    
    return true;
}

// ============================================
// DECRYPT FILE - Unlock a file with your PIN
// ============================================
bool decryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    ifstream in(inputPath, ios::binary);  // Try to open encrypted file
    if (!in) {
        cerr << "ERROR: Could not open encrypted file: " << inputPath << endl;
        return false;
    }

    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});  // Read all encrypted data
    in.close();
    cout << "Read " << data.size() << " bytes from encrypted file" << endl;

    if (data.size() < 4) {  // Check if file is large enough to contain PIN hash
        cout << "Old format file detected (no PIN verification)" << endl;
        
        auto key = makeKey(pin);  // Just decrypt without PIN verification
        xorProcess(data, key);
        
        ofstream out(outputPath, ios::binary);
        if (!out) {
            cerr << "ERROR: Could not create output file" << endl;
            return false;
        }
        char* buffer = (char*)data.data();  // Get pointer to decrypted data
        out.write(buffer, data.size());  // Write all bytes to file
        out.close();
        return true;
    }

    auto key = makeKey(pin);  // Decrypt the data using PIN
    xorProcess(data, key);

    unsigned char hashBytes[4];  // Extract PIN verification code from last 4 bytes
    hashBytes[0] = data[data.size() - 4];  // First byte (most significant)
    hashBytes[1] = data[data.size() - 3];  // Second byte
    hashBytes[2] = data[data.size() - 2];  // Third byte
    hashBytes[3] = data[data.size() - 1];  // Fourth byte (least significant)

    unsigned int storedHash = ((unsigned int)hashBytes[0] << 24) |  // Convert 4 bytes back to hash number
                               ((unsigned int)hashBytes[1] << 16) |  // using bit shifting
                               ((unsigned int)hashBytes[2] << 8) |
                               ((unsigned int)hashBytes[3]);

    unsigned int calculatedHash = simpleHash(pin);  // Calculate what hash should be with provided PIN

    if (storedHash != calculatedHash) {  // Verify PINs match
        cerr << "ERROR: Wrong PIN! The file could not be decrypted." << endl;
        return false;
    }

    data.resize(data.size() - 4);  // Remove PIN hash bytes from end before saving

    ofstream out(outputPath, ios::binary);  // Save decrypted data to output file
    if (!out) {
        cerr << "ERROR: Could not create output file" << endl;
        return false;
    }

    char* buffer = (char*)data.data();  // Get pointer to decrypted data
    out.write(buffer, data.size());  // Write all bytes to file
    out.close();
    cout << "Wrote " << data.size() << " bytes to decrypted file" << endl;
    
    return true;
}

// ============================================
// MAIN FUNCTION - Entry point
// ============================================
int main(int argc, char* argv[]) {
    if (argc < 5) {  // Check if correct number of arguments provided
        cout << "Usage: vault.exe <encrypt/decrypt> <input> <output> <4-digit PIN>\n";
        cout << "Example: vault.exe encrypt myfile.txt myfile.locked 1234\n";
        return 1;
    }

    string cmd = argv[1];          // "encrypt" or "decrypt"
    string inputFile = argv[2];    // File to encrypt/decrypt
    string outputFile = argv[3];   // Where to save result
    string pin = argv[4];          // The 4-digit PIN

    if (pin.size() != 4) {  // Validate PIN is exactly 4 digits
        cout << "ERROR: PIN must be exactly 4 digits (you provided " << pin.size() << ").\n";
        return 1;
    }

    for (int i = 0; i < (int)pin.size(); i++) {  // Check each character is a digit
        char digit = pin[i];
        if (digit < '0' || digit > '9') {
            cout << "ERROR: PIN must contain only digits (0-9).\n";
            return 1;
        }
    }

    if (cmd == "encrypt") {  // Perform requested operation
        cout << "Starting encryption...\n";
        if (encryptFile(inputFile, outputFile, pin)) {
            cout << "SUCCESS: File encrypted successfully!\n";
            return 0;
        } else {
            cout << "FAILED: Encryption did not complete.\n";
            return 1;
        }
    }
    else if (cmd == "decrypt") {
        cout << "Starting decryption...\n";
        if (decryptFile(inputFile, outputFile, pin)) {
            cout << "SUCCESS: File decrypted successfully!\n";
            return 0;
        } else {
            cout << "FAILED: Decryption did not complete. Check your PIN.\n";
            return 1;
        }
    }
    else {
        cout << "ERROR: Unknown command: " << cmd << "\n";
        cout << "Use 'encrypt' or 'decrypt'.\n";
        return 1;
    }
}