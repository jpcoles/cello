//============================================================================
//                                can_interact2
//============================================================================
int can_interact2(uint32_t A, uint32_t B)
{
    const tree_node_t *nA = env.tree + A;
    const tree_node_t *nB = env.tree + B;

    const float R = DIST(nA->cm.x - nB->cm.x, 
                         nA->cm.y - nB->cm.y, 
                         nA->cm.z - nB->cm.z);

#if 0
    printf("can_interact(): NA(%i) * NB(%i) <? Npre(%i) = %i\n", NA, NB, Npre, NA * NB < Npre);
    printf("can_interact(): NA(%i) * NB(%i) <? Npost(%i) = %i\n", NA, NB, Npost, NA * NB < Npost);
#endif
    DBG(DBG_INTERACT)
        fprintf(stderr, "can_interact(): [%i %i]  R(%f) ?> (A_rmax(%f) + B_rmax(%f)) / OA(%f) = %f -> %i\n", 
            A,B, R, nA->rmax, nB->rmax, env.opening_angle, 
            (nA->rmax + nB->rmax) / env.opening_angle, R > (nA->rmax + nB->rmax) / env.opening_angle);

    //fprintf(stderr, "-- (%f %f) --\n", nA->rmax, nB->rmax);
    if (nA->size * nB->size <= 128) return 1;
    return R > ((nA->rmax + nB->rmax) / env.opening_angle);
}

//============================================================================
//                                can_interact3
//============================================================================
int can_interact3(uint32_t A, uint32_t B)
{
    const tree_node_t *nA = env.tree + A;
    const tree_node_t *nB = env.tree + B;

    const float R = DIST(nA->cm.x - nB->cm.x, 
                         nA->cm.y - nB->cm.y, 
                         nA->cm.z - nB->cm.z);

#if 0
    printf("can_interact(): NA(%i) * NB(%i) <? Npre(%i) = %i\n", NA, NB, Npre, NA * NB < Npre);
    printf("can_interact(): NA(%i) * NB(%i) <? Npost(%i) = %i\n", NA, NB, Npost, NA * NB < Npost);
#endif
    DBG(DBG_INTERACT)
        fprintf(stderr, "can_interact(): [%i %i]  R(%f) ?> (A_rmax(%f) + B_rmax(%f)) / OA(%f) = %f -> %i\n", 
            A,B, R, nA->rmax, nB->rmax, env.opening_angle, 
            (nA->rmax + nB->rmax) / env.opening_angle, R > (nA->rmax + nB->rmax) / env.opening_angle);

    //fprintf(stderr, "-- (%f %f) --\n", nA->rmax, nB->rmax);
    if (nA->size * nB->size <= 512) return 1;
    return R > ((nA->rmax + nB->rmax) / env.opening_angle);
}

//============================================================================
//                              interact_dehnen_modified
//============================================================================
int interact_dehnen_modified()
{
    uint32_t i,j;

    fprintf(stderr, "BEGIN interact_dehnen_modified()\n");

    stack_ptr = 0;

    tree_node_t *node = env.tree;

    PUSH(1);
    PUSH(1);

    while (!STACK_ISEMPTY())
    {
        uint32_t A = POP();
        uint32_t B = POP();

        if (can_interact(A,B))
        {
#if 0
            if (A > B) printf("%6i %6i  size:(%i %i)\n", A, B, node[A].size, node[B].size);
            else       printf("%6i %6i  size:(%i %i)\n", B, A, node[B].size, node[A].size);
#else
#if 0
            if (node[A].id < node[B].id) printf("%6i %6i  size:(%i %i)\n", node[A].id, node[B].id, node[A].size, node[B].size);
            else                         printf("%6i %6i  size:(%i %i)\n", node[B].id, node[A].id, node[B].size, node[A].size);
#endif
            //if (node[A].id < node[B].id) printf("%6i %6i\n", node[A].id, node[B].id);
            //else                         printf("%6i %6i\n", node[B].id, node[A].id);
            printf("%6i %6i\n", node[A].id, node[B].id);
#endif
        }
        else
        {
            if (A == B)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    for (j=i; node[B].children[j] != 0 && j < 8; j++)
                    {
                        PUSH(node[B].children[j]);
                        PUSH(node[A].children[i]);
                    }
                }

            }
            else if (node[A].rmax > node[B].rmax)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    PUSH(B);
                    PUSH(node[A].children[i]);
                }
            }
            else 
            {
                for (i=0; node[B].children[i] != 0 && i < 8; i++)
                {
                    PUSH(node[B].children[i]);
                    PUSH(A);
                }
            }
        }
    }

    fprintf(stderr, "END   interact_dehnen_modified()\n");

    return 0;
}

