#include <stdio.h>
#include <stdlib.h>

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

SPAN *re_match(char *pat, char *text)
{
    SPAN *span = re_init_span(text);
    if (*pat == '^')
    {
        re_match_here(pat + 1, text, span);
    }
    do
    {
        span->end = span->start = text;
        re_match_here(pat, text, span);
        if (span->valid)
            return span;
        text++;
    } while (*text != '\0' && *text != '\n');
    return span;
}

void re_match_here(char *pat, char *text, SPAN *span)
{
    span->end = text;
    if (pat[0] == '\0')
    {
        span->valid = 1;
        span->end = text;
        return;
    }
    if (pat[0] == '$' && pat[1] == '\0')
    {
        if (*text == '\0')
            span->valid = 1;
        return;
    }
    BUCKET *bucket = create_bucket(30);
    int close;
    if (pat[0] == '[')
    {
        close = 1;
        while (pat[close] != ']')
        {
            if (pat[close] == '-')
            {
                bucket->range_array_start[bucket->range_array_len] = pat[close - 1];
                bucket->range_array_end[bucket->range_array_len++] = pat[close + 1];
            }
            if (pat[close] != '-' && pat[close - 1] != '-' && pat[close + 1] != '-')
            {
                bucket->indiv_array[bucket->indiv_array_len++] = pat[close];
            }
            close++;
        }
    }
    else if (pat[0] == '\\' && (pat[1] == 'w' || pat[1] == 'd'))
    {
        bucket->range_array_len = 1;
        bucket->range_array_start[0] = '0';
        bucket->range_array_end[0] = '9';
        if (pat[1] == 'w')
        {
            bucket->range_array_len = 3;
            bucket->range_array_start[1] = 'A';
            bucket->range_array_start[2] = 'a';
            bucket->range_array_end[1] = 'Z';
            bucket->range_array_end[2] = 'z';
            bucket->indiv_array_len = 1;
            bucket->indiv_array[0] = '_';
        }
        close = 1;
    }
    else
    {
        bucket->indiv_array_len = 1;
        bucket->indiv_array[0] = pat[0];
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
    return;
}

void re_match_star(BUCKET *bucket, char *pat, char *text, SPAN *span, int greedy)
{
    if (!greedy)
    {
        re_match_here(pat, text, span);
        if (span->valid)
            return;
        while (*text != '\0' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, ++text, span);
            if (span->valid)
                return;
        }
    }
    else
    {
        int count = 0;
        while (*(text + count) != '\0' && re_in_bucket(bucket, *(text + count)))
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
        while (*text != '\0' && re_in_bucket(bucket, *text))
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
        while (*(text + count) != '\0' && re_in_bucket(bucket, *(text + count)))
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
        if (*text != '\0' && re_in_bucket(bucket, *text))
        {
            re_match_here(pat, ++text, span);
        }
    }
    else
    {
        if (*text != '\0' && re_in_bucket(bucket, *text))
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

int main()
{
    char a[10] = "9\0";
    char pat[10] = "\\w+?\\d+\0";
    SPAN *span = re_match(pat, a);
    printf("%d %ld %ld\n", span->valid, (span->start - a) / sizeof(char), (span->end - a) / sizeof(char));
    return 0;
}