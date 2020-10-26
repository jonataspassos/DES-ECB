#ifndef bitwiseop
#define bitwiseop
/*------------------------------------------------------------*/

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

/**
 * Used on printf to print 8bits in binary
 * printf("..."BIN_PAT"...",TO_BIN(byte));
 * printf("..."BIN_PAT" "BIN_PAT"...",TO_BIN(byte>>8),TO_BIN(byte));
*/
#define BIN_PAT "%c%c%c%c%c%c%c%c"
/**
 * Used on printf to print 8bits in binary
 * Ex.:
 * printf("..."BIN_PAT"...",TO_BIN(byte));
 * printf("..."BIN_PAT" "BIN_PAT"...",TO_BIN(byte>>8),TO_BIN(byte));
*/
#define TO_BIN(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define bit_on(a,b)(a=a|(((uint64_t)1)<<b))     // a[b] = 1
#define bit_off(a,b)(a=a&~(((uint64_t)1)<<b))   // a[b] = 0
#define bit_value(a,b)((a&(((uint64_t)1)<<b))!=0)  // a[b]
#define bit_copy(a,b,c)(bit_value(b,c)?bit_on(a,c):bit_off(a,c)) // a[c] = b[c]
#define bit_recive(a,b,c)(c?bit_on(a,b):bit_off(a,b)) // a[b] = c
#define bit_range(i,f)((f==64?(uint64_t)-1:((((uint64_t)1)<<f)-1))^((((uint64_t)1)<<i)-((uint64_t)1)))

void print_8bytes(uint64_t value){
    printf(BIN_PAT" "BIN_PAT" "BIN_PAT" "BIN_PAT" "BIN_PAT" "BIN_PAT" "BIN_PAT" "BIN_PAT"\n",TO_BIN(value>>56),TO_BIN(value>>48),TO_BIN(value>>40),TO_BIN(value>>32),TO_BIN(value>>24),TO_BIN(value>>16),TO_BIN(value>>8),TO_BIN(value));
}

void index_binary(int size){
    int i;
    for(i=size-1;i>=0;i--){
      printf("%c",i%10?' ':(i/10+'0'));
      if(!(i%8))
        printf(" ");
    }
    printf("\n");
    for(i=size-1;i>=0;i--){
      printf("%d",i%10);
      if(!(i%8))
        printf(" ");
    }
    printf("\n");
    //printf("   6          5          4           3          2          1          0\n");
    //printf("32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210\n");
}

/*-------------------------------------------------------------*/
#endif