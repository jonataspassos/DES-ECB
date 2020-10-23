#include <stdio.h>
#include <stdlib.h>

#include "bitwiseop.h"

typedef enum
{
    encrypt = 1,
    decrypt = 0
} Mode;

char *initial_permutation,
    *final_permutation,
    *expansion,
    *permutation_32,
    *compress_56,
    *shift_left,
    *compress_48,
    **sbox_table;

Mode mode;

long long int permutation(long long int value, char *positions, int size)
{
    long long int ret = 0;
    char i;
    for (i = 0; i < size; i++)
    {
        bit_copy(ret, value, positions[i]);
    }
    return ret;
}

long long int swapLR(long long int value, int axis)
{
    return (value >> axis) & bit_range(0, axis)           // L to R
           | (value << axis) & bit_range(axis, 2 * axis); // R to L
}

long long int shiftL_circular_split(long long int value, int axis, int shift)
{
    int temp = (value >> axis - shift) & (bit_range(0, shift) | bit_range(axis, axis + shift));
    //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    return (value << shift) // Faz o deslocamento normal (com os zeros no inicio)
               & (bit_range(shift, axis) | bit_range(axis + shift, axis * 2))
           // zera todos os valores que entraram nas posições dos estourados
           | temp; // inclui os estourados em suas específicas posições
}

long long int shiftR_circular_split(long long int value, int axis, int shift)
{
    int temp = (value << axis - shift) & (bit_range(axis - shift, axis) | bit_range(2 * axis - shift, 2 * axis));
    //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    return (value >> shift) // Faz o deslocamento normal (com os zeros no final)
               & (bit_range(0, axis - shift) | bit_range(axis, 2 * axis - shift))
           // zera todos os valores que entraram nas posições dos estourados
           | temp; // inclui os estourados em suas específicas posições
}

long long int s_box(long long int value, char **table)
{
    char i;
    long long int ret = 0;
    for (i = 0; i < 8; i++)
    {
        ret = ret | (((long long int)(table[i][(value >> (i * 6)) & bit_range(0, 6)])) << i * 4);
    }
    return ret;
}

void sort_sbox_table()
{
    char **table2 = (char **)malloc(sizeof(char *) * 8);
    char i, j, temp;

    for (i = 0; i < 8; i++)
    {
        table2[i] = (char *)malloc(sizeof(char) * 64);
    }

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 16; j++)
        {
            table2[i][2 * j] = sbox_table[i][j];
            table2[i][2 * j + 1] = sbox_table[i][j + 16];
        }

        for (j = 0; j < 16; j++)
        {
            table2[i][32 + 2 * j] = sbox_table[i][32+j];
            table2[i][32 + 2 * j + 1] = sbox_table[i][32 + j + 16];
        }
    }

    free(sbox_table);
    sbox_table = table2;
}

long long int iteration(long long int value, long long int key_48)
{
    long long int ret = 0;

    //F (R,Ki)
    long long int f = permutation(value, expansion, 48);
    f = f ^ key_48;
    f = s_box(f, sbox_table);
    f = permutation(f, permutation_32, 32);

    //swap
    ret = swapLR(value, 32);

    //xor
    return ret ^ f;
}

long long int key_i(long long int value, char i)
{
    switch (mode)
    {
    case encrypt:
        return shiftL_circular_split(value, 28, shift_left[i]);
    case decrypt:
        return shiftR_circular_split(value, 28, shift_left[i]);
    }
}

long long int key_prepare(long long int value)
{
    return permutation(value, compress_56, 56);
}

long long block_encrypt(long long int value, long long int key)
{
    char i;
    value = permutation(value, initial_permutation, 64);

    for (i = 0; i < 16; i++)
    {
        key = key_i(key, mode ? i : (15 - i));
        value = iteration(value, permutation(key, compress_48, 48));
    }

    return permutation(swapLR(value, 32), final_permutation, 64);
}

