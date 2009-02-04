#include <assert.h>
#include <stdlib.h> // drand48
#include <math.h> // sqrt, pow

#include "env.h"
#include "mem.h"
#include "tree.h"
#include "string.h"
#include "prioq.h"
#include "macros.h"


static particulate_t *ps = NULL;
static uint32_t *stack = NULL;
static uint32_t stack_ptr=0, stack_capacity=0;

static uint32_t *queue = NULL;
static uint32_t queue_front=0, queue_back=0, queue_capacity=0, queue_size=0;

static pq_node_t *pqueue = NULL;

//----------------------------------------------------------------------------
// For use in build_oct_tree. Assumes the presence of variables 
// cur_node, next_node, and child_index. The last two are incremented by one.
//----------------------------------------------------------------------------
#define ADD_NODE(add_i, lower,upper, xm,xM, ym,yM, zm, zM) \
    if ((upper) >= (lower)) {\
        DBG(DBG_TREE) fprintf(stderr,"[%i]\tNew node %i  (l,u)=(%3i,%3i;%3i)  ", add_i, next_node, (lower),(upper),(upper)-(lower)+1); \
        DBG(DBG_TREE) fprintf(stderr, "min=(% f,% f,% f) max=(% f,% f,% f)\n", xm,ym,zm, xM,yM,zM); \
        if (next_node == env.max_tree_nodes+1) {\
            if (env.max_tree_nodes == 0) env.max_tree_nodes = 2048; else env.max_tree_nodes *= 2; \
            env.tree = REALLOC(env.tree, tree_node_t, env.max_tree_nodes+1); } \
        env.tree[next_node].l = (lower); env.tree[next_node].u = (upper); \
        env.tree[next_node].size = (upper)-(lower)+1; \
        env.tree[next_node].bnd.x.min = xm; \
        env.tree[next_node].bnd.x.max = xM; \
        env.tree[next_node].bnd.y.min = ym; \
        env.tree[next_node].bnd.y.max = yM; \
        env.tree[next_node].bnd.z.min = zm; \
        env.tree[next_node].bnd.z.max = zM; \
        env.tree[next_node].r.x = (xM+xm) / 2.0F; \
        env.tree[next_node].r.y = (yM+ym) / 2.0F; \
        env.tree[next_node].r.z = (zM+zm) / 2.0F; \
        env.tree[next_node].parent = cur_node; \
        env.tree[next_node].rmax = 0; \
        env.tree[cur_node].children[child_index++] = next_node; \
        PUSH(next_node); next_node++;  } 

#if 0
#define PUSH(l,u) do { stack[stack_ptr].min = l; stack[stack_ptr++].max = u; } while(0)
#define POP(l,u)  do { l = stack[stack_ptr].min; u = stack[stack_ptr--].max; } while(0)
#define PEEK(l,u) do { l = stack[stack_ptr].min; u = stack[stack_ptr].max; } while(0)
#define ISEMPTY() (stack_ptr == 0) 
#define ISFULL() (stack_ptr == MAX_STACK_SIZE) 
#endif

//----------------------------------------------------------------------------
// A stack.
//----------------------------------------------------------------------------
inline uint32_t STACK_ISEMPTY() 
{
    return stack_ptr == 0;
}

inline uint32_t STACK_ISFULL() 
{
    return stack_ptr == stack_capacity;
}

inline void PUSH(uint32_t n)
{
    if (STACK_ISFULL())
    {
        if (stack_capacity == 0) stack_capacity = 2048;
        else stack_capacity *= 2;
        stack = REALLOC(stack, uint32_t, stack_capacity);
    }
    stack[stack_ptr++] = n;
}

inline uint32_t POP()
{
    return stack[--stack_ptr];
}

inline uint32_t PEEK()
{
    return stack[stack_ptr];
}

//----------------------------------------------------------------------------
// A queue.
//----------------------------------------------------------------------------

inline uint32_t QUEUE_ISFULL()
{
    return queue_size == queue_capacity;
}

inline uint32_t QUEUE_ISEMPTY()
{
    return queue_size == 0;
}

