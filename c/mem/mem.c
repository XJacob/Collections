#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *test_string="123456789";

#define BUFSIZE 16
char buf[BUFSIZE];

/*
 * insert_before('9', 'Z', strlen(test_string), _buf, bufsize);
 * result:
 *    before insert: 123456789
 *    after insert: 12345678Z9
 *
 */
void insert_before(char target, char insert, size_t target_len, char *buf, size_t bufsize)
{
    int len = target_len, i;
    for(i=0; i<len; i++) {
        if (buf[i] == target && len < bufsize) {
            memmove(buf+i+1, buf+i, bufsize-i);
            buf[i] = insert;
            len++;
            i++;
        }
    }
}

void main()
{
    char *_buf = buf;
    int bufsize = BUFSIZE;

    strcpy(_buf, test_string);
    insert_before('9', 'Z', strlen(test_string), _buf, bufsize);
}
