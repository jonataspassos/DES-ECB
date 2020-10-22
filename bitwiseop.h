#ifndef bitwiseop
#define bitwiseop
/*------------------------------------------------------------*/

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

#define bit_on(a,b)(a=a|(1<<b))     // a[b] = 1
#define bit_off(a,b)(a=a&~(1<<b))   // a[b] = 0
#define bit_value(a,b)((a&(1<<b))!=0)  // a[b]
#define bit_copy(a,b,c)(bit_value(b,c)?bit_on(a,c):bit_off(a,c)) // a[c] = b[c]
#define bit_recive(a,b,c)(c?bit_on(a,b):bit_off(a,b)) // a[b] = c
#define bit_on_range(i,f)(((1<<f)-1)^((1<<i)-1))
#define bit_off_range(i,f)(~bit_on_range(i,f))

/*-------------------------------------------------------------*/
#endif