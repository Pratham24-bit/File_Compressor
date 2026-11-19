/*
 * File Compression System using Huffman Coding (C language)
 * ---------------------------------------------------------
 * Idea: common bytes get SHORT bit codes, rare bytes get LONG bit codes.
 * Works on any file (txt, csv, c, etc.).
 *
 * Compressed file format:
 *   [4 bytes "HUF1"] [4 bytes original size] [256 x 4 bytes frequency table] [compressed bits]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------- Tree node ---------- */
typedef struct Node {
    unsigned char ch;        /* the byte (only used by leaf nodes) */
    uint32_t freq;           /* how many times it appears */
    struct Node *left, *right;
} Node;

Node *newNode(unsigned char ch, uint32_t freq, Node *left, Node *right) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->ch = ch;
    n->freq = freq;
    n->left = left;
    n->right = right;
    return n;
}

void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

/* ---------- Step 1: build the Huffman tree ---------- */
Node *buildTree(uint32_t freq[256]) {
    Node *list[256];
    int n = 0, i;

    for (i = 0; i < 256; i++)
        if (freq[i] > 0)
            list[n++] = newNode((unsigned char)i, freq[i], NULL, NULL);

    if (n == 0) return NULL;                         /* empty file */
    if (n == 1) return newNode(0, list[0]->freq, list[0], NULL); /* only 1 kind of byte */

    while (n > 1) {
        /* find the two nodes with the smallest frequency */
        int a = 0, b = 1;
        if (list[b]->freq < list[a]->freq) { a = 1; b = 0; }
        for (i = 2; i < n; i++) {
            if (list[i]->freq < list[a]->freq)      { b = a; a = i; }
            else if (list[i]->freq < list[b]->freq) { b = i; }
        }
        /* join them under a new parent */
        Node *parent = newNode(0, list[a]->freq + list[b]->freq, list[a], list[b]);
        list[a] = parent;
        list[b] = list[n - 1];
        n--;
    }
    return list[0];
}

/* ---------- Step 2: give every byte a code like "010" ---------- */
void makeCodes(Node *root, char *path, int depth, char codes[256][256]) {
    if (root == NULL) return;
    if (root->left == NULL && root->right == NULL) {   /* leaf */
        path[depth] = '\0';
        strcpy(codes[root->ch], path);
        return;
    }
    path[depth] = '0';
    makeCodes(root->left, path, depth + 1, codes);
    path[depth] = '1';
    makeCodes(root->right, path, depth + 1, codes);
}

/* ---------- COMPRESS ---------- */
int compressFile(const char *inName, const char *outName) {
    FILE *in = fopen(inName, "rb");
    if (!in) { printf("Error: cannot open '%s'\n", inName); return 1; }

    /* count how often each byte appears */
    uint32_t freq[256] = {0}, size = 0;
    int c;
    while ((c = fgetc(in)) != EOF) { freq[c]++; size++; }

    Node *root = buildTree(freq);
    static char codes[256][256];
    char path[256];
    memset(codes, 0, sizeof(codes));
    makeCodes(root, path, 0, codes);

    FILE *out = fopen(outName, "wb");
    if (!out) { printf("Error: cannot create '%s'\n", outName); fclose(in); return 1; }

    /* header */
    fwrite("HUF1", 1, 4, out);
    fwrite(&size, sizeof(uint32_t), 1, out);
    fwrite(freq, sizeof(uint32_t), 256, out);

    /* write the bits, packing 8 bits into one byte */
    rewind(in);
    unsigned char buffer = 0;
    int bitCount = 0;
    while ((c = fgetc(in)) != EOF) {
        char *code = codes[c];
        for (int i = 0; code[i] != '\0'; i++) {
            buffer = (buffer << 1) | (code[i] - '0');
            bitCount++;
            if (bitCount == 8) {
                fputc(buffer, out);
                buffer = 0;
                bitCount = 0;
            }
        }
    }
    if (bitCount > 0) {                       /* last few bits: pad with zeros */
        buffer <<= (8 - bitCount);
        fputc(buffer, out);
    }

    long compressedSize = ftell(out);
    fclose(in);
    fclose(out);
    freeTree(root);

    printf("\nCompressed successfully!\n");
    printf("Original size   : %u bytes\n", size);
    printf("Compressed size : %ld bytes\n", compressedSize);
    if (size > 0)
        printf("Space saved     : %.2f%%\n", 100.0 * (1.0 - (double)compressedSize / size));
    return 0;
}

/* ---------- DECOMPRESS ---------- */
int decompressFile(const char *inName, const char *outName) {
    FILE *in = fopen(inName, "rb");
    if (!in) { printf("Error: cannot open '%s'\n", inName); return 1; }

    char magic[4];
    uint32_t size, freq[256];
    if (fread(magic, 1, 4, in) != 4 || memcmp(magic, "HUF1", 4) != 0) {
        printf("Error: not a valid compressed file.\n");
        fclose(in);
        return 1;
    }
    fread(&size, sizeof(uint32_t), 1, in);
    fread(freq, sizeof(uint32_t), 256, in);

    Node *root = buildTree(freq);               /* rebuild the SAME tree */
    FILE *out = fopen(outName, "wb");
    if (!out) { printf("Error: cannot create '%s'\n", outName); fclose(in); return 1; }

    Node *cur = root;
    uint32_t written = 0;
    int c;
    while (written < size && (c = fgetc(in)) != EOF) {
        for (int bit = 7; bit >= 0 && written < size; bit--) {
            int b = (c >> bit) & 1;
            cur = (b == 0) ? cur->left : cur->right;   /* 0 = left, 1 = right */
            if (cur->left == NULL && cur->right == NULL) {   /* reached a leaf */
                fputc(cur->ch, out);
                written++;
                cur = root;
            }
        }
    }

    fclose(in);
    fclose(out);
    freeTree(root);
    printf("\nDecompressed successfully! (%u bytes)\n", written);
    return 0;
}

/* ---------- MENU ---------- */
int main(void) {
    int choice;
    char inName[256], outName[256];

    while (1) {
        printf("\n===== FILE COMPRESSION SYSTEM (Huffman) =====\n");
        printf("1. Compress a file\n");
        printf("2. Decompress a file\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) return 0;

        if (choice == 3) { printf("Bye!\n"); break; }
        if (choice != 1 && choice != 2) { printf("Invalid choice.\n"); continue; }

        printf("Input file name  : ");
        scanf(" %255[^\n]", inName);
        printf("Output file name : ");
        scanf(" %255[^\n]", outName);

        if (choice == 1) compressFile(inName, outName);
        else             decompressFile(inName, outName);
    }
    return 0;
}