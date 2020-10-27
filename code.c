#include <stdio.h>
#include <stdlib.h>

//#define DEBUG
#define DECRYPT_STEP

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

uint64_t  permutation(uint64_t  value, char *positions, int sizef, int sizei)
{
    uint64_t  ret = 0;
    char i;
    
    for (i = 0; i < sizef; i++)
    {
    //        printf("%c %2d",(i%8)?'\0':'\n',positions[i]);
        bit_recive(ret, sizef-i-1, bit_value(value,sizei-positions[i]));
    }
    #ifdef DEBUG
        printf("%016llx %016llx\n",value,ret);
        print_8bytes(ret);
        printf("\n");
    #endif
    return ret;
}

uint64_t  swapLR(uint64_t  value, int axis)
{
    return (value >> axis) & bit_range(0, axis)           // L to R
           | (value << axis) & bit_range(axis, 2 * axis); // R to L
}

uint64_t  shiftL_circular_split(uint64_t  value, int axis, int shift)
{
    uint64_t temp = (value >> axis - shift) & (bit_range(0, shift) | bit_range(axis, axis + shift));
    //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    uint64_t ret = (value << shift) // Faz o deslocamento normal (com os zeros no inicio)
               & (bit_range(shift, axis) | bit_range(axis + shift, axis * 2))
           // zera todos os valores que entraram nas posições dos estourados
           | temp; // inclui os estourados em suas específicas posições
    #ifdef DEBUG
        printf("%016llx %016llx\n",value,ret);
        print_8bytes(ret);
        printf("\n");
    #endif
    return ret;
}

uint64_t  shiftR_circular_split(uint64_t  value, int axis, int shift)
{
    uint64_t temp = (value << axis - shift) & (bit_range(axis - shift, axis) | bit_range(2 * axis - shift, 2 * axis));
    //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    uint64_t ret = (value >> shift) // Faz o deslocamento normal (com os zeros no final)
               & (bit_range(0, axis - shift) | bit_range(axis, 2 * axis - shift))
           // zera todos os valores que entraram nas posições dos estourados
           | temp; // inclui os estourados em suas específicas posições
    #ifdef DEBUG
        printf("%016llx %016llx\n",value,ret);
        print_8bytes(ret);
        printf("\n");
    #endif

    return ret;
}

uint64_t  s_box(uint64_t  value, char **table)
{
    char i;
    uint64_t  ret = 0;
    int index;
    for (i = 0; i < 8; i++)
    {
        index = (value >> (i * 6)) & bit_range(0, 6);
        ret = ret | (((uint64_t )(table[7-i][index])) << i * 4);
    }
    #ifdef DEBUG
    printf("sbox %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c %c%c%c%c%c%c\n",TO_BIN(value>>40),TO_BIN(value>>32),TO_BIN(value>>24),TO_BIN(value>>16),TO_BIN(value>>8),TO_BIN(value));
    printf("      %c%c%c%c   %c%c%c%c   %c%c%c%c   %c%c%c%c   %c%c%c%c   %c%c%c%c   %c%c%c%c   %c%c%c%c \n",TO_BIN(ret>>24),TO_BIN(ret>>16),TO_BIN(ret>>8),TO_BIN(ret));
    printf("\n");
    #endif

    return ret;
}