void import_values()
{
    char j;
    FILE *tables;
    char i = 0;
    //alloc spaces
    initial_permutation = (char *)malloc(sizeof(char) * 64);
    if (!initial_permutation)
    {
        printf("ERROR! Over memory to 'initial_permutation'");
        exit(1);
    }
    final_permutation = (char *)malloc(sizeof(char) * 64);
    if (!final_permutation)
    {
        printf("ERROR! Over memory to 'final_permutation'");
        exit(1);
    }
    expansion = (char *)malloc(sizeof(char) * 48);
    if (!expansion)
    {
        printf("ERROR! Over memory to 'expansion'");
        exit(1);
    }
    permutation_32 = (char *)malloc(sizeof(char) * 32);
    if (!permutation_32)
    {
        printf("ERROR! Over memory to 'permutation_32'");
        exit(1);
    }
    compress_56 = (char *)malloc(sizeof(char) * 56);
    if (!compress_56)
    {
        printf("ERROR! Over memory to 'compress_56'");
        exit(1);
    }
    shift_left = (char *)malloc(sizeof(char) * 16);
    if (!shift_left)
    {
        printf("ERROR! Over memory to 'shift_left'");
        exit(1);
    }
    compress_48 = (char *)malloc(sizeof(char) * 48);
    if (!compress_48)
    {
        printf("ERROR! Over memory to 'compress_48'");
        exit(1);
    }
    sbox_table = (char **)malloc(sizeof(char *) * 8);
    if (!sbox_table)
    {
        printf("ERROR! Over memory to 'sblock_table'");
        exit(1);
    }
    for (j = 0; j < 8; j++)
    {
        sbox_table[j] = (char *)malloc(sizeof(char) * 64);
        if (!sbox_table[j])
        {
            printf("ERROR! Over memory to 'sbox_table[%d]'", j);
            exit(1);
        }
    }

    //Open tables
    tables = fopen("tables.txt", "r");

    if (!tables)
    {
        printf("ERROR! File 'tables.txt' not found!");
        exit(1);
    }

    //initial_permutation
    for (i = 0; i < 64; i++)
    {
        fscanf(tables, "%d", initial_permutation + i);
    }
    //final_permutation
    for (i = 0; i < 64; i++)
    {
        fscanf(tables, "%d", final_permutation + i);
    }
    //expansion
    for (i = 0; i < 48; i++)
    {
        fscanf(tables, "%d", expansion + i);
    }
    //permutation_32
    for (i = 0; i < 32; i++)
    {
        fscanf(tables, "%d", permutation_32 + i);
    }
    //compress_56
    for (i = 0; i < 56; i++)
    {
        fscanf(tables, "%d", compress_56 + i);
    }
    //shift_left
    for (i = 0; i < 16; i++)
    {
        fscanf(tables, "%d", shift_left + i);
    }
    //compress_48
    for (i = 0; i < 48; i++)
    {
        fscanf(tables, "%d", compress_48 + i);
    }
    //sbox_table
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            fscanf(tables, "%d", sbox_table[j] + i);
        }
    }

    //Close Tables
    fclose(tables);
}

void print_tables(){
    int i;

    printf("\ninitial_permutation");
    for (i = 0; i < 64; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", initial_permutation[i]);
    }
    //final_permutation
    printf("\n\nfinal_permutation");
    for (i = 0; i < 64; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", final_permutation[i]);
        
    }

    //expansion
    printf("\n\nexpansion");
    for (i = 0; i < 48; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", expansion[i]);
        
    }
    //permutation_32
    printf("\n\npermutation_32");
    for (i = 0; i < 32; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", permutation_32[i]);
        
    }
    //compress_56
    printf("\n\ncompress_56");
    for (i = 0; i < 56; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", compress_56[i]);
        
    }
    //shift_left
    printf("\n\nshift_left");
    for (i = 0; i < 16; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%d ", shift_left[i]);
    }
    //compress_48
    printf("\n\ncompress_48");
    for (i = 0; i < 48; i++)
    {
        if (!(i % 8))
            printf("\n");
        printf("%2d ", compress_48[i]);
        
    }
    //sblock_table
    printf("\n\nsblock_table");

    char j;
    for (j = 0; j < 8; j++)
    {
        printf("\n\ns%d", j);
        for (i = 0; i < 64; i++)
        {
            if (!(i % 8))
                printf("\n");
            printf("%2d ", sbox_table[j][i]);
        }
    }
}

int main()
{
    import_values();
    sort_sbox_table();
    //print_tables();
}