//============================================================================
//                              interact_dehnen_modified2
//============================================================================
int interact_dehnen_modified2()
{
    uint32_t i,j;
    uint32_t order=1;

    fprintf(stderr, "BEGIN interact_dehnen()\n");

    stack_ptr = 0;

    tree_node_t *node = env.tree;

    PUSH(1);
    PUSH(1);
    PUSH(order);
    order++;

    while (!STACK_ISEMPTY())
    {
        uint32_t o = POP();
        uint32_t A = POP();
        uint32_t B = POP();

        if (can_interact2(A,B))
        {
            //if (node[A].id < node[B].id) printf("%6i %6i %6i\n", node[A].id, node[B].id, o);
            //else                         printf("%6i %6i %6i\n", node[B].id, node[A].id, o);
            printf("%6i %6i %6i\n", node[A].id, node[B].id, o);
        }
        else
        {
            if (A == B)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    //PUSH(node[A].children[i]);
                    //PUSH(node[A].children[i]);

                    for (j=i; node[A].children[j] != 0 && j < 8; j++)
                    {
                        PUSH(node[A].children[j]);
                        PUSH(node[A].children[i]);
                        PUSH(order);
                        order++;
                    }
                }

            }
            else if (node[A].rmax > node[B].rmax)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    PUSH(B);
                    PUSH(node[A].children[i]);
                    PUSH(order);
                    order++;
                }
            }
            else 
            {
                for (i=0; node[B].children[i] != 0 && i < 8; i++)
                {
                    PUSH(node[B].children[i]);
                    PUSH(A);
                    PUSH(order);
                    order++;
                }
            }
        }
    }

    fprintf(stderr, "END   interact_dehnen()\n");

    return 0;
}

//============================================================================
//                              interact_dehnen_modified3
//============================================================================
int interact_dehnen_modified3()
{
    uint32_t i,j;
    uint32_t order=1;

    fprintf(stderr, "BEGIN interact_dehnen()\n");

    stack_ptr = 0;

    tree_node_t *node = env.tree;

    PUSH(1);
    PUSH(1);
    PUSH(order);
    order++;

    while (!STACK_ISEMPTY())
    {
        uint32_t o = POP();
        uint32_t A = POP();
        uint32_t B = POP();

        if (can_interact3(A,B))
        {
            //if (node[A].id < node[B].id) printf("%6i %6i %6i\n", node[A].id, node[B].id, o);
            //else                         printf("%6i %6i %6i\n", node[B].id, node[A].id, o);
            printf("%6i %6i %6i\n", node[A].id, node[B].id, o);
        }
        else
        {
            if (A == B)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    //PUSH(node[A].children[i]);
                    //PUSH(node[A].children[i]);

                    for (j=i; node[A].children[j] != 0 && j < 8; j++)
                    {
                        PUSH(node[A].children[j]);
                        PUSH(node[A].children[i]);
                        PUSH(order);
                        order++;
                    }
                }

            }
            else if (node[A].rmax > node[B].rmax)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    PUSH(B);
                    PUSH(node[A].children[i]);
                    PUSH(order);
                    order++;
                }
            }
            else 
            {
                for (i=0; node[B].children[i] != 0 && i < 8; i++)
                {
                    PUSH(node[B].children[i]);
                    PUSH(A);
                    PUSH(order);
                    order++;
                }
            }
        }
    }

    fprintf(stderr, "END   interact_dehnen()\n");

    return 0;
}

//============================================================================
//                               interact_queue
//============================================================================

