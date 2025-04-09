#include "base64.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Base64 encoding table
static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Decoding table - initialized in build_decoding_table()
static unsigned char *base64_decode_table = NULL;

void build_decoding_table() {
    if (base64_decode_table != NULL) {
        return; // Table already built
    }
    
    base64_decode_table = malloc(256);
    if (base64_decode_table == NULL) {
        perror("Échec d'allocation mémoire");
        exit(1);
    }
    
    // Initialize to invalid value
    memset(base64_decode_table, 0xFF, 256);
    
    // Set valid values
    for (int i = 0; i < 64; i++) {
        base64_decode_table[(unsigned char)base64_chars[i]] = i;
    }
}

void base64_cleanup() {
    free(base64_decode_table);
    base64_decode_table = NULL;
}

char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) {
    if (data == NULL || output_length == NULL) {
        return NULL;
    }

    *output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(*output_length + 1); // +1 for null terminator
    
    if (encoded_data == NULL) {
        return NULL;
    }

    for (size_t i = 0, j = 0; i < input_length;) {
        // Get three bytes (or what's available)
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        // Combine into 24 bits
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        // Split into four 6-bit values and map to base64 characters
        encoded_data[j++] = base64_chars[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 12) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 6) & 0x3F];
        encoded_data[j++] = base64_chars[triple & 0x3F];
    }

    // Add padding if necessary
    if (input_length % 3 == 1) {
        encoded_data[*output_length - 2] = '=';
        encoded_data[*output_length - 1] = '=';
    } else if (input_length % 3 == 2) {
        encoded_data[*output_length - 1] = '=';
    }

    encoded_data[*output_length] = '\0';
    return encoded_data;
}

unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length) {
    printf("Décodage base64: %s (longueur: %zu)\n", data, input_length);

    if (data == NULL || output_length == NULL) {
        printf("Erreur: Paramètres d'entrée NULL\n");
        return NULL;
    }

    if (input_length == 0 || input_length % 4 != 0) {
        printf("Erreur: Longueur base64 invalide %zu (doit être un multiple de 4)\n", input_length);
        return NULL;
    }

    if (base64_decode_table == NULL) {
        build_decoding_table();
    }

    // Calculate output size (remove padding)
    size_t padding = 0;
    if (input_length > 0) {
        if (data[input_length - 1] == '=') padding++;
        if (input_length > 1 && data[input_length - 2] == '=') padding++;
    }
    
    *output_length = (input_length / 4) * 3 - padding;
    
    // Allocate memory for decoded data
    unsigned char *decoded_data = malloc(*output_length + 1); // +1 for null terminator
    if (decoded_data == NULL) {
        printf("Erreur: Échec d'allocation mémoire\n");
        return NULL;
    }

    // Process data in 4-byte chunks
    size_t i = 0, j = 0;
    while (i < input_length) {
        // Get four characters
        unsigned char c1 = data[i++];
        unsigned char c2 = data[i++];
        unsigned char c3 = data[i++];
        unsigned char c4 = data[i++];
        
        // Validate each character
        if ((c1 == '=') || (base64_decode_table[c1] == 0xFF) ||
            (c2 == '=') || (base64_decode_table[c2] == 0xFF) ||
            ((c3 != '=') && (base64_decode_table[c3] == 0xFF)) ||
            ((c4 != '=') && (base64_decode_table[c4] == 0xFF))) {
            printf("Erreur: Caractère base64 invalide trouvé\n");
            free(decoded_data);
            return NULL;
        }
        
        // Decode the four characters into three bytes
        uint32_t sextet_a = base64_decode_table[c1];
        uint32_t sextet_b = base64_decode_table[c2];
        uint32_t sextet_c = (c3 == '=') ? 0 : base64_decode_table[c3];
        uint32_t sextet_d = (c4 == '=') ? 0 : base64_decode_table[c4];
        
        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;
        
        // Store bytes in output buffer (respecting padding)
        if (j < *output_length) decoded_data[j++] = (triple >> 16) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = triple & 0xFF;
        
        // Stop if we've reached padded section
        if (c3 == '=' || c4 == '=') break;
    }
    
    // Add null terminator
    decoded_data[*output_length] = '\0';
    
    printf("Données décodées: %s\n", decoded_data);
    printf("Longueur de sortie: %zu\n", *output_length);
    
    return decoded_data;
}

// Simple wrapper functions
char *encode(const char *text) {
    if (text == NULL) {
        return NULL;
    }
    
    size_t output_length;
    char *encoded = base64_encode((const unsigned char*)text, strlen(text), &output_length);
    return encoded;
}

char *decode(const char *base64text) {
    if (base64text == NULL) {
        printf("Erreur: Entrée NULL à décoder\n");
        return NULL;
    }
    
    // Create a copy of the input string that we can modify
    size_t len = strlen(base64text);
    char *trimmed = malloc(len + 1);
    if (trimmed == NULL) {
        printf("Erreur: Échec d'allocation mémoire\n");
        return NULL;
    }
    strcpy(trimmed, base64text);
    
    // Remove trailing whitespace
    size_t trimmed_len = len;
    while (trimmed_len > 0 && (trimmed[trimmed_len-1] == ' ' || trimmed[trimmed_len-1] == '\n' || 
                        trimmed[trimmed_len-1] == '\r' || trimmed[trimmed_len-1] == '\t')) {
        trimmed[--trimmed_len] = '\0';
    }
    
    // Determine start position (skip leading whitespace)
    char *start = trimmed;
    while (*start && (*start == ' ' || *start == '\n' || 
                     *start == '\r' || *start == '\t')) {
        start++;
    }
    
    // Check if we have anything left after trimming
    if (*start == '\0') {
        printf("Erreur: Chaîne base64 vide après suppression des espaces\n");
        free(trimmed);
        return NULL;
    }
    
    // Decode the trimmed string
    size_t output_length;
    unsigned char *decoded = base64_decode(start, strlen(start), &output_length);
    
    // Free the trimmed copy, regardless of decode result
    free(trimmed);
    
    if (decoded == NULL) {
        printf("Erreur: Échec du décodage de la chaîne base64\n");
        return NULL;
    }
    
    return (char*)decoded;
}