void sort_sbox_table()
{
    char **table2 = (char **)malloc(sizeof(char *) * 8);
    if(!table2)
    {
        printf("ERROR! Missing memory for 'table2'");
        exit(1);
    }
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

uint64_t  iteration(uint64_t  value, uint64_t  key_48)
{
    uint64_t  ret = 0;

    //F (R,Ki)
    #ifdef DEBUG
    printf("expansion 48 ");
    #endif
    uint64_t  f = permutation(value, expansion, 48,32);

    #ifdef DEBUG
    printf("xor %016llx + %016llx = %016llx\n\n",f,key_48,f ^ key_48);
    #endif

    f = f ^ key_48;
    f = s_box(f, sbox_table);
    #ifdef DEBUG
    printf("permutation 32 ");
    #endif
    f = permutation(f, permutation_32, 32,32);

    //swap
    ret = swapLR(value, 32);

    #ifdef DEBUG
    printf("swap %016llx %016llx\n\n",value,ret);
    #endif

    //xor
    #ifdef DEBUG
    printf("xor %016llx + %016llx = %016llx\n\n",ret,f,ret ^ f);
    #endif
    return ret ^ f;
}

uint64_t  key_i(uint64_t  value, char i)
{
    switch (mode)
    {
    case encrypt:
        #ifdef DEBUG
        printf("shift split L %d %d ",i,shift_left[i]);
        #endif
        return shiftL_circular_split(value, 28, shift_left[i]);
    case decrypt:
        #ifdef DEBUG
        printf("shift split R %d %d ",i,shift_left[i]);
        #endif
        return shiftR_circular_split(value, 28, shift_left[i]);
    }
}

uint64_t  key_prepare(char * key)
{
    uint64_t  value = 0;
    char i=0;
    while(key[i]){
        value = value<<8 | key[i++];
    }
    return value;
}

int message_prepare(uint64_t  * blocks, char * message){
    int i=0,j=-1;
    while (message[i]){
        if(!(i%8))
            blocks[++j]=0;
        blocks[j] = blocks[j]<<8 | message[i];
        i++;
    }
    while (i%8){
        blocks[j] = blocks[j]<<8;
        i++;
    }
    blocks[j+1] = 0;
    return j+1;
}

long long block_encrypt(uint64_t  value, uint64_t  key)
{
    char i;
    uint64_t ret;
    #ifdef DEBUG
    printf("initial permutation ");
    #endif
    value = permutation(value, initial_permutation, 64,64);

    for (i = 0; i < 16; i++)
    {
        if(mode == encrypt)
            key = key_i(key, i);
        #ifdef DEBUG
        printf("compress key 48 ");
        #endif
        value = iteration(value, permutation(key, compress_48, 48,56));
        if(mode == decrypt)
            key = key_i(key, 15 - i);
    }
    ret = swapLR(value, 32);
    #ifdef DEBUG
    printf("swap %016llx %016llx\n\n",value,ret);
    #endif

    #ifdef DEBUG
    printf("final permutation ");
    #endif
    return permutation(ret, final_permutation, 64,64);
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

void test_block(){
    uint64_t  key,value;

    
    sscanf(" 133457799BBCDFF1 0123456789ABCDEF","%llx %llx",&key,&value);//85E813540F0AB405
    //sscanf(" 0E329232EA6D0D73 8787878787878787","%llx %llx",&key,&value);//0000000000000000

    mode = encrypt;

    #ifdef DEBUG
    printf("compress_56 ");
    #endif
    key = permutation(key, compress_56, 56,64);
    
    printf("%016llx %016llx\n\n",key,value);

    value = block_encrypt(value,key);
    printf("encrypted: %016llx \n",value);

    #ifdef DECRYPT_STEP
    mode = decrypt;
    printf("%016llx %016llx\n\n",key,value);

    value = block_encrypt(value,key);
    printf("decrypted: %016llx \n",value);
    #endif
}

void test_string(){
    char i;
    char string[] = "Your lips are smoother than vaseline";
    uint64_t  message[80];
    uint64_t key;
    int message_length;
    import_values();
    sort_sbox_table();
    
    message_length = message_prepare(message,string);
    message[message_length-1] = message[message_length-1] | 0x0D0A0000;

    key = 0x0E329232EA6D0D73;


    #ifdef DEBUG
    printf("compress_56 ");
    #endif
    key = permutation(key, compress_56, 56,64);

    
    printf("string:\n%s\n",string);
    
    mode = encrypt;
    
    printf("plain text:\n");
    for(i=0;i<message_length;i++){
        printf("%016llx ",message[i]);
    }
    printf("\ncypher text:\n");
    for(i=0;i<message_length;i++){
        message[i] = block_encrypt(message[i],key);
        printf("%016llx ",message[i]);
    }

    #ifdef DECRYPT_STEP
    mode = decrypt;

    printf("\nplain text:\n");
    for(i=0;i<message_length;i++){
        message[i] = block_encrypt(message[i],key);
        printf("%016llx ",message[i]);
    }
    #endif
}

int main(int argc,char ** argv)
{
    //test_block();
    //test_string();
    int i;
    char key_string[9];
    uint64_t key;
    uint64_t value;
    FILE *fplain_text,
        *fcypher_text;

    if(argc<=1){
        printf("ERRO! Inform the file's name to be encrypted.");
        exit(1);
    }
    fplain_text = fopen(argv[1],"rb");
    if(!fplain_text){
        printf("ERRO! File not found");
        exit(1);
    }

    if(argc<=2){
        fcypher_text = fopen("cypher.txt","ab+");
    }else{
        fcypher_text = fopen(argv[2],"ab+");
    }

    if(!fcypher_text){
        printf("ERRO! Can't open or create out file!");
        exit(1);
    }

    printf("Inform key: ");
    scanf("%s",key_string);
    key = key_prepare(".0[@>?z$");//

    printf("%016llx\n",key);

    printf("Choose the process:\n\t1-encrypt\t2-decrypt\n\tinput number:");
    scanf("%d",&i);
    mode = i!=0;

    while(i = fread(&value,8,1,fplain_text),i==1){
        printf("%016llx\n",value);
        value = block_encrypt(value,key);
        printf("%016llx\n",value);
        fwrite(&value,8,1,fcypher_text);
    }
    printf("%016llx %d\n",value,i);
    
    fclose(fplain_text);
    fclose(fcypher_text);
}