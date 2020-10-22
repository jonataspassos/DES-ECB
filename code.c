#include <stdio.h>

#include "bitwiseop.h"

long long int permutation (long long int value, char * positions, int size){
    long long int ret = 0;
    char i;
    for(i=0;i<size;i++){
        bit_copy(ret,value,positions[i]);
    }
    return ret;
}

long long int swapLR (long long int value, int axis){
    return (value>>axis)&bit_on_range(0,axis) // L to R
            | (value<< axis)&bit_on_range(axis,2*axis); // R to L
}

long long int shiftL_circular_split (long long int value, int axis, int shift){
    int temp = (value >> axis-shift) & (bit_on_range(0,shift) | bit_on_range(axis,axis+shift));
        //Salva os que estourariam já na posição nova, com todos os outros valores zerados
    return (value << shift) // Faz o deslocamento normal (com os zeros no inicio)
        & (bit_on_range(shift,axis) | bit_on_range(axis+shift,axis*2))
            // zera todos os valores que entraram nas posições dos estourados
                | temp;// inclui os estourados em suas específicas posições
}

int main(){
    printf("%2x\n",swapLR(0x5A,4));
    printf("%2x\n",swapLR(0x857A,4));
    
}