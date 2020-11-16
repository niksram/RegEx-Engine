//S NIKHIL RAM
//PES1201801972

#include <stdio.h>
#include <stdlib.h>
#define STRSIZE 5000 // SIZE OF INPUT STRING
#define PATSIZE 2000 // SIZE OF INPUT PATTERN
#define NUMSIZE 10   // SIZE OF NUMBER

typedef struct SPAN // a structure which is returned which stores the span of the match string according to the pattern
{
    int valid;   // a boolean value which indicates whether a match is found
    char *start; // start pointer of the substring
    char *end;   // end pointer of the substring
} SPAN;

typedef struct BUCKET // a structure which preserves a character class
{
    int range_array_len;     // the length of the range array
    int indiv_array_len;     // the length if the indivdual array
    char *range_array_start; // the range array which consists of corresponding starting character of range
    char *range_array_end;   // the range array which consists of corresponding ending character of range
    char *indiv_array;       // the array which preserves the individual characters
} BUCKET;

BUCKET *create_bucket(int range_len, int indiv_len);                                   // creating the character class bucket
SPAN *re_init_span(char *);                                                            // initialise a span with the given character as the start point
SPAN *re_match(char *pat, char *text);                                                 // the wrapper function which performs the regex search
void re_match_here(char *pat, char *text, SPAN *span);                                 // a recursive function which is used to match the substring
void re_match_star(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy);     // matching a star
void re_match_plus(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy);     // matching a plus
void re_match_question(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy); // matching a question
int re_in_bucket(BUCKET *bucket, char c);                                              // checks whether a character is in a bucket
void free_bucket(BUCKET *bucket);                                                      // freeing a bucket
int bucket_maker(const char *s, BUCKET *bucket);                                       // generating a bucket given a character class substring

SPAN *re_match(char *pat, char *text) // wrapper function
{
    SPAN *span = re_init_span(text);
    if (*pat == '^') // anchor
    {
        span->end = span->start = text;
        re_match_here(pat + 1, text, span); // attempt to match from the start point
    }
    else
    {
        do
        {
            span->end = span->start = text;
            re_match_here(pat, text, span); // match iteratively from different startig points
            if (span->valid)                // if match is a success then return
                return span;
            text++;
        } while (*text != '\0' && *text != '\n'); // attempt until end of string or successful match
    }
    return span; // return in case of a failed match
}

void re_match_here(char *pat, char *text, SPAN *span)
{
    span->end = text;                     // span end is initially set to the current point
    if (pat[0] == '\0' || pat[0] == '\n') // if its the end of the pattern
    {
        span->valid = 1; // hench match is set as one
        return;
    }
    if (pat[0] == '$' && (pat[1] == '\0' || pat[1] == '\n')) // if '$' symbol is found
    {
        if (*text == '\0' || *text == '\n') // match valid is set 1 if match is found
            span->valid = 1;
        return;
    }
    BUCKET *bucket = create_bucket(10, 96); // a bucket is created with arbitrary size (can be altered)
    int close;                              // a variable used for reference to point to the character to be checked at various stages
    if (pat[0] == '[')                      // when character class is found
    {
        close = bucket_maker(pat, bucket); // bucket maker is called upon
    }
    else if (pat[0] == '\\') // when backslash if found
    {
        if (pat[1] == 'w') // if its '\w'
        {
            bucket_maker("[A-Za-z0-9_]", bucket);
        }
        else if (pat[1] == 'd') // if its '\d'
        {
            bucket_maker("[0-9]", bucket);
        }
        else if (pat[1] == 's') // if its '\s'
        {
            bucket_maker("[ \t]", bucket);
        }
        else // else treat it as a normal character
        {
            bucket->indiv_array[bucket->indiv_array_len++] = pat[1];
        }
        close = 1; // increment by pattern point
    }
    else if (pat[0] == '.') // if its .
    {
        bucket_maker("[.]", bucket);
        close = 0;
    }
    else // else if its a normal single character
    {
        bucket->indiv_array[bucket->indiv_array_len++] = pat[0];
        close = 0;
    }
    if (pat[close + 1] == '*') // star
    {
        if (pat[close + 2] == '?')
            re_match_star(bucket, pat + close + 3, text, span, 0); //non greedy
        else
            re_match_star(bucket, pat + close + 2, text, span, 1); // greedy
    }
    else if (pat[close + 1] == '+') // plus (dealt same like star)
    {
        if (pat[close + 2] == '?')
            re_match_plus(bucket, pat + close + 3, text, span, 0); //non greedy
        else
            re_match_plus(bucket, pat + close + 2, text, span, 1); // greedy
    }
    else if (pat[close + 1] == '?') // question (dealt same like star)
    {
        if (pat[close + 2] == '?')
            re_match_question(bucket, pat + close + 3, text, span, 0); //non greedy
        else
            re_match_question(bucket, pat + close + 2, text, span, 1); // greedy
    }
    else // else treat it as a single match
    {
        if (re_in_bucket(bucket, *text))                    // checked whether the character belong to the bucket
            re_match_here(pat + close + 1, text + 1, span); //recursively the function is called upon
    }
    free_bucket(bucket); // bucket is freed
    return;
}

