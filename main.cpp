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
// This function takes a 4-digit PIN and creates
// a unique number from it. We use this to verify
// that the correct PIN was used during decryption.
unsigned int simpleHash(const string& pin) {
    // 5381 is a magic number that helps avoid hash collisions
    // (when different PINs create the same hash). This starting value
    // is from the DJB2 algorithm, which distributes hash values evenly.
    // Without a good starting point, different PINs might hash to the same number.
    unsigned int hash = 5381;
    
    // Go through each character in the PIN
    for (int i = 0; i < (int)pin.size(); i++) {
        char character = pin[i];
        
        // Step 1: Multiply hash by 33 using bit shift
        // (hash << 5) means shift left 5 bits, which multiplies by 32
        // Adding hash again gives us: (32 * hash) + hash = 33 * hash
        hash = hash * 33;
        
        // Step 2: Mix in the character using XOR
        // This combines the character into our hash
        hash = hash ^ (unsigned char)character;
    }
    
    return hash;  // Return the final hash number
}

// ============================================
// CREATE KEY - Convert PIN into encryption key
// ============================================
// This converts the 4-digit PIN string into
// a format we can use for encryption.
vector<unsigned char> makeKey(const string& pin) {
    // Create an empty vector to store the key bytes
    vector<unsigned char> key;
    // Go through each character in the PIN
    for (int i = 0; i < (int)pin.size(); i++) {
        // Convert the character to a byte and add it to the key
        unsigned char byte = (unsigned char)pin[i];
        key.push_back(byte);
    }
    return key;
}

// ============================================
// XOR PROCESS - Main encryption/decryption
// ============================================
// XOR is a simple but effective cipher.
// The same function works for both encrypting
// and decrypting (that's why it's called XOR!).
void xorProcess(vector<unsigned char>& data, const vector<unsigned char>& key) {
    // Go through each byte in the file
    for (int i = 0; i < (int)data.size(); i++) {
        // XOR it with the corresponding key byte
        // (repeat key if file is longer than key)
        data[i] ^= key[i % key.size()];
    }
}

// ============================================
// ENCRYPT FILE - Lock a file with your PIN
// ============================================
// This function:
// 1. Reads the file to encrypt
// 2. Adds a PIN verification code at the end
// 3. Encrypts everything with the PIN
// 4. Saves the encrypted file
bool encryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    // Step 1: Try to open the input file
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "ERROR: Could not open input file: " << inputPath << endl;
        return false;
    }

    // Step 2: Read all the file data into memory
    // istreambuf_iterator reads directly from the file stream
    // It converts all file bytes into unsigned char values and stores them in the vector
    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});
    in.close();
    cout << "Read " << data.size() << " bytes from file" << endl;

    // Step 3: Create a PIN verification code and add it to the end
    // (This allows us to verify the PIN is correct when decrypting)
    unsigned int pinHash = simpleHash(pin);
    
    // Convert the hash number into 4 bytes
    unsigned char hashBytes[4];
    hashBytes[0] = (pinHash >> 24) & 0xFF;  // First byte
    hashBytes[1] = (pinHash >> 16) & 0xFF;  // Second byte
    hashBytes[2] = (pinHash >> 8) & 0xFF;   // Third byte
    hashBytes[3] = pinHash & 0xFF;          // Fourth byte

    // Add these 4 bytes to the end of the file
    for (int i = 0; i < 4; i++) {
        data.push_back(hashBytes[i]);
    }

    // Step 4: Encrypt the data using the PIN
    auto key = makeKey(pin);
    xorProcess(data, key);

    // Step 5: Save the encrypted data to the output file
    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "ERROR: Could not create output file: " << outputPath << endl;
        return false;
    }

    // Get a pointer to the encrypted data
    char* buffer = (char*)data.data();
    // Write all the bytes to the file
    out.write(buffer, data.size());
    out.close();
    cout << "Wrote " << data.size() << " bytes to encrypted file" << endl;
    
    return true;
}

