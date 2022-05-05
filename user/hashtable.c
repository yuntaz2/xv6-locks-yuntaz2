#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DATA_SIZE 1024
#define NBUCKETS 31
#define BUCKET_SIZE 66

typedef struct Data
{
    int secret;
    int ID;
    int dep;
} Data;

Data glob_data[DATA_SIZE];

int hash(int a, int b)
{
    return (a * b) % NBUCKETS;
}

struct bucket
{
    Data *bucket_data[BUCKET_SIZE];
} hashtable[NBUCKETS];

int set_mapping(int s_hash, Data *s_data)
{
    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
        if (hashtable[s_hash].bucket_data[i] == 0) // True if there is not a pointer already in that position -> empty
        {
            hashtable[s_hash].bucket_data[i] = s_data;
            return 1;
        }
        else
            continue; // else just start looping the next position
    }
    return 0; // all position is occupied
}

// search the hashtbl with hash key, dep, and id, return data entry (0 if not found)
static Data *search_mapping(int s_hash, int dep, int ID)
{
    Data *d;
    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
        if (hashtable[s_hash].bucket_data[i])
        {
            d = hashtable[s_hash].bucket_data[i];
            if (d->ID == ID && d->dep == dep)
            {
                return d;
            }
        }
    }
    return 0;
}

int delete_mapping(int r_hash, Data *r_data)
{
    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
        if (hashtable[r_hash].bucket_data[i] == r_data) // True if there is the exact pointer already in that position
        {
            hashtable[r_hash].bucket_data[i] = 0; // remove data in that position -> empty
            return 1;
        }
        else
            continue; // else just start looping the next position
    }
    return 0; // all position is checked but not found
}

static Data *dget(int dep, int id) // get data from department and id and put it to the hashtable without change its size`
{
    Data *d;
    int s_hash = hash(dep, id);
    d = search_mapping(s_hash, dep, id);
    if (d) // 1. the provided department and ID are alreacy in hashtbl
    {
        d->secret++;
    }
    else // 2. not already in the hashtbl
    {
        int min_secret = 0x8FFF;
        for (int i = 0; i < DATA_SIZE; ++i)
        {
            if (glob_data[i].secret < min_secret)
            {
                d = &(glob_data[i]);
                min_secret = d->secret;
            }
        }

        if (!d)
        {
            return 0;
        }

        int doner_hash = hash(d->dep, d->ID);
        delete_mapping(doner_hash, d);

        d->dep = dep;
        d->ID = id;
        d->secret++;

        if (!set_mapping(s_hash, d))
        {
            return 0;
        }
    }

    return d;
}

void init(void)
{
    for (int i = 0; i < DATA_SIZE; ++i)
    {
        glob_data[i].ID = i;
        glob_data[i].secret = 314;
        glob_data[i].dep = i * 17;
    }
    printf("glob_data[1] = %d\n", glob_data[1].ID);
    for (int i = 0; i < NBUCKETS; ++i)
    {
        for (int j = 0; j < BUCKET_SIZE; ++j)
        {
            hashtable[i].bucket_data[j] = 0;
        }
    }
}

int main(void)
{
    init();
    Data *d = dget(13, 13 * 17);
    if (d)
    {
        printf("Data.secret: %d\n", d->secret);
    }
    exit(0);
}