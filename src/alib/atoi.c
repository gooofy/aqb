
#define bool char
#define TRUE 1
#define FALSE 0

/* A utility function to reverse a string  */
static void reverse(char *str, int length) 
{ 
    int start = 0; 
    int end   = length -1; 
    while (start < end) 
    { 
        char c = *(str+start);
        *(str+start) = *(str+end);
        *(str+end)   = c;
        start++; 
        end--; 
    } 
} 
  
void itoa(int num, char* str, int base) 
{ 
    int i = 0; 
    bool isNegative = FALSE; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return; 
    } 
  
    // In standard itoa(), negative numbers are handled only with  
    // base 10. Otherwise numbers are considered unsigned. 
    if (num < 0 && base == 10) 
    { 
        isNegative = TRUE; 
        num = -num; 
    } 
  
    // Process individual digits 
    while (num != 0) 
    { 
        int rem = num % base; 
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
        num = num/base; 
    } 
  
    // If number is negative, append '-' 
    if (isNegative) 
        str[i++] = '-'; 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse(str, i); 
} 