// ============================================
// DECRYPT FILE - Unlock a file with your PIN
// ============================================
// This function:
// 1. Reads the encrypted file
// 2. Decrypts it using the PIN
// 3. Verifies the PIN was correct
// 4. Saves the decrypted file
bool decryptFile(const string& inputPath, const string& outputPath, const string& pin) {
    // Step 1: Try to open the encrypted file
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "ERROR: Could not open encrypted file: " << inputPath << endl;
        return false;
    }

    // Step 2: Read all the encrypted data
    // istreambuf_iterator reads directly from the file stream
    // It converts all file bytes into unsigned char values and stores them in the vector
    vector<unsigned char> data((istreambuf_iterator<char>(in)), {});
    in.close();
    cout << "Read " << data.size() << " bytes from encrypted file" << endl;

    // Step 3: Check if file is large enough to contain a PIN hash
    if (data.size() < 4) {
        // File is too small - it's probably an old file
        // Just decrypt it without PIN verification
        cout << "Old format file detected (no PIN verification)" << endl;
        
        auto key = makeKey(pin);
        xorProcess(data, key);
        
        ofstream out(outputPath, ios::binary);
        if (!out) {
            cerr << "ERROR: Could not create output file" << endl;
            return false;
        }
        // Get a pointer to the decrypted data
        char* buffer = (char*)data.data();
        // Write all the bytes to the file
        out.write(buffer, data.size());
        out.close();
        return true;
    }

    // Step 4: Decrypt the data using the PIN
    auto key = makeKey(pin);
    xorProcess(data, key);

    // Step 5: Extract the PIN verification code from the last 4 bytes
    unsigned char hashBytes[4];
    // Read the last 4 bytes from the end of the file
    // (Remember: we added these bytes at the end during encryption)
    hashBytes[0] = data[data.size() - 4];  // First byte (most significant)
    hashBytes[1] = data[data.size() - 3];  // Second byte
    hashBytes[2] = data[data.size() - 2];  // Third byte
    hashBytes[3] = data[data.size() - 1];  // Fourth byte (least significant)

    // Convert the 4 bytes back into a single hash number
    // We use bit shifting to reconstruct the original hash value:
    // - Byte 0 shifts left 24 bits (multiplied by 16,777,216)
    // - Byte 1 shifts left 16 bits (multiplied by 65,536)
    // - Byte 2 shifts left 8 bits (multiplied by 256)
    // - Byte 3 stays in place (no shift)
    // Then we combine them all with OR to get the final hash number
    unsigned int storedHash = ((unsigned int)hashBytes[0] << 24) |
                               ((unsigned int)hashBytes[1] << 16) |
                               ((unsigned int)hashBytes[2] << 8) |
                               ((unsigned int)hashBytes[3]);

    // Step 6: Calculate what the hash should be with the provided PIN
    // We run the hash function again using the PIN that was provided
    unsigned int calculatedHash = simpleHash(pin);

    // Step 7: Verify that the PINs match
    if (storedHash != calculatedHash) {
        cerr << "ERROR: Wrong PIN! The file could not be decrypted." << endl;
        return false;  // Wrong PIN
    }

    // Step 8: Remove the PIN hash bytes from the end before saving
    data.resize(data.size() - 4);

    // Step 9: Save the decrypted data to the output file
    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "ERROR: Could not create output file" << endl;
        return false;
    }

    // Get a pointer to the decrypted data
    char* buffer = (char*)data.data();
    // Write all the bytes to the file
    out.write(buffer, data.size());
    out.close();
    cout << "Wrote " << data.size() << " bytes to decrypted file" << endl;
    
    return true;
}

// ============================================
// MAIN FUNCTION - Entry point
// ============================================
// This is where the program starts.
// It receives commands from the Electron app
// and calls the appropriate encryption function.
int main(int argc, char* argv[]) {
    // Check if the correct number of arguments were provided
    if (argc < 5) {
        cout << "Usage: vault.exe <encrypt/decrypt> <input> <output> <4-digit PIN>\n";
        cout << "Example: vault.exe encrypt myfile.txt myfile.locked 1234\n";
        return 1;  // Error: not enough arguments
    }

    // Extract the arguments from command line
    string cmd = argv[1];          // "encrypt" or "decrypt"
    string inputFile = argv[2];    // File to encrypt/decrypt
    string outputFile = argv[3];   // Where to save the result
    string pin = argv[4];          // The 4-digit PIN

    // Validate that the PIN is exactly 4 digits
    if (pin.size() != 4) {
        cout << "ERROR: PIN must be exactly 4 digits (you provided " << pin.size() << ").\n";
        return 1;  // Error: invalid PIN
    }

    // Check that each character is a digit
    for (int i = 0; i < (int)pin.size(); i++) {
        char digit = pin[i];
        if (digit < '0' || digit > '9') {
            cout << "ERROR: PIN must contain only digits (0-9).\n";
            return 1;
        }
    }

    // Perform the requested operation
    if (cmd == "encrypt") {
        cout << "Starting encryption...\n";
        if (encryptFile(inputFile, outputFile, pin)) {
            cout << "SUCCESS: File encrypted successfully!\n";
            return 0;  // Success
        } else {
            cout << "FAILED: Encryption did not complete.\n";
            return 1;  // Error
        }
    }
    else if (cmd == "decrypt") {
        cout << "Starting decryption...\n";
        if (decryptFile(inputFile, outputFile, pin)) {
            cout << "SUCCESS: File decrypted successfully!\n";
            return 0;  // Success
        } else {
            cout << "FAILED: Decryption did not complete. Check your PIN.\n";
            return 1;  // Error
        }
    }
    else {
        cout << "ERROR: Unknown command: " << cmd << "\n";
        cout << "Use 'encrypt' or 'decrypt'.\n";
        return 1;  // Error: invalid command
    }
}