void re_match_star(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy) // matching star
{
    if (!greedy) // if non greedy
    {
        re_match_here(pat, text, span); // in case of star, match of none is attempted first
        if (span->valid)                // if span is valid
            return;
        while (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text)) // else iteratively the string is processed until the match exists
        {
            re_match_here(pat, ++text, span); // the recursive function is called upon
            if (span->valid)                  // if its valid, then its returned
                return;
        }
    }
    else // greedy
    {
        int count = 0; // counter
        while (*(text + count) != '\0' && *(text + count) != '\n' && re_in_bucket(bucket, *(text + count)))
        {
            count++; // counter is incremented till the match (present in bucket) in a greedy fashion
        }
        count--;            // the match point is count-1
        while (count >= -1) //the match is then performed recursively from the last point till the beginning
        {
            re_match_here(pat, text + count + 1, span);
            if (span->valid)
                return;
            count--;
        }
    }
    span->valid = 0; // when all attempts are unsuccessful
    return;
}

void re_match_plus(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy) // matching plus
{
    if (!greedy) // if non greedy
    {
        while (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text)) // start from first character match till possible
        {
            re_match_here(pat, ++text, span); // keep attempting till successful match
            if (span->valid)
                return;
        }
        span->valid = 0; // else return as no match
        return;
    }
    else // greedy
    {
        int count = 0; // counter
        while (*(text + count) != '\0' && *(text + count) != '\n' && re_in_bucket(bucket, *(text + count)))
        {
            count++; // inremented till match (present in bucket) is possible
        }
        count--;
        while (count >= 0)
        {
            re_match_here(pat, text + count + 1, span); //the match is then performed recursively from the last point till the beginning
            if (span->valid)                            // if valid, then return
                return;
            count--;
        }
        span->valid = 0; // else set invalid
    }
    return;
}

void re_match_question(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy)
{
    if (!greedy) // non greedy
    {
        re_match_here(pat, text, span); // try match iitially for empty string
        if (span->valid)
            return;
        if (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text)) // then try for single match
        {
            re_match_here(pat, ++text, span);
        }
    }
    else // greedy
    {
        if (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text)) // try single match initially
        {
            re_match_here(pat, text + 1, span);
            if (span->valid)
                return;
        }
        re_match_here(pat, text, span); // then try for empty string match
    }
}

SPAN *re_init_span(char *c) // initialise span structure
{
    SPAN *span = (SPAN *)malloc(sizeof(SPAN)); // malloc
    span->valid = 0;                           // set initially span is invalid
    span->start = c;                           // set start point as the given pointer
    return span;
}

BUCKET *create_bucket(int range_len, int indiv_len) // range_len is arbitrary length of range_array, indiv_len is arbitrary length of indiv_array
{
    BUCKET *bucket = (BUCKET *)malloc(sizeof(BUCKET)); // malloc
    bucket->range_array_len = 0;                       // the following lengths determine till what point are the buckets filled
    bucket->indiv_array_len = 0;
    bucket->indiv_array = (char *)malloc(indiv_len * sizeof(char));       // indicates individual character array length to match individual characters
    bucket->range_array_start = (char *)malloc(range_len * sizeof(char)); // stores the start point of the given range
    bucket->range_array_end = (char *)malloc(range_len * sizeof(char));   // stores the end point of the given range
    return bucket;                                                        // return bucket
}

void free_bucket(BUCKET *bucket) // free the bucket
{
    free(bucket->indiv_array);
    free(bucket->range_array_start);
    free(bucket->range_array_end);
    free(bucket);
}