int interact_queue()
{
    uint32_t i,j;

    //queue = REALLOC(queue, uint32_t, MAX_QUEUE_SIZE);
    //queue_size=0;
    queue_front = queue_back = queue_size = 0;

    tree_node_t *node = env.tree;

    ENQUEUE(1);
    ENQUEUE(1);

    while (!QUEUE_ISEMPTY())
    {
        uint32_t A = DEQUEUE();
        uint32_t B = DEQUEUE();

        if (can_interact(A,B))
        {
#if 0
            if (node[A].id < node[B].id) printf("%6i %6i  size:(%i %i)\n", node[A].id, node[B].id, node[A].size, node[B].size);
            else                         printf("%6i %6i  size:(%i %i)\n", node[B].id, node[A].id, node[B].size, node[A].size);
#endif
            if (node[A].id < node[B].id) printf("%6i %6i\n", node[A].id, node[B].id);
            else                         printf("%6i %6i\n", node[B].id, node[A].id);
        }
        else
        {
            if (A == B)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    for (j=i; node[B].children[j] != 0 && j < 8; j++)
                    {
                        ENQUEUE(node[A].children[i]);
                        ENQUEUE(node[B].children[j]);
                    }
                }

            }
            else if (node[A].rmax > node[B].rmax)
            {
                for (i=0; node[A].children[i] != 0 && i < 8; i++)
                {
                    ENQUEUE(node[A].children[i]);
                    ENQUEUE(B);
                }
            }
            else 
            {
                for (i=0; node[B].children[i] != 0 && i < 8; i++)
                {
                    ENQUEUE(A);
                    ENQUEUE(node[B].children[i]);
                }
            }
        }
    }

    return 0;
}

//============================================================================
//                               interact_prioq
//============================================================================
static inline void PQ_ENQUEUE(uint32_t A, uint32_t B)
{
    //static float key = 0.0;

    pq_node_t *n = MALLOC(pq_node_t, 1);
    n->A = A;
    n->B = B;
    //n->key = fmax(env.tree[A].rmax, env.tree[B].rmax) / fmin(env.tree[A].rmax, env.tree[B].rmax);
    //n->key = fmax(env.tree[A].rmax, env.tree[B].rmax) / fmin(env.tree[A].rmax, env.tree[B].rmax);
    n->key = fmax(env.tree[A].rmax, env.tree[B].rmax);

    //static float key = 0.0; n->key = key++; // -> Dehnen order
    //static float key = 0.0; n->key = key--; // -> LIFO order
    //static double key = 0.0; n->key = key; key += ((double)rand())/RAND_MAX - 0.5; // -> Random order
    //fprintf(stderr, "key = %f\n", key);
    //n->key = 0.0F; ; // -> ? order

    n->dist = 1;
    n->left = n->right = NULL;
    pqueue = pq_merge(pqueue, n);
}

static inline void PQ_DEQUEUE(uint32_t *A, uint32_t *B)
{
    pq_node_t *n = pq_dequeue(&pqueue);
    *A = n->A;
    *B = n->B;
    FREE(n);
}

static inline uint32_t PQ_ISEMPTY()
{
    return pqueue == NULL;
}

int interact_prioq()
{
    uint32_t A,B;
    uint32_t a,b;
    uint32_t child_A, child_B;

    tree_node_t *node = env.tree;

    fprintf(stderr, "BEGIN interact_prioq()\n");

    PQ_ENQUEUE(1,1);

    while (!PQ_ISEMPTY())
    {
        PQ_DEQUEUE(&A,&B);

        if (can_interact(A,B))
        {
#if 0
            if (A > B) printf("%6i %6i  size:(%i %i)\n", A, B, node[A].size, node[B].size);
            else       printf("%6i %6i  size:(%i %i)\n", B, A, node[B].size, node[A].size);
#else
            assert(node[A].id != 0);
            assert(node[B].id != 0);
#if 0
            if (node[A].id < node[B].id) printf("%6i %6i  size:(%i %i)\n", node[A].id, node[B].id, node[A].size, node[B].size);
            else                         printf("%6i %6i  size:(%i %i)\n", node[B].id, node[A].id, node[B].size, node[A].size);
#endif
            if (node[A].id < node[B].id) printf("%6i %6i\n", node[A].id, node[B].id);
            else                         printf("%6i %6i\n", node[B].id, node[A].id);
#endif
        }
        else
        {
            if (A == B)
            {
                forall_tree_node_child_pairs(A, a,b, child_A,child_B)
                    PQ_ENQUEUE(child_A, child_B);
            }
            else if (node[A].rmax > node[B].rmax)
            {
                forall_tree_node_children(A, a, child_A)
                    PQ_ENQUEUE(child_A, B);
            }
            else 
            {
                forall_tree_node_children(B, b, child_B)
                    PQ_ENQUEUE(A, child_B);
            }
        }
    }

    fprintf(stderr, "END   interact_prioq()\n");
    return 0;
}