inline void ENQUEUE(uint32_t n)
{
    if (QUEUE_ISFULL())
    {
        fprintf(stderr, "queue_capacity: %i front:%i back:%i empty:%i full:%i\n", queue_capacity, queue_front, queue_back, QUEUE_ISEMPTY(), QUEUE_ISFULL());
        uint32_t old_capacity = queue_capacity;
        if (queue_capacity == 0) queue_capacity = 2048; else queue_capacity *= 2;
        queue = REALLOC(queue, uint32_t, queue_capacity);
        assert(queue != NULL);

        const uint32_t len = old_capacity-queue_front;
        uint32_t new_front = queue_capacity-len;
        if (new_front == queue_capacity) new_front = 0; 
        memmove(queue+new_front, queue+queue_front, len * sizeof(uint32_t));
        queue_front = new_front;
    }
    queue_size++;
    queue[queue_back] = n; 
    if (++queue_back == queue_capacity) queue_back = 0;
}

inline uint32_t DEQUEUE()
{
    assert(!QUEUE_ISEMPTY());
    uint32_t ret = queue[queue_front];
    if (++queue_front == queue_capacity) queue_front = 0; 
    queue_size--;
    return ret;
}


//============================================================================
//                               build_oct_tree
//============================================================================
int build_oct_tree()
{
    uint32_t i;

    fprintf(stderr, "BEGIN Build oct-tree\n");

    ps = REALLOC(ps, particulate_t, env.n_particles + 1);

    //------------------------------------------------------------------------
    // We use the stack for storing the index of the next node to subdivide.
    // This naturally creates a depth-first like structure.
    //------------------------------------------------------------------------
    //stack = REALLOC(stack, uint32_t, stack_capacity);
    stack_ptr=0;

    float xmin, xmax, 
          ymin, ymax, 
          zmin, zmax;

    //------------------------------------------------------------------------
    // Copy the position info into the structure to sort.
    //------------------------------------------------------------------------
    myassert(env.n_particles > 0, "Trying to build tree with no particles.")

    ps[1].pid = id(1);
    ps[1].r.x = xmin = xmax = rx(1);
    ps[1].r.y = ymin = ymax = ry(1);
    ps[1].r.z = zmin = zmax = rz(1);

    for (i=2; i <= env.n_particles; i++)
    {
        ps[i].pid = id(i);
        ps[i].r.x = rx(i);
        ps[i].r.y = ry(i);
        ps[i].r.z = rz(i);

        if (rx(i) < xmin) xmin = rx(i); if (rx(i) > xmax) xmax = rx(i);
        if (ry(i) < ymin) ymin = ry(i); if (ry(i) > ymax) ymax = ry(i);
        if (rz(i) < zmin) zmin = rz(i); if (rz(i) > zmax) zmax = rz(i);
    }

    //------------------------------------------------------------------------
    // TODO: Should the root cube be centered on the simulation???
    //------------------------------------------------------------------------


    //------------------------------------------------------------------------
    // Setup the root node and push it on the stack to get the whole thing going.
    //------------------------------------------------------------------------
    uint32_t child_index = 0;
    uint32_t cur_node    = 0;
    uint32_t next_node   = 1;
    ADD_NODE(0, 1,env.n_particles, xmin,xmax, ymin,ymax, zmin,zmax);


    //------------------------------------------------------------------------
    // Start the "recursion"
    //------------------------------------------------------------------------
    while (!STACK_ISEMPTY())
    {
        cur_node = POP();

        particulate_t tmp;

#if 0
        float x_split = (node[cur_node].bnd.x.min + node[cur_node].bnd.x.max) / 2.0F;
        float y_split = (node[cur_node].bnd.y.min + node[cur_node].bnd.y.max) / 2.0F;
        float z_split = (node[cur_node].bnd.z.min + node[cur_node].bnd.z.max) / 2.0F;
#endif

        const float x_split = env.tree[cur_node].r.x;
        const float y_split = env.tree[cur_node].r.y;
        const float z_split = env.tree[cur_node].r.z;

        DBG(DBG_TREE) 
        {
            fprintf(stderr, "\ncur_node=%i  next_node=%i  ", cur_node, next_node);
            fprintf(stderr, "(x_split,y_split,z_split)=(%.2f, %.2f, %.2f) (stack_ptr=%i)\n",
                x_split, y_split, z_split, stack_ptr);
        }

        const uint32_t first = env.tree[cur_node].l;
        const uint32_t last  = env.tree[cur_node].u;

        assert(env.tree[cur_node].size == last-first+1);

        if (env.tree[cur_node].size <= 8) continue;

        //------------------------------------------------------------------------
        // Partition the current cube into eight pieces.
        //------------------------------------------------------------------------

        /* x split */
        uint32_t l0=first, u0 = last;
        PARTITION(ps, tmp, l0,u0, ps[l0].r.x < x_split, x_split <= ps[u0].r.x);
        DBG(DBG_TREE) fprintf(stderr, "P1: (%i %i) -> (%i %i)\n", first, last, l0, u0);

            /* y split */
            uint32_t l1=first, u1 = l0-1;
            PARTITION(ps, tmp, l1,u1, ps[l1].r.y < y_split, y_split <= ps[u1].r.y);
            DBG(DBG_TREE) fprintf(stderr, "\tP2: (%i %i) -> (%i %i)\n", first, u0, l1, u1);

            uint32_t l2=l0, u2 = last;
            PARTITION(ps, tmp, l2,u2, ps[l2].r.y < y_split, y_split <= ps[u2].r.y);
            DBG(DBG_TREE) fprintf(stderr, "\tP3: (%i %i) -> (%i %i)\n", l0, last, l2, u2);

                /* z split */
                uint32_t l3=first, u3 = l1-1;
                PARTITION(ps, tmp, l3,u3, ps[l3].r.z < z_split, z_split <= ps[u3].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP4: (%i %i) -> (%i %i)\n", first, u1, l3, u3);

                uint32_t l4=l1, u4 = l0-1;
                PARTITION(ps, tmp, l4,u4, ps[l4].r.z < z_split, z_split <= ps[u4].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP5: (%i %i) -> (%i %i)\n", l1, u0, l4, u4);

                uint32_t l5=l0, u5 = l2-1;
                PARTITION(ps, tmp, l5,u5, ps[l5].r.z < z_split, z_split <= ps[u5].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP6: (%i %i) -> (%i %i)\n", l0, u2, l5, u5);

                uint32_t l6=l2, u6 = last;
                PARTITION(ps, tmp, l6,u6, ps[l6].r.z < z_split, z_split <= ps[u6].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP7: (%i %i) -> (%i %i)\n", l2, last, l6, u6);


        DBG(DBG_TREE) fprintf(stderr, "%i %i %i %i %i %i %i %i %i\n", first,l3,l1,l4,l0,l5,l2,l6,last);


        //------------------------------------------------------------------------
        // For each new node, add it to the tree (if not empty) and push it
        // on the stack.
        //------------------------------------------------------------------------
        child_index=0;

        ADD_NODE(1,first,l3-1, env.tree[cur_node].bnd.x.min, x_split,
                               env.tree[cur_node].bnd.y.min, y_split,
                               env.tree[cur_node].bnd.z.min, z_split);

        ADD_NODE(2,l3, l1-1, env.tree[cur_node].bnd.x.min, x_split,
                             env.tree[cur_node].bnd.y.min, y_split,
                             z_split, env.tree[cur_node].bnd.z.max);

        ADD_NODE(3,l1,l4-1, env.tree[cur_node].bnd.x.min, x_split,
                            y_split, env.tree[cur_node].bnd.y.max,
                            env.tree[cur_node].bnd.z.min, z_split);

        ADD_NODE(4,l4,l0-1, env.tree[cur_node].bnd.x.min, x_split,
                            y_split, env.tree[cur_node].bnd.y.max,
                            z_split, env.tree[cur_node].bnd.z.max);

        ADD_NODE(5,l0,l5-1, x_split, env.tree[cur_node].bnd.x.max,
                            env.tree[cur_node].bnd.y.min, y_split,
                            env.tree[cur_node].bnd.z.min, z_split);

        ADD_NODE(6,l5,l2-1, x_split, env.tree[cur_node].bnd.x.max,
                            env.tree[cur_node].bnd.y.min, y_split,
                            z_split, env.tree[cur_node].bnd.z.max);

        ADD_NODE(7,l2,l6-1, x_split, env.tree[cur_node].bnd.x.max,
                            y_split, env.tree[cur_node].bnd.y.max,
                            env.tree[cur_node].bnd.z.min, z_split);

        ADD_NODE(8,l6,last, x_split, env.tree[cur_node].bnd.x.max,
                            y_split, env.tree[cur_node].bnd.y.max,
                            z_split, env.tree[cur_node].bnd.z.max);
    }
    

    env.n_tree_nodes = next_node-1;

    //------------------------------------------------------------------------
    // Reorder the real particle array to match the sorted list
    //------------------------------------------------------------------------
    particle_t *new_ps = MALLOC(particle_t, env.n_particles+1);

    for (i=1; i <= env.n_particles; i++)
        new_ps[i] = env.ps[ ps[i].pid ];

    FREE(env.ps);
    env.ps = new_ps;

    fprintf(stderr, "END   Build oct-tree\n");

    return 0;
}

//============================================================================
//                                 fill_tree
//============================================================================
int fill_tree()
{
    uint32_t i,j;

    if (env.tree == NULL) 
    {
        printf("Oct tree has not been created.\n");
        return 1;
    }

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    stack_ptr=0;
    tree_node_t *node = env.tree;

    for (i=1; i <= env.n_tree_nodes; i++)
    {
        //--------------------------------------------------------------------
        // Calculate the CM of each node
        //--------------------------------------------------------------------
        float cx=0, cy=0, cz=0, m=0;
        for (j=node[i].l; j <= node[i].u; j++)
        {
            cx += M(j) * rx(j);
            cy += M(j) * ry(j);
            cz += M(j) * rz(j);
             m += M(j);
        }
        cx = node[i].cm.x = cx / m;
        cy = node[i].cm.y = cy / m;
        cz = node[i].cm.z = cz / m;
             node[i].M    = m;

        //--------------------------------------------------------------------
        // Find the most distant particle in each node from its CM 
        //--------------------------------------------------------------------
        float rmax2=0;
        for (j=node[i].l; j <= node[i].u; j++)
        {
#if 1
#if 0
            fprintf(stderr, "node[%i]::particle[%i]\n", i, j);
            fprintf(stderr, "min=(% f,% f,% f) max=(% f,% f,% f)\n", node[i].bnd.x.min,node[i].bnd.y.min,node[i].bnd.z.min, node[i].bnd.x.max,node[i].bnd.y.max,node[i].bnd.z.max);
            fprintf(stderr, "xyz=(% f,% f,% f)\n", rx(j),ry(j),rz(j));
#endif
            assert(node[i].bnd.x.min <= rx(j) && rx(j) <= node[i].bnd.x.max);
            assert(node[i].bnd.y.min <= ry(j) && ry(j) <= node[i].bnd.y.max);
            assert(node[i].bnd.z.min <= rz(j) && rz(j) <= node[i].bnd.z.max);
#else
            assert(node[i].bnd.x.min <= ps[j].r.x && ps[j].r.x <= node[i].bnd.x.max);
            assert(node[i].bnd.y.min <= ps[j].r.y && ps[j].r.y <= node[i].bnd.y.max);
            assert(node[i].bnd.z.min <= ps[j].r.z && ps[j].r.z <= node[i].bnd.z.max);
#endif

            const float r2 = DIST2(rx(j)-cx, ry(j)-cy, rz(j)-cz);
            if (r2 > rmax2) rmax2 = r2;
        }

        //--------------------------------------------------------------------
        // rmax is the distance between the CM and the furthest particle.
        //--------------------------------------------------------------------
        node[i].rmax = sqrt(rmax2);

#if 0
        if (i>1)
        {
            uint32_t parent = node[i].parent;
            node[i].rmax += DIST(cx - node[parent].r.x, 
                                 cy - node[parent].r.y, 
                                 cz - node[parent].r.z);
        }
#endif
    }

#if 1
    //------------------------------------------------------------------------
    // Fill in the node id's in breadth-first order.
    //------------------------------------------------------------------------
    queue_front = queue_back = queue_size = 0;
    ENQUEUE(1);

    uint32_t id = 1;
    uint32_t a, child_A;
    while (!QUEUE_ISEMPTY())
    {
        uint32_t A = DEQUEUE();
        env.tree[A].id = id++;

        forall_tree_node_children(A, a, child_A)
            ENQUEUE(child_A);
    }


    // Sanity checks for now
    // (1)
    if (id != env.n_tree_nodes+1)
    {
        fprintf(stderr, "Wrong number of id's: %i (should be %i)\n", id, env.n_tree_nodes+1);
        exit(1);
    }

    // (2)
    for (i=1; i <= env.n_tree_nodes; i++)
    {
        if (env.tree[i].id == 0)
        {
            fprintf(stderr, "HOW? %i %i\n", i, env.tree[i].id);
            exit(2);
        }

    }

    //----------------------------------------------------------------------------
    // Fill in the node id's in breadth-first order.
    //----------------------------------------------------------------------------
    queue_front = queue_back = queue_size = 0;
    ENQUEUE(1);

    while (!QUEUE_ISEMPTY())
    {
        uint32_t A = DEQUEUE();
        //fprintf(stderr, "ID %i.%i  node ID %i.%i  --  %i\n", env.tree[env.tree[A].parent].id, env.tree[A].id, env.tree[A].parent, A, env.tree[A].size);

        forall_tree_node_children(A, a, child_A)
            ENQUEUE(child_A);
    }
#endif

#if 0
    PUSH(1);

    tree_node_t *node = env.tree;

    while (!STACK_ISEMPTY())
    {
        uint32_t cur_node = POP();

        if (cur_node == 0)
        {
            cur_node = POP();

#if 1
            float cx=0, cy=0, cz=0, m=0;
            for (i=0; node[cur_node].children[i] != 0 && i < 8; i++)
            {
                uint32_t child_id = node[cur_node].children[i];

                cx += node[child_id].M * node[child_id].cm.x;
                cy += node[child_id].M * node[child_id].cm.y;
                cz += node[child_id].M * node[child_id].cm.z;
                 m += node[child_id].M;
            }

            node[cur_node].cm.x = cx / m;
            node[cur_node].cm.y = cy / m;
            node[cur_node].cm.z = cz / m;
            node[cur_node].M = m;

            float rmax2=0;
            for (j=node[cur_node].l; j <= node[cur_node].u; j++)
            {
                const float r2 = pow(rx(j)-cx,2) + pow(ry(j)-cy,2) + pow(rz(j)-cz,2);
                if (r2 > rmax2) rmax2 = r2;
            }

            uint32_t parent = node[cur_node].parent;
            node[cur_node].rmax = 
                sqrt(rmax2) 
                +
                sqrt(pow(node[cur_node].cm.x-node[parent].r.x, 2) 
                   + pow(node[cur_node].cm.y-node[parent].r.y, 2) 
                   + pow(node[cur_node].cm.z-node[parent].r.z, 2));
#endif
        }
        else
        {
            if (node[cur_node].children[0] == 0)
            {
#if 1
                float cx=0, cy=0, cz=0, m=0;
                for (i=node[cur_node].l; i <= node[cur_node].u; i++)
                {
                    cx += M(i) * rx(i);
                    cy += M(i) * ry(i);
                    cz += M(i) * rz(i);
                     m += M(i);
                }
                node[cur_node].cm.x = cx / m;
                node[cur_node].cm.y = cy / m;
                node[cur_node].cm.z = cz / m;
                node[cur_node].M = m;

                float rmax2=0;
                for (j=node[cur_node].l; j <= node[cur_node].u; j++)
                {
                    const float r2 = pow(rx(j)-cx,2) + pow(ry(j)-cy,2) + pow(rz(j)-cz,2);
                    if (r2 > rmax2) rmax2 = r2;
                }

                uint32_t parent = node[cur_node].parent;
                node[cur_node].rmax = 
                    sqrt(rmax2) 
                    +
                    sqrt(pow(node[cur_node].cm.x-node[parent].r.x, 2) 
                       + pow(node[cur_node].cm.y-node[parent].r.y, 2) 
                       + pow(node[cur_node].cm.z-node[parent].r.z, 2));
#endif
            }
            else
            {
                PUSH(cur_node);
                PUSH(0);

                for (i=0; node[cur_node].children[i] != 0 && i < 8; i++)
                    PUSH(node[cur_node].children[i]);
            }
        }
    }
#endif

    return 0;
}

//============================================================================
//                               print_oct_tree
//============================================================================
int print_oct_tree()
{
    uint32_t i;

    if (env.tree == NULL) 
    {
        printf("Oct tree has not been created.\n");
        return 1;
    }

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    stack_ptr=0;

    PUSH(1);

    tree_node_t *node = env.tree;

    uint32_t node_count=0, leaf_count=0, p_count=0;

    while (!STACK_ISEMPTY())
    {
        uint32_t cur_node = POP();
        node_count++;

        printf("Node %i (", cur_node);
        if (node[cur_node].children[0] == 0)
        {
            printf("Leaf");
            leaf_count++;
            p_count += node[cur_node].u - node[cur_node].l + 1;
        }
        else
        {
            printf("Children:");
            for (i=0; node[cur_node].children[i] != 0 && i < 8; i++)
            {
                printf(" %i", node[cur_node].children[i]);
                PUSH(node[cur_node].children[i]);
            }
        }
        printf(")  ");
        printf("(lo up #)=(%4i %4i %4i)  ", node[cur_node].l, node[cur_node].u, node[cur_node].u-node[cur_node].l+1);
        //printf("(xm xM ym yM zm zM)=(%.2f %.2f %.2f %.2f %.2f %.2f)\n",
        printf("(xm ym zm  zM yM zM)=(% .2f % .2f % .2f  % .2f % .2f % .2f)  ",
            node[cur_node].bnd.x.min,
            node[cur_node].bnd.y.min,
            node[cur_node].bnd.z.min,
            node[cur_node].bnd.x.max,
            node[cur_node].bnd.y.max,
            node[cur_node].bnd.z.max);
        printf("CM=(% .2f % .2f % .2f)", node[cur_node].cm.x, node[cur_node].cm.y, node[cur_node].cm.z);
        printf("\n");
    }

    if (node_count != env.n_tree_nodes)
        printf("\nERROR: Number of nodes in tree (%i) does not match n_tree_nodes (%i)!\n\n", node_count, env.n_tree_nodes);
    
    printf("Nodes: %i   Leaves: %i   Particles: %i", node_count, leaf_count, p_count);
    if (p_count != env.n_particles)
        printf(" ** Why isn't this the total number of particle (%i)", env.n_particles);
    printf("\n");

    return 0;
}

//============================================================================
//                                can_interact
//============================================================================
int can_interact(uint32_t A, uint32_t B)
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
    //if (A==B && nA->size <= 8) return 1;
    return R > ((nA->rmax + nB->rmax) / env.opening_angle);
}

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
//                              interact_dehnen
//============================================================================
int interact_dehnen()
{
    uint32_t i,j;

    fprintf(stderr, "BEGIN interact_dehnen()\n");

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
            printf("%6i %6i\n", node[B].id, node[A].id);
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

    fprintf(stderr, "END   interact_dehnen()\n");

    return 0;
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
inline void PQ_ENQUEUE(uint32_t A, uint32_t B)
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

inline void PQ_DEQUEUE(uint32_t *A, uint32_t *B)
{
    pq_node_t *n = pq_dequeue(&pqueue);
    *A = n->A;
    *B = n->B;
    FREE(n);
}

inline uint32_t PQ_ISEMPTY()
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
inline void PQ2_ENQUEUE(uint32_t A, uint32_t B)
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

inline void PQ2_DEQUEUE(uint32_t *A, uint32_t *B)
{
    pq_node_t *n = pq_dequeue(&pqueue);
    *A = n->A;
    *B = n->B;
    FREE(n);
}

inline uint32_t PQ2_ISEMPTY()
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
