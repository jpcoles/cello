#include <stdio.h>

#include "prioq.h"

pq_node_t *pq_merge(pq_node_t *queue,pq_node_t *p)
{
    pq_node_t *q,*temp,*parent,*left,*right;
    int ld,rd;
	
    if (p == NULL) return queue;

    if (queue) 
    {
        if (queue->key < p->key) 
        {
            temp = queue;
            queue = p;
            p = temp;
        }
        queue->parent = NULL;
        parent = queue;
        q = queue->right;
        while (q) 
        {
            if (q->key < p->key) 
            {
                temp = q;
                q = p;
                p = temp;
                q->parent = parent;
            }
            parent->right = q;
            parent = q;
            q = parent->right;
        }

        parent->right = p;
        p->parent = parent;
        p = parent;
        while (p) 
        {
            left = p->left;
            right = p->right;
            if (left) 
                ld = left->dist;
            else 
                ld = 0;

            rd = right->dist;
            if (ld < rd) 
            {
                p->left = right;
                p->right = left;
                p->dist = ld + 1;
            }
            else 
            {
                p->dist = rd + 1;
            }
            p = p->parent;
        }
    }
    else 
    {
        queue = p;
        p->parent = NULL;
    }

    return queue;
}


pq_node_t *pq_dequeue(pq_node_t **queue)
{
    pq_node_t *p = *queue;

    if (p != NULL)
        *queue = pq_merge(p->left, (*queue)->right);

    return p;
}


void pq_showqueue(pq_node_t *q,int lvl)
{
    int i;
	
    for (i=0;i<lvl;++i) printf("  ");
    if (q == NULL) 
    {
        printf("---\n");
    }
    else 
    {
        printf("%4f dist:%2d\n",q->key,q->dist);
        pq_showqueue(q->right,lvl+1);
        pq_showqueue(q->left,lvl+1);
    }
}

