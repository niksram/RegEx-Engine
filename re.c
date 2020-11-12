#include <stdio.h>
#include <stdlib.h>
#define STRSIZE 4000
#define PATSIZE 1000

typedef struct SPAN
{
    int valid;
    char *start;
    char *end;
} SPAN;

typedef struct BUCKET
{
    int range_array_len;
    int indiv_array_len;
    char *range_array_start;
    char *range_array_end;
    char *indiv_array;
} BUCKET;

BUCKET *create_bucket(int len);
SPAN *re_init_span(char *);
SPAN *re_match(char *pat, char *text);
void re_match_here(char *pat, char *text, SPAN *span);
void re_match_star(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy);
void re_match_plus(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy);
void re_match_question(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy);
int re_in_bucket(BUCKET *bucket, char c);
void free_bucket(BUCKET *bucket);
int bucket_maker(const char *s, BUCKET *bucket);

SPAN *re_match(char *pat, char *text)
{
    SPAN *span = re_init_span(text);
    if (*pat == '^')
    {
        span->end = span->start = text;
        re_match_here(pat + 1, text, span);
    }
    else
    {
        do
        {
            span->end = span->start = text;
            re_match_here(pat, text, span);
            if (span->valid)
                return span;
            text++;
        } while (*text != '\0' && *text != '\n');
    }
    return span;
}

void re_match_here(char *pat, char *text, SPAN *span)
{
    span->end = text;
    if (pat[0] == '\0' || pat[0] == '\n')
    {
        span->valid = 1;
        span->end = text;
        return;
    }
    if (pat[0] == '$' && (pat[1] == '\0' || pat[1] == '\n'))
    {
        if (*text == '\0' || *text == '\n')
            span->valid = 1;
        return;
    }
    BUCKET *bucket = create_bucket(10);
    int close;
    if (pat[0] == '[')
    {
        close = bucket_maker(pat, bucket);
    }
    else if (pat[0] == '\\')
    {
        if (pat[1] == 'w')
        {
            bucket_maker("[A-Za-z0-9_]", bucket);
        }
        else if (pat[1] == 'd')
        {
            bucket_maker("[0-9]", bucket);
        }
        else
        {
            bucket->indiv_array[bucket->indiv_array_len++] = pat[1];
        }
        close = 1;
    }
    else if (pat[0] == '.')
    {
        bucket_maker("[.]", bucket);
    }
    else
    {
        bucket->indiv_array[bucket->indiv_array_len++] = pat[0];
        close = 0;
    }
    if (pat[close + 1] == '*')
    {
        if (pat[close + 2] == '?')
            re_match_star(bucket, pat + close + 3, text, span, 0);
        else
            re_match_star(bucket, pat + close + 2, text, span, 1);
    }
    else if (pat[close + 1] == '+')
    {
        if (pat[close + 2] == '?')
            re_match_plus(bucket, pat + close + 3, text, span, 0);
        else
            re_match_plus(bucket, pat + close + 2, text, span, 1);
    }
    else if (pat[close + 1] == '?')
    {
        if (pat[close + 2] == '?')
            re_match_question(bucket, pat + close + 3, text, span, 0);
        else
            re_match_question(bucket, pat + close + 2, text, span, 1);
    }
    else
    {
        if (re_in_bucket(bucket, *text))
            re_match_here(pat + close + 1, text + 1, span);
    }
    free_bucket(bucket);
    return;
}

void re_match_star(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy)
{
    if (!greedy)
    {
        re_match_here(pat, text, span);
        if (span->valid)
            return;
        while (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, ++text, span);
            if (span->valid)
                return;
        }
    }
    else
    {
        int count = 0;
        while (*(text + count) != '\0' && *(text + count) != '\n' && re_in_bucket(bucket, *(text + count)))
        {
            count++;
        }
        count--;
        while (count >= -1)
        {
            re_match_here(pat, text + count + 1, span);
            if (span->valid)
                return;
            count--;
        }
    }
    span->valid = 0;
    return;
}

