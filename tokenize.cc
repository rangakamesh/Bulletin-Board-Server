/*
 * In-place string tokenizer, by Stefan Bruda.  Read the comments in
 * the header before using it.
 */

#include "tokenize.h"

size_t str_tokenize(char* str, char** tokens, const size_t n) {
  size_t tok_size = 1;
  tokens[0] = str;

  size_t i = 0;
  while (i < n) {
    if (str[i] == '=' || str[i] == ' ' ) {
      str[i] = '\0';
      i++;
      for (; i < n && str[i] == ' '; i++)
        /* NOP */;
      if (i < n) {
        tokens[tok_size] = str + i;
        tok_size++;
      }
    }
    else
      i++;
  }

  return tok_size;
}

void trimTrailingSpaces(char* str)
{
    int index, i;
    index = -1;
    i = 0;

    while(str[i] != '\0')
    {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            index= i;
        }
        i++;
    }

    str[index + 1] = '\0';
}

char* tolwr(char* input)
{
  char* output;
  output = new char[strlen(input)];
  output[0]='\0';

  for(int i=0;i<strlen(input);i++)
  {
    char l_let = tolower(input[i]);
    // strcat(output,l_let);
    sprintf(output,"%s%c",output,l_let);
  }

  if(output!=NULL)
  {
    // printf("To Lower invoked for %s. Converted to %s.\n",input,output);
    return output;
  }
  else
  {
    return input;
  }
}

size_t bbline_tokenize(char* str, char** tokens, const size_t n)
{
  size_t tok_size = 1;
  tokens[0] = str;

  size_t i = 0;
  while (i < n) {
    if (str[i] == '/') {
      str[i] = '\0';
      i++;
      for (; i < n && str[i] == ' '; i++)
        /* NOP */;
      if (i < n) {
        tokens[tok_size] = str + i;
        tok_size++;
      }
    }
    else
      i++;
  }

  return tok_size;
}
