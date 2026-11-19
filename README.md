# File Compression System in C (Huffman Coding)

## What to install (Windows 11)
1. **VS Code**  -> https://code.visualstudio.com
2. **C/C++ extension** (by Microsoft) -> VS Code > Extensions (Ctrl+Shift+X) > search "C/C++" > Install
3. **MinGW-w64 (GCC compiler)** via MSYS2:
   - Download MSYS2 from https://www.msys2.org and install (default path `C:\msys64`)
   - Open **"MSYS2 UCRT64"** from Start menu and run:
     `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`   (press Enter / Y)
   - Add to Windows PATH: `C:\msys64\ucrt64\bin`
     (Start > "Edit environment variables" > Path > New > paste it > OK)
4. Restart VS Code, then check in its terminal: `gcc --version`

(Optional) **Code Runner** extension for a one-click Run button.

## Run it
1. VS Code > File > Open Folder > select `file-compressor`
2. Open terminal (Ctrl + `) and run:
   ```
   gcc main.c -o compressor.exe
   .\compressor.exe
   ```
3. Menu: `1` Compress -> input `sample.txt` -> output `sample.huf`
4. Menu: `2` Decompress -> input `sample.huf` -> output `restored.txt`

## How it works
1. Count how many times each byte appears
2. Build a Huffman tree (rare bytes go deeper)
3. Path in tree = bit code (left = 0, right = 1)
4. Replace every byte with its bit code, pack 8 bits per byte
5. Save the frequency table in the file header so decompression can rebuild the same tree

## Notes
- Best on text files (typical saving 30-60%)
- Very small files may get bigger (header = 1 KB)
- Already-compressed files (.zip, .jpg, .mp4) won't shrink