void re_match_plus(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy)
{
    if (!greedy)
    {
        while (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, ++text, span);
            if (span->valid)
                return;
        }
        span->valid = 0;
        return;
    }
    else
    {
        int count = 0;
        while (*(text + count) != '\0' && *(text + count) != '\n' && re_in_bucket(bucket, *(text + count)))
        {
            count++;
        }
        count--;
        while (count >= 0)
        {
            re_match_here(pat, text + count + 1, span);
            if (span->valid)
                return;
            count--;
        }
        span->valid = 0;
    }
    return;
}

void re_match_question(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy)
{
    if (!greedy)
    {
        re_match_here(pat, text, span);
        if (span->valid)
            return;
        if (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, ++text, span);
        }
    }
    else
    {
        if (*text != '\0' && *text != '\n' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, text + 1, span);
            if (span->valid)
                return;
        }
        re_match_here(pat, text, span);
    }
}

SPAN *re_init_span(char *c)
{
    SPAN *span = (SPAN *)malloc(sizeof(SPAN));
    span->valid = 0;
    span->start = c;
    return span;
}

BUCKET *create_bucket(int len)
{
    BUCKET *bucket = (BUCKET *)malloc(sizeof(BUCKET));
    bucket->range_array_len = 0;
    bucket->indiv_array_len = 0;
    bucket->indiv_array = (char *)malloc(len * sizeof(char));
    bucket->range_array_start = (char *)malloc(len * sizeof(char));
    bucket->range_array_end = (char *)malloc(len * sizeof(char));
    return bucket;
}

void free_bucket(BUCKET *bucket)
{
    free(bucket->indiv_array);
    free(bucket->range_array_start);
    free(bucket->range_array_end);
    free(bucket);
}

int re_in_bucket(BUCKET *bucket, char c)
{
    for (int i = 0; i < bucket->range_array_len; i++)
    {
        if (c >= bucket->range_array_start[i] && c <= bucket->range_array_end[i])
        {
            return 1;
        }
    }
    for (int i = 0; i < bucket->indiv_array_len; i++)
    {
        if (c == bucket->indiv_array[i])
        {
            return 1;
        }
    }
    return 0;
}

int bucket_maker(const char *pat, BUCKET *bucket)
{
    int close = 1;
    while (pat[close] != ']')
    {
        if (pat[close] == '-' && pat[close - 1] != '[' && pat[close - 1] != '\\' && pat[close + 1] != ']')
        {
            bucket->range_array_start[bucket->range_array_len] = pat[close - 1];
            bucket->range_array_end[bucket->range_array_len++] = pat[close + 1];
            close++;
        }
        else if (pat[close - 1] == '\\')
        {
            if (pat[1] == 'w')
            {
                bucket_maker("[A-Za-z0-9_]", bucket);
            }
            else if (pat[1] == 'd')
            {
                bucket_maker("[0-9]", bucket);
            }
            else
            {
                bucket->indiv_array[bucket->indiv_array_len++] = pat[close];
            }
        }
        else if (pat[close] == '.')
        {
            bucket->range_array_start[bucket->range_array_len] = 0;
            bucket->range_array_end[bucket->range_array_len++] = 127;
        }
        else if (pat[close] != '\\' && pat[close+1] != '-')
        {
            bucket->indiv_array[bucket->indiv_array_len++] = pat[close];
        }
        close++;
    }
    return close;
}

int main()
{
    char *string = (char *)malloc((STRSIZE + 1) * sizeof(char));
    char *pat = (char *)malloc((PATSIZE + 1) * sizeof(char));
    int n;
    fgets(string, STRSIZE + 1, stdin);
    scanf("%d\n", &n);
    for (int i = 0; i < n; i++)
    {
        fgets(pat, PATSIZE + 1, stdin);
        SPAN *span = re_match(pat, string);
        if (span->valid)
        {
            long int start = (span->start - string) / sizeof(char);
            long int end = (span->end - string) / sizeof(char);
            if (start == end)
                start = end = 0;
            else
                end--;
            printf("1 %ld %ld\n", start, end);
        }
        else
        {
            printf("0\n");
        }
        free(span);
    }
    free(string);
    free(pat);
    return 0;
}