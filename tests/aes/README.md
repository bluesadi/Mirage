AES-128 ECB test case for obfuscation passes.

- Source: Minimal self-contained AES-128 implementation (ECB encrypt) for testing IR transformations.
- Usage:
  - Build with repo `build/bin/clang` at O0 and enable substitution:
    
    ```fish
    set -l CLANG ./build/bin/clang
    $CLANG -O0 -mllvm -enable-sub-obfu tests/aes/aes.c -o tests/aes/aes
    ./tests/aes/aes
    ```

This is intentionally compact and public-domain style code authored for this repository.
