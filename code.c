#include <stdio.h>
#include <stdlib.h>

#include "bitwiseop.h"

typedef enum {
    encrypt=1,decrypt=0
}Mode;

char * initial_permutation,
    *final_permutation,
    *expansion,
    *compress_56,
    *shift_left,
    *compress_48,
    **sbox_table;

Mode mode;


long long int permutation (long long int value, char * positions, int size){
    long long int ret = 0;
    char i;
    for(i=0;i<size;i++){
        bit_copy(ret,value,positions[i]);
    }
    return ret;
}

long long int swapLR (long long int value, int axis){
    return (value>>axis)&bit_range(0,axis) // L to R
            | (value<< axis)&bit_range(axis,2*axis); // R to L
}

long long int shiftL_circular_split (long long int value, int axis, int shift){
    int temp = (value >> axis-shift) & (bit_range(0,shift) | bit_range(axis,axis+shift));
        //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    return (value << shift) // Faz o deslocamento normal (com os zeros no inicio)
        & (bit_range(shift,axis) | bit_range(axis+shift,axis*2))
            // zera todos os valores que entraram nas posições dos estourados
                | temp;// inclui os estourados em suas específicas posições
}

long long int shiftR_circular_split (long long int value, int axis, int shift){
    int temp = (value << axis-shift) & (bit_range(axis-shift,axis) | bit_range(2*axis-shift,2*axis));
        //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    return (value >> shift) // Faz o deslocamento normal (com os zeros no final)
        & (bit_range(0,axis-shift) | bit_range(axis,2*axis-shift))
            // zera todos os valores que entraram nas posições dos estourados
                | temp;// inclui os estourados em suas específicas posições
}

long long int s_box (long long int value,char ** table){
    char i;
    long long int ret = 0;
    for(i=0;i<8;i++){
        ret = ret | (((long long int)(table[i][(value>>(i*6)) & bit_range(0,6)]))<<i*4);
    }
    return ret;
}

void sort_sbox_table(char *** table){
    char ** table2 = (char **) malloc (sizeof(char*)*8);
    char i,j,temp;

    for(i=0;i<8;i++){
        table2[i] = (char *) malloc (sizeof(char)*64);
    }

    for(i=0;i<8;i++){
        for(j=0;j<48;j = (j==15)?32:(j+1)){
            table2[i][2*j] = table[i][j];
            table2[i][2*j+1] = table[i][j+16];
        }
    }

    free(*table);
    *table = table2;
}

long long int iteration (long long int value,long long int key_48){
    long long int ret = 0;

    //F (R,Ki)
    long long int f = permutation(value,expansion,48);
    f = f ^ key_48;
    f = s_box(f,sbox_table);

    //swap
    ret = swapLR(value,32);

    //xor
    return ret ^ f;
}

long long int key_i(long long int value, char i){
    switch (mode)
    {
    case encrypt:
        return shiftL_circular_split(value,28,shift_left[i]);
    case decrypt:   
        return shiftR_circular_split(value,28,shift_left[i]);
    }
}

long long int key_prepare(long long int value){
    return permutation(value,compress_56,56);
}

long long block_encrypt(long long int value,long long int key){
    char i;
    value = permutation(value,initial_permutation,64);

    for(i=0;i<16){
        key = key_i(key,mode?i:(15-i));
        value = iteration(value,permutation(key,compress_48,48));
    }

    return permutation(swapLR(value,32),final_permutation,64);
}

int main(){
    printf("%2x\n",swapLR(0x5A,4));
    printf("%2x\n\n",swapLR(0x857A,4));

    printf("%2x\n",shiftL_circular_split(0x5A,4,1));
    printf("%2x\n",shiftL_circular_split(0x857A,8,2));
    printf("%2x\n",shiftL_circular_split(0x857A,6,2));
    printf("%2x\n\n",shiftL_circular_split(0x857A,4,2));
    
}