#include <cstdio>
#include <cstring>
#include <cstdlib>

// Flag: flag{xor_is_classic}
// Key: 0x63 ('c')
unsigned char encoded_flag[] = {
    0x05, 0x0f, 0x02, 0x04, 0x18, // flag{
    0x1b, 0x0c, 0x11, 0x3c, 0x0a, // xor_i
    0x10, 0x3c, 0x00, 0x0f, 0x02, // s_cla
    0x10, 0x10, 0x0a, 0x00, 0x1e  // ssic}
};

bool check(const char* input) {
    int len = 20;
    
    // Length check
    if (strlen(input) != len) return false;

    unsigned char key = 0x63;

    // This loop will be flattened
    for (int i = 0; i < len; i++) {
        // XOR operation
        unsigned char encrypted = input[i] ^ key;
        
        // Branch inside loop
        if (encrypted != encoded_flag[i]) {
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <flag>\n", argv[0]);
        return 1;
    }

    if (check(argv[1])) {
        puts("Correct! You got the flag.");
    } else {
        puts("Wrong flag.");
    }
    return 0;
}