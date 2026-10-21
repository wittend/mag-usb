#include <string.h>
#include "node_id.h"

int node_id_is_valid(const char *s)
{
    size_t len;

    if(s == NULL)
    {
        return 0;
    }
    len = strlen(s);
    if(len < 1 || len > NODE_ID_MAX_LEN)
    {
        return 0;
    }
    for(size_t i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)s[i];
        if(c < 0x21 || c > 0x7E || c == '"' || c == '\\')
        {
            return 0;
        }
    }
    return 1;
}
