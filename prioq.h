#ifndef PRIOQ_H
#define PRIOQ_H

#include <stdint.h>

typedef struct prioq_node_s
{
    float key;
    struct prioq_node_s *left;
    struct prioq_node_s *right;
    struct prioq_node_s *parent;
    uint32_t dist;
    uint32_t A, B;
} pq_node_t;

pq_node_t *pq_merge(pq_node_t *queue,pq_node_t *p);
pq_node_t *pq_dequeue(pq_node_t **queue);
void pq_showqueue(pq_node_t *q,int lvl);

#endif

