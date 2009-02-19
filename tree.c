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

static uint32_t *queue = NULL;
static uint32_t queue_front=0, queue_back=0, queue_capacity=0, queue_size=0;

static pq_node_t *pqueue = NULL;

//----------------------------------------------------------------------------
// For use in build_oct_tree. Assumes the presence of variables 
// cur_node, next_node, and child_index. The last two are incremented by one.
//----------------------------------------------------------------------------
#define ADD_NODE(add_i, lower,upper, xm,xM, ym,yM, zm, zM) \
    if ((upper) >= (lower)) {\
        DBG(DBG_TREE) fprintf(stderr,"[%i]\tNew node %i  (l,u)=(%3i,%3i;%3i)  ", \
            (int)add_i, (int)next_node, (int)(lower),(int)(upper),(int)((upper)-(lower)+1)); \
        DBG(DBG_TREE) fprintf(stderr, "min=(% f,% f,% f) max=(% f,% f,% f)\n", xm,ym,zm, xM,yM,zM); \
        if (next_node == tree->allocd_nodes+1) {\
            if (tree->allocd_nodes == 0) tree->allocd_nodes = 2048; else tree->allocd_nodes *= 2; \
            root = tree->root = REALLOC(tree->root, tree_node_t, tree->allocd_nodes+1); } \
        root[next_node].l = (lower); root[next_node].u = (upper); \
        root[next_node].size = (upper)-(lower)+1; \
        root[next_node].bnd.x.min = xm; \
        root[next_node].bnd.x.max = xM; \
        root[next_node].bnd.y.min = ym; \
        root[next_node].bnd.y.max = yM; \
        root[next_node].bnd.z.min = zm; \
        root[next_node].bnd.z.max = zM; \
        root[next_node].r.x = (xM+xm) / 2.0F; \
        root[next_node].r.y = (yM+ym) / 2.0F; \
        root[next_node].r.z = (zM+zm) / 2.0F; \
        root[next_node].parent = cur_node; \
        root[next_node].rmax = 0; \
        root[cur_node].children[child_index++] = next_node; \
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
#define MAKE_STACK(T) \
static T *stack = NULL; \
static uint64_t stack_ptr, stack_capacity; \
stack_ptr=0; stack_capacity=0;\
inline uint32_t STACK_ISEMPTY() { return stack_ptr == 0; } \
inline uint32_t STACK_ISFULL()  { return stack_ptr == stack_capacity; } \
inline void PUSH(T n) { \
    if (STACK_ISFULL()) \
    { \
        if (stack_capacity == 0) stack_capacity = 2048; \
        else stack_capacity *= 2; \
        stack = REALLOC(stack, T, stack_capacity); \
    } \
    stack[stack_ptr++] = n; \
} \
inline T POP() { return stack[--stack_ptr]; } \
inline T PEEK() { return stack[stack_ptr]; }

//----------------------------------------------------------------------------
// A queue.
//----------------------------------------------------------------------------

static inline uint32_t QUEUE_ISFULL()
{
    return queue_size == queue_capacity;
}

static inline uint32_t QUEUE_ISEMPTY()
{
    return queue_size == 0;
}

static inline void ENQUEUE(uint32_t n)
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

static inline uint32_t DEQUEUE()
{
    assert(!QUEUE_ISEMPTY());
    uint32_t ret = queue[queue_front];
    if (++queue_front == queue_capacity) queue_front = 0; 
    queue_size--;
    return ret;
}

static int _psort_cmp(const void *a0, const void *b0)
{
    particle_t *a = (particle_t *)a0;
    particle_t *b = (particle_t *)b0;
    if (a->sid > b->sid) return +1;
    if (a->sid < b->sid) return -1;
    myassert(0, "Two particles have the same sid!");
}

//============================================================================
//                               build_oct_tree
//============================================================================
int build_oct_tree(tree_t *tree)
{
    uint32_t i;

    ANNOUNCE_BEGIN(__FUNCTION__);


    //------------------------------------------------------------------------
    // We use the stack for storing the index of the next node to subdivide.
    // This naturally creates a depth-first like structure.
    //------------------------------------------------------------------------
    //stack = REALLOC(stack, uint32_t, stack_capacity);
    //stack_ptr=0;

    float xmin, xmax, 
          ymin, ymax, 
          zmin, zmax;

    Pid_t first = tree->l, 
          last  = tree->u, 
          N     = 1 + last - first;

    tree_node_t *root = tree->root;

    //------------------------------------------------------------------------
    // Copy the position info into the structure to sort.
    //------------------------------------------------------------------------
    myassert(N > 0, "Trying to build tree with no particles.")

    ps = REALLOC(ps, particulate_t, N + 1);

    ps[first].pid = id(first);
    ps[first].r.x = xmin = xmax = rx(first);
    ps[first].r.y = ymin = ymax = ry(first);
    ps[first].r.z = zmin = zmax = rz(first);

    for (i=first+1; i <= last; i++)
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

    MAKE_STACK(cid_t)

    //------------------------------------------------------------------------
    // Setup the root node and push it on the stack to get the whole thing going.
    //------------------------------------------------------------------------
    uint32_t child_index = 0;
    uint32_t cur_node    = 0;
    uint32_t next_node   = 1;

    dist_t xR = (xmax-xmin)/2;
    dist_t yR = (ymax-ymin)/2;
    dist_t zR = (zmax-zmin)/2;
    dist_t  R = fmax(xR, fmax(yR, zR)) + 1e-4;

    dist_t xC = (xmax+xmin)/2;
    dist_t yC = (ymax+ymin)/2;
    dist_t zC = (zmax+zmin)/2;


    ADD_NODE(0, first,last, xC-R,xC+R, yC-R,yC+R, zC-R,zC+R);



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

        const float x_split = root[cur_node].r.x;
        const float y_split = root[cur_node].r.y;
        const float z_split = root[cur_node].r.z;

        DBG(DBG_TREE) 
        {
            fprintf(stderr, "\ncur_node=%i  next_node=%i  ", cur_node, next_node);
            fprintf(stderr, "(x_split,y_split,z_split)=(%.2f, %.2f, %.2f) (stack_ptr=%i)\n",
                x_split, y_split, z_split, (int)stack_ptr);
        }

        const cid_t L = root[cur_node].l;
        const cid_t U = root[cur_node].u;

        assert(root[cur_node].size == U-L+1);

        if (root[cur_node].size <= 8) continue;

        //------------------------------------------------------------------------
        // Partition the current cube into eight pieces.
        //------------------------------------------------------------------------

        /* x split */
        cid_t l0=L, u0 = U;
        PARTITION(ps, tmp, l0,u0, ps[l0].r.x < x_split, x_split <= ps[u0].r.x);
        DBG(DBG_TREE) fprintf(stderr, "P1: (%i %i) -> (%i %i)\n", L, U, l0, u0);

            /* y split */
            cid_t l1=L, u1 = l0-1;
            PARTITION(ps, tmp, l1,u1, ps[l1].r.y < y_split, y_split <= ps[u1].r.y);
            DBG(DBG_TREE) fprintf(stderr, "\tP2: (%i %i) -> (%i %i)\n", L, u0, l1, u1);

            cid_t l2=l0, u2 = U;
            PARTITION(ps, tmp, l2,u2, ps[l2].r.y < y_split, y_split <= ps[u2].r.y);
            DBG(DBG_TREE) fprintf(stderr, "\tP3: (%i %i) -> (%i %i)\n", l0, U, l2, u2);

                /* z split */
                cid_t l3=L, u3 = l1-1;
                PARTITION(ps, tmp, l3,u3, ps[l3].r.z < z_split, z_split <= ps[u3].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP4: (%i %i) -> (%i %i)\n", L, u1, l3, u3);

                cid_t l4=l1, u4 = l0-1;
                PARTITION(ps, tmp, l4,u4, ps[l4].r.z < z_split, z_split <= ps[u4].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP5: (%i %i) -> (%i %i)\n", l1, u0, l4, u4);

                cid_t l5=l0, u5 = l2-1;
                PARTITION(ps, tmp, l5,u5, ps[l5].r.z < z_split, z_split <= ps[u5].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP6: (%i %i) -> (%i %i)\n", l0, u2, l5, u5);

                cid_t l6=l2, u6 = U;
                PARTITION(ps, tmp, l6,u6, ps[l6].r.z < z_split, z_split <= ps[u6].r.z);
                DBG(DBG_TREE) fprintf(stderr, "\t\tP7: (%i %i) -> (%i %i)\n", l2, U, l6, u6);


        DBG(DBG_TREE) fprintf(stderr, "%i %i %i %i %i %i %i %i %i\n", L,l3,l1,l4,l0,l5,l2,l6,U);


        //------------------------------------------------------------------------
        // For each new node, add it to the tree (if not empty) and push it
        // on the stack.
        //------------------------------------------------------------------------
        child_index=0;

        ADD_NODE(1,L,l3-1, root[cur_node].bnd.x.min, x_split,
                           root[cur_node].bnd.y.min, y_split,
                           root[cur_node].bnd.z.min, z_split);

        ADD_NODE(2,l3, l1-1, root[cur_node].bnd.x.min, x_split,
                             root[cur_node].bnd.y.min, y_split,
                             z_split, root[cur_node].bnd.z.max);

        ADD_NODE(3,l1,l4-1, root[cur_node].bnd.x.min, x_split,
                            y_split, root[cur_node].bnd.y.max,
                            root[cur_node].bnd.z.min, z_split);

        ADD_NODE(4,l4,l0-1, root[cur_node].bnd.x.min, x_split,
                            y_split, root[cur_node].bnd.y.max,
                            z_split, root[cur_node].bnd.z.max);

        ADD_NODE(5,l0,l5-1, x_split, root[cur_node].bnd.x.max,
                            root[cur_node].bnd.y.min, y_split,
                            root[cur_node].bnd.z.min, z_split);

        ADD_NODE(6,l5,l2-1, x_split, root[cur_node].bnd.x.max,
                            root[cur_node].bnd.y.min, y_split,
                            z_split, root[cur_node].bnd.z.max);

        ADD_NODE(7,l2,l6-1, x_split, root[cur_node].bnd.x.max,
                            y_split, root[cur_node].bnd.y.max,
                            root[cur_node].bnd.z.min, z_split);

        ADD_NODE(8,l6,U, x_split, root[cur_node].bnd.x.max,
                         y_split, root[cur_node].bnd.y.max,
                         z_split, root[cur_node].bnd.z.max);
    }
    

    tree->n_nodes = next_node-1;

    //------------------------------------------------------------------------
    // Reorder the real particle array to match the sorted list
    //------------------------------------------------------------------------
    for (i=first; i <= last; i++)
        sid(ps[i].pid) = i;

    qsort(env.ps+first, N, sizeof(env.ps[0]), _psort_cmp);

    ANNOUNCE_END(__FUNCTION__);

    return 0;
}

//============================================================================
//                                 fill_tree
//============================================================================
int fill_tree(tree_t *tree)
{
    uint32_t i,j;

    ANNOUNCE_BEGIN(__FUNCTION__);

    if (tree == NULL) 
    {
        printf("Oct tree has not been created.\n");
        return 1;
    }

    MAKE_STACK(cid_t);

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    //stack_ptr=0;
    tree_node_t *node = tree->root;

    for (i=1; i <= tree->n_nodes; i++)
    {
        MOMR M;

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

#if 0
        if (i==1)
        {
            fprintf(stderr, "CELL 1: m=%f M(1)=%f l=%i u=%i\n", m, M(1), (uint32_t)node[i].l, (uint32_t)node[i].u);
        }
#endif

        momClearLocr(&node[i].L);
        momClearMomr(&node[i].M);
        for (j=node[i].l; j <= node[i].u; j++)
        {
            momMakeMomr(&M, M(j), rx(j)-cx, ry(j)-cy, rz(j)-cz);
            momAddMomr(&node[i].M, &M);
        }

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
            if (!(node[i].bnd.x.min <= rx(j) && rx(j) <= node[i].bnd.x.max))
                fprintf(stderr, "** %20.15f %20.15f %20.15f\n", node[i].bnd.x.min, rx(j), node[i].bnd.x.max);
            if (!(node[i].bnd.y.min <= ry(j) && ry(j) <= node[i].bnd.y.max))
                fprintf(stderr, "** %f %f %f\n", node[i].bnd.y.min, ry(j), node[i].bnd.y.max);
            if (!(node[i].bnd.z.min <= rz(j) && rz(j) <= node[i].bnd.z.max))
                fprintf(stderr, "** %f %f %f\n", node[i].bnd.z.min, rz(j), node[i].bnd.z.max);

            assert(node[i].bnd.x.min <= rx(j) && rx(j) <= node[i].bnd.x.max);
            assert(node[i].bnd.y.min <= ry(j) && ry(j) <= node[i].bnd.y.max);
            assert(node[i].bnd.z.min <= rz(j) && rz(j) <= node[i].bnd.z.max);
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

    cid_t id = 1;
    cid_t a, child_A;
    while (!QUEUE_ISEMPTY())
    {
        cid_t A = DEQUEUE();
        node[A].id = id++;

        forall_tree_node_children(A, a, child_A)
            ENQUEUE(child_A);
    }


    //----------------------------------------------------------------------------
    // Sanity checks for now
    //----------------------------------------------------------------------------
    // (1)
    if (id != tree->n_nodes+1)
    {
        fprintf(stderr, "Wrong number of id's: %i (should be %i)\n", id, tree->n_nodes+1);
        exit(1);
    }

    // (2)
    for (i=1; i <= tree->n_nodes; i++)
    {
        if (node[i].id == 0)
        {
            fprintf(stderr, "HOW? %i %i\n", i, node[i].id);
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
        cid_t A = DEQUEUE();
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

    ANNOUNCE_END(__FUNCTION__);

    return 0;
}

//============================================================================
//                               print_oct_tree
//============================================================================
int print_oct_tree(tree_t *tree)
{
    uint32_t i;

    if (tree == NULL) 
    {
        printf("Oct tree has not been created.\n");
        return 1;
    }

    MAKE_STACK(cid_t)

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    //stack_ptr=0;

    PUSH(1);

    tree_node_t *node = tree->root;
    cid_t N = tree->u - tree->l + 1;

    uint32_t node_count=0, leaf_count=0, p_count=0;

    while (!STACK_ISEMPTY())
    {
        cid_t cur_node = POP();

        if ((node_count % 40) == 0) 
        {
            printf("%-5s %-4s  %4s %4s %4s %5s %5s %5s  %5s %5s %5s  %5s %5s %5s %5s\n",
                "Node", "Type", "lowr", "uppr", "#", "xm","ym","zm", "xM","yM","zM",
                "CMx", "CMy", "CMz", "M");
            printf(BAR1 "------------\n");
        }

        node_count++;

        printf("%5i ", cur_node);
        if (node[cur_node].children[0] == 0)
        {
            printf("%-4s", " L");
            leaf_count++;
            p_count += node[cur_node].u - node[cur_node].l + 1;
        }
        else
            printf("%-4s", "N");

        printf("  ");
        printf("%4i %4i %4i  ", (uint32_t)node[cur_node].l, (uint32_t)node[cur_node].u, (uint32_t)(node[cur_node].u-node[cur_node].l+1));
        printf("% .2f % .2f % .2f  % .2f % .2f % .2f  ",
            node[cur_node].bnd.x.min,
            node[cur_node].bnd.y.min,
            node[cur_node].bnd.z.min,
            node[cur_node].bnd.x.max,
            node[cur_node].bnd.y.max,
            node[cur_node].bnd.z.max);
        printf("% .2f % .2f % .2f % .2e", node[cur_node].cm.x, node[cur_node].cm.y, node[cur_node].cm.z, node[cur_node].M.m);

        if (node[cur_node].children[0] != 0)
        {
            printf(" ");
            for (i=0; node[cur_node].children[i] != 0 && i < 8; i++)
            {
                printf("%i,", node[cur_node].children[i]);
                PUSH(node[cur_node].children[i]);
            }
        }
        printf("\n");
    }

    if (node_count != tree->n_nodes)
        printf("\nERROR: Number of nodes in tree (%i) does not match n_tree_nodes (%i)!\n\n", node_count, tree->n_nodes);
    
    printf("Nodes: %i   Leaves: %i   Particles: %i", node_count, leaf_count, p_count);
    if (p_count != N)
        printf(" ** Why isn't this the total number of particle (%i)", N);
    printf("\n");

    return 0;
}

//============================================================================
//                                can_interact
//============================================================================
int can_interact(const tree_node_t *nA, const tree_node_t *nB)
{
    //const tree_node_t *nA = env.tree + A;
    //const tree_node_t *nB = env.tree + B;

    const float R = DIST(nA->cm.x - nB->cm.x, 
                         nA->cm.y - nB->cm.y, 
                         nA->cm.z - nB->cm.z);

#if 0
    printf("can_interact(): NA(%i) * NB(%i) <? Npre(%i) = %i\n", NA, NB, Npre, NA * NB < Npre);
    printf("can_interact(): NA(%i) * NB(%i) <? Npost(%i) = %i\n", NA, NB, Npost, NA * NB < Npost);
#endif
    DBG(DBG_INTERACT)
        fprintf(stderr, "can_interact(): [%i %i]  R(%f) ?> (A_rmax(%f) + B_rmax(%f)) / OA(%f) = %f -> %i\n", 
            nA->id,nB->id, R, nA->rmax, nB->rmax, env.opening_angle, 
            (nA->rmax + nB->rmax) / env.opening_angle, R > (nA->rmax + nB->rmax) / env.opening_angle);

    //fprintf(stderr, "-- (%f %f) --\n", nA->rmax, nB->rmax);
    //if (A==B && nA->size <= 8) return 1;
    return R > ((nA->rmax + nB->rmax) / env.opening_angle);
}


//============================================================================
//                              interact_dehnen
//============================================================================
int interact_dehnen(tree_t *tree)
{
    cid_t a,b;
    cid_t child_A, child_B;

    ANNOUNCE_BEGIN(__FUNCTION__);

    MAKE_STACK(cid_t)

    tree_node_t *node = tree->root;

    PUSH(1);
    PUSH(1);

    //fprintf(stderr, "---------------\n");
    //momPrintLocr(&node[9].L);

    while (!STACK_ISEMPTY())
    {
        cid_t A = POP();
        cid_t B = POP();

        if (can_interact(&node[A],&node[B]))
        {
            //printf("%6i %6i\n", node[B].id, node[A].id);
            momFloat dx = node[A].cm.x - node[B].cm.x;
            momFloat dy = node[A].cm.y - node[B].cm.y;
            momFloat dz = node[A].cm.z - node[B].cm.z;
            double df0, df1, df2;
            momFloat Rinv = 1.0 / DIST(dx, dy, dz);
            momLocrAddMomr5(&node[A].L, &node[B].M, Rinv,  dx,  dy,  dz, &df0, &df1, &df2);
            momLocrAddMomr5(&node[B].L, &node[A].M, Rinv, -dx, -dy, -dz, &df0, &df1, &df2);   
        }
        else
        {
            if (A == B)
            {
                forall_tree_node_child_pairs(A, a,b, child_A,child_B)
                {
                    PUSH(child_B);
                    PUSH(child_A);
                }
            }
            else if (node[A].rmax > node[B].rmax)
            {
                forall_tree_node_children(A, a, child_A)
                {
                    PUSH(B);
                    PUSH(child_A);
                }
            }
            else 
            {
                forall_tree_node_children(B, b, child_B)
                {
                    PUSH(child_B);
                    PUSH(A);
                }
            }
        }
    }

    //fprintf(stderr, "---------------\n");
    //momPrintLocr(&node[9].L);
    ANNOUNCE_END(__FUNCTION__);

    return 0;
}

//============================================================================
//                              evaluate_dehnen
//============================================================================
int evaluate_dehnen(tree_t *tree)
{
    Pid_t i;
    cid_t a, child_A;

    typedef struct
    {
        LOCR L;
        cid_t cid;
        pos_t r;
    } eval_t;

    ANNOUNCE_BEGIN(__FUNCTION__);

    MAKE_STACK(eval_t)

    tree_node_t *node = tree->root;

    eval_t T0;

    //------------------------------------------------------------------------
    // Start the evaluation at the root node with an empty expansion.
    //------------------------------------------------------------------------

    T0.cid = 1;
    T0.r.x = node[1].r.x;
    T0.r.y = node[1].r.y;
    T0.r.z = node[1].r.z;
    momClearLocr(&T0.L);

    PUSH(T0);

    //------------------------------------------------------------------------
    // Walk the tree.
    //------------------------------------------------------------------------

    while (!STACK_ISEMPTY())
    {
        T0 = POP();

        cid_t  A = T0.cid;
        //fprintf(stderr, "CELL %i\n", (uint32_t)A);

        momFloat dx = node[A].cm.x - T0.r.x;
        momFloat dy = node[A].cm.y - T0.r.y;
        momFloat dz = node[A].cm.z - T0.r.z;

        //--------------------------------------------------------------------
        // Accumulate the local expansion.
        //--------------------------------------------------------------------

        LOCR TA = node[A].L;
        momShiftLocr(&T0.L, dx, dy, dz);
        momAddLocr(&TA, &T0.L);

        //--------------------------------------------------------------------
        // Open up the cell and recurse, or evaluate the expansion at the
        // particles.
        //--------------------------------------------------------------------

        if (node[A].size > 8)
        {
            forall_tree_node_children(A, a, child_A)
            {
                T0.cid = child_A;
                T0.L  = TA;
                PUSH(T0);
            }
        }
        else
        {
            for (i=node[A].l; i <= node[A].u; i++)
            {
                dx = rx(i) - node[A].cm.x;
                dy = ry(i) - node[A].cm.y;
                dz = rz(i) - node[A].cm.z;
                momEvalLocr(&TA, dx, dy, dz, (momFloat *)&pot(i), (momFloat *)&ax(i), (momFloat *)&ay(i), (momFloat *)&az(i));
            }
        }
    }

    ANNOUNCE_END(__FUNCTION__);
    
    return 0;
}