int re_in_bucket(BUCKET *bucket, char c) // check whether the given character belongs to the given bucket (character class)
{
    for (int i = 0; i < bucket->range_array_len; i++)
    {
        if (c >= bucket->range_array_start[i] && c <= bucket->range_array_end[i])
        {
            return 1; // if the given character belongs to any given range
        }
    }
    for (int i = 0; i < bucket->indiv_array_len; i++)
    {
        if (c == bucket->indiv_array[i])
        {
            return 1; // if the given character belongs to any individual characters
        }
    }
    return 0; // else return 0
}

int bucket_maker(const char *pat, BUCKET *bucket) // given a character valid class string, it adds the character class to the given bucket
{
    int close = 1; // reference counter to traverse the string
    while (pat[close] != ']')
    {
        if (pat[close] == '-' && pat[close - 1] != '[' && pat[close + 1] != ']') // if - if found between 2 valid characters (indicating range)
        {
            bucket->range_array_start[bucket->range_array_len] = pat[close - 1];
            bucket->range_array_end[bucket->range_array_len++] = pat[close + 1];
            close++;
        }
        else if (pat[close] == '\\') // if backslash is found
        {
            if (pat[close + 1] == 'w') // \w, then recursively the bucket maker function class is called with the given character class
            {
                bucket_maker("[A-Za-z0-9_]", bucket);
            }
            else if (pat[close + 1] == 'd') // \d, then recursively the bucket maker function class is called with the given character class
            {
                bucket_maker("[0-9]", bucket);
            }
            else // else treat as a normal character
            {
                bucket->indiv_array[bucket->indiv_array_len++] = pat[close + 1];
            }
            close++;
        }
        else if (pat[close] == '.') // in case of . , the ascii range is assumed 0-127
        {
            bucket->range_array_start[bucket->range_array_len] = 0;
            bucket->range_array_end[bucket->range_array_len++] = 127;
        }
        else if (pat[close + 1] != '-') // this is to include individual characters which donot belong to a range
        {
            bucket->indiv_array[bucket->indiv_array_len++] = pat[close];
        }
        close++;
    }
    return close;
}

char *string_strip(char *s, long int len) // this function is used just to strip the string (through fgets) from endline and add null character
{
    long int i = 0;
    char *strip = (char *)calloc((len + 1), sizeof(char));
    while (s[i] != '\n' && s[i] != '\0')
    {
        strip[i] = s[i];
        i++;
    }
    strip[i] = '\0';
    return strip;
}

int string_to_num(char *s) // convert a string from fgets to number, this is used in main as intermixing scanf and fgets leads to few bugs
{
    int n = 0, i = 0;
    while (s[i] <= '9' && s[i] >= '0') // individually determine digits and multiply
    {
        n *= 10;
        n += (s[i] - '0');
        i++;
    }
    return n;
}

int main()
{
    char *string = (char *)calloc((STRSIZE + 1), sizeof(char)); // string
    char *pat = (char *)calloc((PATSIZE + 1), sizeof(char));    // pattern
    char *num = (char *)calloc((NUMSIZE + 1), sizeof(char));    // number through fgets
    fgets(string, STRSIZE, stdin);
    fgets(num, NUMSIZE, stdin);
    char *stripped_string = string_strip(string, STRSIZE); // strip the string
    int n = string_to_num(num);                            // generate number from number string
    free(num);
    free(string);
    for (int i = 0; i < n; i++) // reading each pattern
    {
        fgets(pat, PATSIZE, stdin); // pattern
        char *stripped_pat = string_strip(pat, PATSIZE);
        SPAN *span = re_match(stripped_pat, stripped_string);                // generate match
        if (span->valid)                                                     // ** important point is the span (1,n) which is generated actually includes the match (1,n-1) (just like python)
        {                                                                    // ** hence this is converted to the convention given by them
            long int start = (span->start - stripped_string) / sizeof(char); // finding numerical range from pointers
            long int end = (span->end - stripped_string) / sizeof(char);
            if (start == end) // modifying according to given convention which is both left and right inclusive
                start = end = 0;
            else
                end--;
            printf("1 %ld %ld\n", start, end);
        }
        else
        {
            printf("0\n");
        }
        free(span); // free the memory
        free(stripped_pat);
    }
    free(stripped_string);
    free(pat);
    return 0;
}