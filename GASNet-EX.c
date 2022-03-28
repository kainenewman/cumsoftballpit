#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <strings.h>

#define TEAM_DIR_SIZE 1021
#define gasneti_malloc(SZ) malloc(SZ)

#define TABLE_INIT_SIZE 10

typedef struct gasnete_table_item
{
	uint32_t key;
	void * data;
} gasnete_table_item_t;

typedef struct gasnete_table
{
	gasnete_table_item_t *slots;
	uint32_t size;
	uint32_t num;
} gasnete_table_t;

typedef struct gasnete_hashtable
{
  gasnete_table_t ** buckets;
  uint32_t size; /*< hash table size (# of buckets) */
  uint32_t num;  /*< number of elements in the hash table */
} gasnete_hashtable_t;





struct test_vectors {
	char *dst;
	char *src;
	uint32_t hash;	
};

typedef struct test_vectors *test_vectors_ptr;


typedef struct test_vectors *test_vectors_search;



typedef struct test_vectors *test_vectors_search_test;

gasnete_table_t * gasnete_table_create(uint32_t size)
{
  gasnete_table_t * table;

  /* gasneti_assert(size > 0); */
  table = (gasnete_table_t *)gasneti_malloc(sizeof(gasnete_table_t));

  table->slots = (gasnete_table_item_t *)gasneti_malloc(sizeof(gasnete_table_item_t)*size);

  table->size = size;
  table->num = 0;

  return table;
}

gasnete_hashtable_t * gasnete_hashtable_create(uint32_t size)
{
  gasnete_hashtable_t * ht;
  uint32_t i;

  /* gasneti_assert(size > 0); */
  ht = (gasnete_hashtable_t *)gasneti_malloc(sizeof(gasnete_hashtable_t));
  ht->buckets = (gasnete_table_t **)gasneti_malloc(sizeof(gasnete_table_t *)*size);
  ht->size = size;
  ht->num = 0;

  for (i=0; i<size; i++) {
    ht->buckets[i] = gasnete_table_create(TABLE_INIT_SIZE);
    /* gasneti_assert(ht->buckets[i] != NULL); */
  }
 
  return ht;
}

void gasnete_table_copy(const gasnete_table_t * const src, gasnete_table_t * const dst)
{
  uint32_t i;
  gasnete_table_item_t * src_slots, * dst_slots;

  /* gasneti_assert(dst->size >= src->num); */

  src_slots = src->slots;
  dst_slots = dst->slots;
  for (i=0; i<src->num; i++)
    dst_slots[i] = src_slots[i];

  dst->num = src->num;
}

uint32_t gasnete_hashtable_hash(gasnete_hashtable_t * ht, uint32_t key)
{
  return (key % ht->size);
}

uint32_t gasnete_table_insert(gasnete_table_t * const table, gasnete_table_item_t item)
{
  if (table->num >= table->size)
    return 1; /* insertion failed because the table is full */

  /* added the item to the end of the table */
  table->slots[table->num] = item;
  table->num++;

  return 0; /* success */
}

uint32_t gasnete_hashtable_insert(gasnete_hashtable_t * ht, uint32_t key, void * data)
{
  gasnete_table_t * table;
  gasnete_table_item_t item;
  uint32_t i;

  /* gasneti_assert(ht != NULL); */

  item.key = key;
  item.data = data;

  i = gasnete_hashtable_hash(ht, key);
  table = ht->buckets[i];
  /* gasneti_assert (table != NULL); */

  /* double the size of the table if the table is full */
  if (table->num == table->size) {
    gasnete_table_t * new_table;
    new_table = gasnete_table_create(table->size*2);
    /* gasneti_assert(new_table != NULL); */
    gasnete_table_copy(table, new_table);
    ht->buckets[i] = new_table;
    table = new_table;
  }

  ht->num++;
  return gasnete_table_insert(table, item);
}

gasnete_table_item_t * gasnete_table_search(const gasnete_table_t * const table, uint32_t key)
{
  uint32_t i;
  gasnete_table_item_t * slots;

  /* gasneti_assert(table != NULL); */
  slots = table->slots;
  for (i=0; i<table->num; i++)
    if (key == slots[i].key)
      return &slots[i];

  return NULL; /* item with key is not found in the table */
}


uint32_t gasnete_hashtable_search(gasnete_hashtable_t * ht, uint32_t key, void ** data)
{
  gasnete_table_t * table;
  gasnete_table_item_t * item;

  /*  gasneti_assert(ht != NULL); */

  table = ht->buckets[gasnete_hashtable_hash(ht, key)];
  /* gasneti_assert (table != NULL); */

  item = gasnete_table_search(table, key);
  if (item == NULL)
    return 1; /* cannot find the item with key */

  if (data != NULL)
    *data = item->data;

  return 0; /* success */
}



int main()
{
test_vectors_ptr gasnet_team_handle_t;
test_vectors_search gasnet_team_handle_search;

test_vectors_search_test gasnet_team_handle_search_test;
gasnet_team_handle_t = malloc(sizeof(struct test_vectors));

gasnet_team_handle_t->src= "0.0.0.0";
gasnet_team_handle_t->dst= "0.0.0.0";
gasnet_team_handle_t->hash = ;
static  gasnete_hashtable_t *team_dir = NULL;

if (team_dir == NULL) {
    team_dir = gasnete_hashtable_create(TEAM_DIR_SIZE);
   /* gasneti_assert(team_dir != NULL);*/
  }
 gasnete_hashtable_insert(team_dir, gasnet_team_handle_t->hash , gasnet_team_handle_t); 
 
 if ( ! gasnete_hashtable_search(team_dir, gasnet_team_handle_t->hash , (void **)&gasnet_team_handle_search))
 {
   printf("found\n");
  }
  else
  {
    printf("not found\n");
	}
	
  printf("%s  %s   %x",gasnet_team_handle_search->src, gasnet_team_handle_search->dst , gasnet_team_handle_search->hash);
  return 0;
}  
