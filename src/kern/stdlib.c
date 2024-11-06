#include "stdlib.h"

// Function to convert a string to an integer
int atoi(const char *str)
{
  int res = 0;  // Initialize result
  int sign = 1; // Initialize sign as positive
  int i = 0;    // Initialize index of first digit

  // If number is negative, then update sign
  if (str[0] == '-')
  {
    sign = -1;
    i++; // Also update index of first digit
  }

  // Iterate through all digits of input string and update result
  for (; str[i] != '\0'; ++i)
    res = res * 10 + str[i] - '0';

  // Return result with sign
  return sign * res;
}

// Function to convert an integer to a string
char *itoa(int num, char *str, int base)
{
  int i = 0;
  int isNegative = 0;

  // Handle 0 explicitly, otherwise empty string is printed for 0
  if (num == 0)
  {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }

  // In standard itoa(), negative numbers are handled only with base 10.
  // Otherwise numbers are considered unsigned.
  if (num < 0 && base == 10)
  {
    isNegative = 1;
    num = -num;
  }

  // Process individual digits
  while (num != 0)
  {
    int rem = num % base;
    str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
    num = num / base;
  }

  // If number is negative, append '-'
  if (isNegative)
    str[i++] = '-';

  str[i] = '\0'; // Append string terminator

  // Reverse the string
  int start = 0;
  int end = i - 1;
  while (start < end)
  {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }

  return str;
}

char *strtok(char *str, const char *delim)
{
  static char *ptr;
  if (str != NULL)
  {
    ptr = str;
  }
  else if (ptr == NULL)
  {
    return NULL;
  }

  char *start = ptr;
  while (*ptr != '\0')
  {
    const char *d = delim;
    while (*d != '\0')
    {
      if (*ptr == *d)
      {
        *ptr = '\0';
        ptr++;
        if (start == ptr)
        {
          start++;
          continue;
        }
        return start;
      }
      d++;
    }
    ptr++;
  }
  return start;
}