//============================================================================
//                               interact_prioq2
//============================================================================
static inline void PQ2_ENQUEUE(uint32_t A, uint32_t B)
{
    //static float key = 0.0;

    pq_node_t *n = MALLOC(pq_node_t, 1);
    n->A = A;
    n->B = B;
    //n->key = fmax(env.tree[A].rmax, env.tree[B].rmax) / fmin(env.tree[A].rmax, env.tree[B].rmax);
    //n->key = fmax(env.tree[A].rmax, env.tree[B].rmax) / fmin(env.tree[A].rmax, env.tree[B].rmax);
    n->key = A; //fmax(env.tree[A].rmax, env.tree[B].rmax);

    //static float key = 0.0; n->key = key++; // -> Dehnen order
    //static float key = 0.0; n->key = key--; // -> LIFO order
    //static double key = 0.0; n->key = key; key += ((double)rand())/RAND_MAX - 0.5; // -> Random order
    //fprintf(stderr, "key = %f\n", key);
    //n->key = 0.0F; ; // -> ? order

    n->dist = 1;
    n->left = n->right = NULL;
    pqueue = pq_merge(pqueue, n);
}

static inline void PQ2_DEQUEUE(uint32_t *A, uint32_t *B)
{
    pq_node_t *n = pq_dequeue(&pqueue);
    *A = n->A;
    *B = n->B;
    FREE(n);
}

static inline uint32_t PQ2_ISEMPTY()
{
    return pqueue == NULL;
}

int interact_prioq2()
{
    uint32_t A,B;
    uint32_t a,b;
    uint32_t child_A, child_B;

    tree_node_t *node = env.tree;

    fprintf(stderr, "BEGIN interact_prioq2()\n");

    PQ2_ENQUEUE(1,1);

    while (!PQ2_ISEMPTY())
    {
        PQ2_DEQUEUE(&A,&B);

        if (can_interact(A,B))
        {
#if 0
            if (A > B) printf("%6i %6i  size:(%i %i)\n", A, B, node[A].size, node[B].size);
            else       printf("%6i %6i  size:(%i %i)\n", B, A, node[B].size, node[A].size);
#else
            assert(node[A].id != 0);
            assert(node[B].id != 0);
#if 0
            if (node[A].id < node[B].id) printf("%6i %6i  size:(%i %i)\n", node[A].id, node[B].id, node[A].size, node[B].size);
            else                         printf("%6i %6i  size:(%i %i)\n", node[B].id, node[A].id, node[B].size, node[A].size);
#endif
            if (node[A].id < node[B].id) printf("%6i %6i\n", node[A].id, node[B].id);
            else                         printf("%6i %6i\n", node[B].id, node[A].id);
#endif
        }
        else
        {
            if (A == B)
            {
                forall_tree_node_child_pairs(A, a,b, child_A,child_B)
                    PQ2_ENQUEUE(child_A, child_B);
            }
            else if (node[A].rmax > node[B].rmax)
            {
                forall_tree_node_children(A, a, child_A)
                    PQ2_ENQUEUE(child_A, B);
            }
            else 
            {
                forall_tree_node_children(B, b, child_B)
                    PQ2_ENQUEUE(A, child_B);
            }
        }
    }

    fprintf(stderr, "END   interact_prioq2()\n");
    return 0;
}
