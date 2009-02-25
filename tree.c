#include <assert.h>
#include <stdlib.h> // drand48
#include <math.h> // sqrt, pow

#include "env.h"
#include "mem.h"
#include "tree.h"
#include "string.h"
#include "prioq.h"
#include "macros.h"

//============================================================================
// Protoypes
//============================================================================
static int slow_fill_tree(tree_t *tree);

//============================================================================
// Globals
//============================================================================
static particulate_t *ps = NULL;

static uint32_t *queue = NULL;
static uint32_t queue_front=0, queue_back=0, queue_capacity=0, queue_size=0;

//============================================================================
// Inline functions
//============================================================================

//============================================================================
//                                 root_node
//============================================================================
static inline tree_node_t *root_node(tree_t *tree)
{
    myassert(tree != NULL, "");
    myassert(tree->total_lists != 0, "");
    myassert(tree->node_lists != NULL, "");
    myassert(tree->node_lists[0].allocd != 0, "");
    myassert(tree->node_lists[0].nodes != NULL, "");
    return &tree->node_lists[0].nodes[0];
}

static inline int HAS_CELL_PTR(tree_node_t *n, int i)
{
    return n->flags & (1<<(i+CELLREF_FLAG_OFFS));
}

static inline void SET_CELL_PTR_FLAG(tree_node_t *n, int i)
{
    n->flags |= 1 << (i+CELLREF_FLAG_OFFS);
}

static inline void UNSET_CELL_PTR_FLAG(tree_node_t *n, int i) 
{
    n->flags &= ~(1<<(i+CELLREF_FLAG_OFFS));
}

static inline int CELL_CHILD_COUNT(tree_node_t *n)
{
    return (n->flags & CELL_CHILD_COUNT_MASK) >> CELL_CHILD_COUNT_MASK_OFFS;
}

static inline void INC_CHILD_COUNT(tree_node_t *n)
{
    int f = CELL_CHILD_COUNT(n) + 1;
    dbgprintf(DBG_TREE, "%i %i\n", (int)n->id, f);
    n->flags = (n->flags & ~CELL_CHILD_COUNT_MASK) | (f << CELL_CHILD_COUNT_MASK_OFFS);
}

static inline void DEC_CHILD_COUNT(tree_node_t *n)
{
    int f = n->flags >> CELL_CHILD_COUNT_MASK_OFFS;
    n->flags = (n->flags & ~CELL_CHILD_COUNT_MASK) + ((f-1) << CELL_CHILD_COUNT_MASK_OFFS);
}

//============================================================================
//                                  cellref
//============================================================================
static inline tree_node_t *cellref(tree_node_t *node, int c)
{
    if (HAS_CELL_PTR(node, c)) 
        return (tree_node_t *)node->cellrefs[c].p;
    else if (node->cellrefs[c].ref == 0) 
        return NULL;
    else
    {
        int i;
        for (i=0; i <= 8; i++)
            eprintf("%i %p\n", node->cellrefs[i].ref, node->cellrefs[i].p);
        myassert(0, "Remote node references not currently supported.");
        return NULL;
    }
}

//============================================================================
//                                   parent
//============================================================================
static inline tree_node_t *parent(tree_node_t *node)
{
    return cellref(node, 0);
}

//============================================================================
//                                   child
//============================================================================
static tree_node_t *child(tree_node_t *node, int c)
{
    dbgassert(c > 0);
    return cellref(node, c);
}

//============================================================================
//                              set_cell_pointer
//============================================================================
static inline void set_cell_pointer(tree_node_t * const node, int c, tree_node_t * const p)
{
    node->cellrefs[c].p = p;
    SET_CELL_PTR_FLAG(node, c);
}

static inline void reset_node_list(tree_t *tree)
{
    int i;
    tree->n_nodes  = 0;
    tree->cur_list = 0;
    for (i=0; i < tree->total_lists; i++)
        tree->node_lists[i].used = 0;
}

//============================================================================
//                             current_node_list
//============================================================================
static inline node_list_t *current_node_list(tree_t *tree)
{
    if (tree->cur_list == tree->total_lists)
    {
        int nlists = tree->total_lists;
        if (tree->total_lists == 0) tree->total_lists = 1; else tree->total_lists *= 2;
        tree->node_lists = REALLOC(tree->node_lists, node_list_t, tree->total_lists);
        memset(&tree->node_lists[tree->cur_list], 0, 
               (tree->total_lists - nlists+1) * sizeof(node_list_t));
    }

    return &tree->node_lists[tree->cur_list];
}

//============================================================================
//                                  new_node
//============================================================================
static inline tree_node_t *new_node(tree_t * const tree)
{
    node_list_t *list = current_node_list(tree);

    if (list->used == list->allocd) 
    {
        if (list->allocd == 0) 
        {
            list->allocd = 2048;
            dbgassert(list->nodes == NULL);
            list->nodes = MALLOC(tree_node_t, 2048);
        }
        else
        {
            tree->cur_list++;
            list = current_node_list(tree);
        }
    } 

    tree->n_nodes++;
    return &list->nodes[list->used++];
}


//static pq_node_t *pqueue = NULL;

//============================================================================
//                                  add_node
//
// For use in build_oct_tree. 
//============================================================================
static inline tree_node_t *add_node(tree_t * const tree,
                             tree_node_t * const p, 
                             const int ci, const int level,
                             const Pid_t lower,
                             const Pid_t upper, 
                             const dist_t xm, const dist_t xM, 
                             const dist_t ym, const dist_t yM, 
                             const dist_t zm, const dist_t zM) 
{
    tree_node_t *c = NULL;
    if (upper >= lower) 
    {
        c = new_node(tree);

        c->id = tree->n_nodes;
        c->l = lower; 
        c->u = upper;
        c->size = upper-lower+1; 
        c->bnd.x.min = xm; 
        c->bnd.x.max = xM; 
        c->bnd.y.min = ym; 
        c->bnd.y.max = yM; 
        c->bnd.z.min = zm; 
        c->bnd.z.max = zM; 
        c->r.x = (xM+xm) / 2.0F; 
        c->r.y = (yM+ym) / 2.0F; 
        c->r.z = (zM+zm) / 2.0F; 
        c->rmax = 0; 
        c->flags = 0; 
        if (p != NULL) 
        {
            set_cell_pointer(p, ci, c);
            INC_CHILD_COUNT(p);
        }
        set_cell_pointer(c, 0, p);
        memset(c->cellrefs, 0, 9 * sizeof(c->cellrefs[0].ref)); 

        dbgprintf(DBG_TREE,"[%i]\tNew node %i %p  (l,u)=(%3i,%3i;%3i)  "
                           "min=(% f,% f,% f) max=(% f,% f,% f)\n", 
            (int)ci, (int)c->id, c, (int)lower,(int)upper,(int)(upper-lower+1),
            xm,ym,zm, xM,yM,zM); 
    } 

    return c;
}


//============================================================================
//                                 MAKE_STACK
//
// This creates a inline functions for manipulating a stack. It should be
// used *within* a function to create a stack specific to that function.
// This requires nested functions to be supported by the compiler.
//============================================================================
#define MAKE_STACK(T)                                                        \
static T *stack = NULL;                                                      \
static uint64_t stack_ptr, stack_capacity=0;                                 \
stack_ptr=0;                                                                 \
inline int STACK_ISEMPTY() { return stack_ptr == 0; }                   \
inline int STACK_ISFULL()  { return stack_ptr == stack_capacity; }      \
inline void PUSH(T n)                                                        \
{                                                                            \
    dbgassert(stack_ptr > stack_capacity, "stack_ptr: %ld  stack_capacity: %ld",\
        (long int)stack_ptr, (long int)stack_capacity);\
    if (stack_ptr != stack_capacity)                                                     \
    {                                                                        \
        if (0) eprintf("! %i stack " #T " ptr:%ld cap:%ld\n", env.current_step, (long int)stack_ptr, (long int)stack_capacity);  \
        stack[stack_ptr++] = n;                                              \
        return;                                                              \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        if (stack_capacity == 0) stack_capacity = 2048;                      \
        else stack_capacity *= 2;                                            \
        stack = REALLOC(stack, T, stack_capacity);                           \
        stack[stack_ptr++] = n;                                              \
        return;                                                              \
    }                                                                        \
}                                                                            \
inline T POP()  { return stack[--stack_ptr]; }                               \
inline T PEEK() { return stack[  stack_ptr]; }                               \
inline void STACK_INFO()                                                     \
{                                                                            \
    eprintf("Stack type: " #T "  capacity: %ld  ptr: %ld\n",                 \
        (long int)stack_capacity, (long int)stack_ptr);                                          \
}

//============================================================================
// A queue.
//============================================================================

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
    const particle_t *a = (particle_t *)a0;
    const particle_t *b = (particle_t *)b0;
#if 1
    if (a->sid > b->sid) return +1;
    if (a->sid < b->sid) return -1;
    myassert(0, "Particles %i and %i have the same sid!", (int)a->pid, (int)b->pid);
    return 0;
#else
    return (a->sid < b->sid) * 2 - 1;
#endif
}

//============================================================================
//                               build_oct_tree
//============================================================================
int build_oct_tree(tree_t *tree)
{
    uint32_t i;
    tree_node_t *node;
    tree_node_t *nn;

    ANNOUNCE_BEGIN("%s",__FUNCTION__);

    //------------------------------------------------------------------------
    // We use the stack for storing the index of the next node to subdivide.
    // This naturally creates a depth-first like structure.
    //------------------------------------------------------------------------
    MAKE_STACK(tree_node_t *)


    float xmin, xmax, 
          ymin, ymax, 
          zmin, zmax;

    int level = 0;

    Pid_t first = tree->l, 
          last  = tree->u, 
          N     = 1 + last - first;

    //------------------------------------------------------------------------
    // Copy the position info into the structure to sort. While we are 
    // looping over all the particles, find the max and min extents.
    //------------------------------------------------------------------------
    myassert(N > 0, "Trying to build tree with no particles.")

    ps = REALLOC(ps, particulate_t, N + 1);

    ps[first].index             = first;
    ps[first].r.x = xmin = xmax = rx(first);
    ps[first].r.y = ymin = ymax = ry(first);
    ps[first].r.z = zmin = zmax = rz(first);

    for (i=first+1; i <= last; i++)
    {
        ps[i].index = i;
        ps[i].r.x   = rx(i);
        ps[i].r.y   = ry(i);
        ps[i].r.z   = rz(i);

        if (rx(i) < xmin) xmin = rx(i); if (rx(i) > xmax) xmax = rx(i);
        if (ry(i) < ymin) ymin = ry(i); if (ry(i) > ymax) ymax = ry(i);
        if (rz(i) < zmin) zmin = rz(i); if (rz(i) > zmax) zmax = rz(i);
    }

    //------------------------------------------------------------------------
    // Setup the root node and push it on the stack to get the whole 
    // recursion going. The root node is centered at the middle of all the
    // particles. 
    //------------------------------------------------------------------------

    dist_t xR = (xmax-xmin)/2;
    dist_t yR = (ymax-ymin)/2;
    dist_t zR = (zmax-zmin)/2;
    dist_t  R = fmax(xR, fmax(yR, zR));

    //------------------------------------------------------------------------
    // Grow the box a bit so that we definitely encompass all the particles.
    // Sometime we may not due to round-off error.
    //------------------------------------------------------------------------
            R *= 1.0001;    

    dist_t xC = (xmax+xmin)/2;
    dist_t yC = (ymax+ymin)/2;
    dist_t zC = (zmax+zmin)/2;

    //fprintf(stderr, "%f %f %f  %f %f %f\n", xmin, ymin, zmin, xmax, ymax, zmax);
    //fprintf(stderr, "%f %f %f %f  %f %f %f\n", xR, yR, zR, R, xC, yC, zC);

    reset_node_list(tree);

    //------------------------------------------------------------------------
    // Root node
    //------------------------------------------------------------------------
#define ADD_NODE(_n, lower, upper, xm,xM, ym,yM, zm,zM) \
do { \
     if ((nn=add_node(tree, node, _n, level, lower,upper, xm,xM, ym,yM, zm,zM)) != NULL) PUSH(nn);\
} while (0)

    node = NULL;
    ADD_NODE(0, first, last, xC-R,xC+R, yC-R,yC+R, zC-R,zC+R);

    //------------------------------------------------------------------------
    // Start the "recursion"
    //------------------------------------------------------------------------
    while (!STACK_ISEMPTY())
    {
        node = POP();
        if (node == NULL)
        {
            level--;
            continue;
        }

        particulate_t tmp;

#if 0
        float x_split = (node[cur_node].bnd.x.min + node[cur_node].bnd.x.max) / 2.0F;
        float y_split = (node[cur_node].bnd.y.min + node[cur_node].bnd.y.max) / 2.0F;
        float z_split = (node[cur_node].bnd.z.min + node[cur_node].bnd.z.max) / 2.0F;
#endif

        const float x_split = node->r.x;
        const float y_split = node->r.y;
        const float z_split = node->r.z;

        dbgprintf(DBG_TREE, "cur_node=%i level=%i (x_split,y_split,z_split)=(%.2f, %.2f, %.2f) (stack_ptr=%i)\n",
            node->id, level, x_split, y_split, z_split, (int)stack_ptr);

        const cid_t L = node->l;
        const cid_t U = node->u;

        myassert(node->size == U-L+1, "Node: %i  %i != %i-%i+1 != %i", (int)node->id, (int)node->size, (int)U,(int)L,(int)(U-L+1));

        if (node->size <= tree->bucket_size) continue;

        //------------------------------------------------------------------------
        // Partition the current cube into eight pieces.
        //------------------------------------------------------------------------

        /* x split */
        cid_t l0=L, u0 = U;
        PARTITION(ps, tmp, l0,u0, ps[l0].r.x < x_split, x_split <= ps[u0].r.x);
        dbgprintf(DBG_TREE, "P1: (%i %i) -> (%i %i)\n", L, U, l0, u0);

            /* y split */
            cid_t l1=L, u1 = l0-1;
            PARTITION(ps, tmp, l1,u1, ps[l1].r.y < y_split, y_split <= ps[u1].r.y);
            dbgprintf(DBG_TREE, "\tP2: (%i %i) -> (%i %i)\n", L, u0, l1, u1);

            cid_t l2=l0, u2 = U;
            PARTITION(ps, tmp, l2,u2, ps[l2].r.y < y_split, y_split <= ps[u2].r.y);
            dbgprintf(DBG_TREE, "\tP3: (%i %i) -> (%i %i)\n", l0, U, l2, u2);

                /* z split */
                cid_t l3=L, u3 = l1-1;
                PARTITION(ps, tmp, l3,u3, ps[l3].r.z < z_split, z_split <= ps[u3].r.z);
                dbgprintf(DBG_TREE, "\t\tP4: (%i %i) -> (%i %i)\n", L, u1, l3, u3);

                cid_t l4=l1, u4 = l0-1;
                PARTITION(ps, tmp, l4,u4, ps[l4].r.z < z_split, z_split <= ps[u4].r.z);
                dbgprintf(DBG_TREE, "\t\tP5: (%i %i) -> (%i %i)\n", l1, u0, l4, u4);

                cid_t l5=l0, u5 = l2-1;
                PARTITION(ps, tmp, l5,u5, ps[l5].r.z < z_split, z_split <= ps[u5].r.z);
                dbgprintf(DBG_TREE, "\t\tP6: (%i %i) -> (%i %i)\n", l0, u2, l5, u5);

                cid_t l6=l2, u6 = U;
                PARTITION(ps, tmp, l6,u6, ps[l6].r.z < z_split, z_split <= ps[u6].r.z);
                dbgprintf(DBG_TREE, "\t\tP7: (%i %i) -> (%i %i)\n", l2, U, l6, u6);


        dbgprintf(DBG_TREE, "%i %i %i %i %i %i %i %i %i\n", L,l3,l1,l4,l0,l5,l2,l6,U);


        //------------------------------------------------------------------------
        // For each new node, add it to the tree (if not empty) and push it on
        // the stack.
        //------------------------------------------------------------------------

        level++;
        PUSH(NULL);

        ADD_NODE(1, L,l3-1, node->bnd.x.min, x_split,
                            node->bnd.y.min, y_split,
                            node->bnd.z.min, z_split);
        
        ADD_NODE(2,l3, l1-1, node->bnd.x.min, x_split,
                             node->bnd.y.min, y_split,
                             z_split,         node->bnd.z.max);
        
        ADD_NODE(3,l1,l4-1, node->bnd.x.min, x_split,
                            y_split,         node->bnd.y.max,
                            node->bnd.z.min, z_split);
        
        ADD_NODE(4,l4,l0-1, node->bnd.x.min, x_split,
                            y_split,         node->bnd.y.max,
                            z_split,         node->bnd.z.max);
        
        ADD_NODE(5,l0,l5-1, x_split,         node->bnd.x.max,
                            node->bnd.y.min, y_split,
                            node->bnd.z.min, z_split);
        
        ADD_NODE(6,l5,l2-1, x_split,         node->bnd.x.max,
                            node->bnd.y.min, y_split,
                            z_split,         node->bnd.z.max);
        
        ADD_NODE(7,l2,l6-1, x_split,         node->bnd.x.max,
                            y_split,         node->bnd.y.max,
                            node->bnd.z.min, z_split);
        
        ADD_NODE(8,l6,U, x_split, node->bnd.x.max,
                         y_split, node->bnd.y.max,
                         z_split, node->bnd.z.max);
    }
    

    //tree->n_nodes = next_node-1;
    log("TREE", "#nodes %i\n", tree->n_nodes);


    //------------------------------------------------------------------------
    // Reorder the real particle array to match the sorted list
    //------------------------------------------------------------------------
    for (i=first; i <= last; i++) sid(ps[i].index) = i;
    qsort(env.ps+first, N, sizeof(env.ps[0]), _psort_cmp);

    //eprintf("first=%i last=%i\n", (int)first, (int)last);
    //eprintf("j=%i %f,%f,%f\n", 3, rx(3),ry(3),rz(3));

#if 0
    Pid_t j;
    tree_node_t *node = tree->root;
    for (i=1; i <= tree->n_nodes; i++)
    {
        for (j=node[i].l; j <= node[i].u; j++)
        {
            myassert(node[i].bnd.x.min <= rx(j) && rx(j) <= node[i].bnd.x.max, 
                "(i,j)=(%i,%i)  %e <= %e <= %e", 
                (int)i,(int)j, 
                (float)node[i].bnd.x.min, (float)rx(j), (float)node[i].bnd.x.max);
            myassert(node[i].bnd.y.min <= ry(j) && ry(j) <= node[i].bnd.y.max, 
                "(i,j)=(%i,%i)  %e <= %e <= %e", 
                (int)i,(int)j, 
                (float)node[i].bnd.y.min, (float)ry(j), (float)node[i].bnd.y.max);
            myassert(node[i].bnd.z.min <= rz(j) && rz(j) <= node[i].bnd.z.max, 
                "(i,j)=(%i,%i) (%i,%i) %e <= %e <= %e", 
                (int)i,(int)j, (int)node[i].id, (int)id(j),
                (float)node[i].bnd.z.min, (float)rz(j), (float)node[i].bnd.z.max);
        }
    }
#endif

    ANNOUNCE_END(__FUNCTION__);

    return 0;
}

//============================================================================
//                                 fill_tree
//============================================================================
int fill_tree(tree_t *tree)
{
    uint32_t i;

    myassert(tree != NULL, "Oct tree has not been created.\n");

    MAKE_STACK(tree_node_t *)

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    //stack_ptr=0;

    PUSH(root_node(tree));

    cid_t N = tree->u - tree->l + 1;

    cid_t node_count=0, leaf_count=0, p_count=0;

    while (!STACK_ISEMPTY())
    {
        tree_node_t *node = POP();
    }

    return slow_fill_tree(tree);
}

//============================================================================
//                               slow_fill_tree
//============================================================================
static int slow_fill_tree(tree_t *tree)
{
    uint32_t i,j;

    ANNOUNCE_BEGIN(__FUNCTION__);

    myassert(tree != NULL, "Oct tree has not been created.\n");

    tree_node_t *node;

    for (i=0; i < tree->total_lists; i++)
    {
        for (j=0; j < tree->node_lists[i].used; j++)
        {
            MOMR M;

            node = &tree->node_lists[i].nodes[j];

            //----------------------------------------------------------------
            // Calculate the center of mass of each node
            //----------------------------------------------------------------
            float cx=0, cy=0, cz=0, m=0;
            for (j=node->l; j <= node->u; j++)
            {
                cx += M(j) * rx(j);
                cy += M(j) * ry(j);
                cz += M(j) * rz(j);
                 m += M(j);
            }
            cx = node->cm.x = cx / m;
            cy = node->cm.y = cy / m;
            cz = node->cm.z = cz / m;

#if 0
            if (i==1)
            {
                fprintf(stderr, "CELL 1: m=%f M(1)=%f l=%i u=%i\n", m, M(1), (uint32_t)node[i].l, (uint32_t)node[i].u);
            }
#endif

            momClearLocr(&node->L);
            momClearMomr(&node->M);
            for (j=node[i].l; j <= node[i].u; j++)
            {
                momMakeMomr(&M, M(j), rx(j)-cx, ry(j)-cy, rz(j)-cz);
                momAddMomr(&node->M, &M);
            }

            //----------------------------------------------------------------
            // Find the most distant particle in each node from its CM 
            //----------------------------------------------------------------
            float rmax2=0;
            for (j=node->l; j <= node->u; j++)
            {
                myassert(node[i].bnd.x.min <= rx(j) && rx(j) <= node[i].bnd.x.max, 
                    "(i,j)=(%i,%i)  %e <= %e <= %e", i,j, node[i].bnd.x.min, rx(j), node[i].bnd.x.max);
                myassert(node[i].bnd.y.min <= ry(j) && ry(j) <= node[i].bnd.y.max, 
                    "(i,j)=(%i,%i)  %e <= %e <= %e", i,j, node[i].bnd.y.min, ry(j), node[i].bnd.y.max);
                myassert(node[i].bnd.z.min <= rz(j) && rz(j) <= node[i].bnd.z.max, 
                    "(i,j)=(%i,%i)  %e <= %e <= %e", i,j, node[i].bnd.z.min, rz(j), node[i].bnd.z.max);

                const dist_t r2 = DIST2(rx(j)-cx, ry(j)-cy, rz(j)-cz);
                if (r2 > rmax2) rmax2 = r2;
            }

            //----------------------------------------------------------------
            // rmax is the distance between the CM and the furthest particle,
            // unless there is only a single particle in the box, in which 
            // case we take the size of the box itself. Since the boxes are
            // cubes, we can just use the length in any of the dimensions.
            //----------------------------------------------------------------
            if (rmax2 == 0)
            {
                //dist_t xR  = (node->bnd.x.max - node->bnd.x.min)/2;
                //dist_t yR  = (node->bnd.y.max - node->bnd.y.min)/2;
                //dist_t zR  = (node->bnd.z.max - node->bnd.z.min)/2;
                //node->rmax = fmax(xR, fmax(yR, zR));
                node->rmax = (node->bnd.x.max - node->bnd.x.min)/2;
            }
            else
            {
                node[i].rmax = sqrt(rmax2);
            }

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
    }

#if 0
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

    ANNOUNCE_END(__FUNCTION__);

    return 0;
}

//============================================================================
//                               print_oct_tree
//============================================================================
int print_oct_tree(tree_t *tree)
{
    uint32_t i;

    myassert(tree != NULL, "Oct tree has not been created.\n");

    MAKE_STACK(tree_node_t *)

    //stack = REALLOC(stack, uint32_t, MAX_STACK_SIZE);
    //stack_ptr=0;

    PUSH(root_node(tree));

    cid_t N = tree->u - tree->l + 1;

    cid_t node_count=0, leaf_count=0, p_count=0;

    while (!STACK_ISEMPTY())
    {
        tree_node_t *node = POP();

        if ((node_count % 40) == 0) 
        {
            log("PTREE", "%-5s %-4s  %4s %4s %4s %5s %5s %5s  %5s %5s %5s  %5s %5s %5s %5s\n",
                "Node", "Type", "lowr", "uppr", "#", "xm","ym","zm", "xM","yM","zM",
                "CMx", "CMy", "CMz", "M");
            log("PTREE", BAR1 "------------\n");
        }

        node_count++;

        log("PTREE", "%5i %i %x %p ", (int)node->id, CELL_CHILD_COUNT(node), node->flags, node);
        if (CELL_CHILD_COUNT(node) == 0)
        {
            log("", "%-4s", " L");
            leaf_count++;
            p_count += node->u - node->l + 1;
        }
        else
            log("", "%-4s", "N");

        log("", "  ");
        log("", "%4i %4i %4i  ", (uint32_t)node->l, (uint32_t)node->u, 
                                (uint32_t)(node->u-node->l+1));
        log("", "% .2f % .2f % .2f  % .2f % .2f % .2f  ",
            node->bnd.x.min,
            node->bnd.y.min,
            node->bnd.z.min,
            node->bnd.x.max,
            node->bnd.y.max,
            node->bnd.z.max);
        log("", "% .2f % .2f % .2f % .2e", 
            node->cm.x, node->cm.y, node->cm.z, node->M.m);

        // Print the list of children
        tree_node_t *c;
        log("", " ");
        forall_tree_node_children(node, i, c)
        {
            log("", "%i,", c->id);
            PUSH(c);
        }
        log("", "\n");
    }

    myassert(node_count == tree->n_nodes,
        "Number of nodes in tree (%i) does not match n_nodes (%i)!", node_count, tree->n_nodes);
    
    log("PTREE", "Nodes: %i   Leaves: %i   Particles: %i\n", node_count, leaf_count, p_count);
    if (p_count != N)
        log("ERROR", " ** Why isn't this the total number of particle (%i)", N);
    log("", "\n");

    return 0;
}

//============================================================================
//                                 n2_mutual
//============================================================================
inline static int n2_mutual(const Pid_t i0, const Pid_t i1)
{
    Pid_t i,j;

    dbgassert(0 < i0 && i0 <= env.n_particles, "");
    dbgassert(0 < i1 && i1 <= env.n_particles, "");

    //eprintf("n2 mutual on %i particles %i-%i\n", (int)(1+i1-i0), (int)i0, (int)i1);
    for (i=i0; i < i1; i++)
    {
        const dist_t rx = rx(i), 
                     ry = ry(i), 
                     rz = rz(i);
        const mass_t M  =  M(i);
        dist_t ax = 0, ay = 0, az = 0;

        for (j=i+1; j <= i1; j++)
        {
            const dist_t dx = rx(j)-rx;
            const dist_t dy = ry(j)-ry;
            const dist_t dz = rz(j)-rz;
            const dist_t Rinv = 1.0 / DIST(dx,dy,dz);

            ax    += (((M(j) * dx) * Rinv) * Rinv) * Rinv;
            ay    += (((M(j) * dy) * Rinv) * Rinv) * Rinv;
            az    += (((M(j) * dz) * Rinv) * Rinv) * Rinv;

            ax(j) -= (((M    * dx) * Rinv) * Rinv) * Rinv;
            ay(j) -= (((M    * dy) * Rinv) * Rinv) * Rinv;
            az(j) -= (((M    * dz) * Rinv) * Rinv) * Rinv;
        }

        ax(i) += ax;
        ay(i) += ay;
        az(i) += az;
    }
    
    return 0;
}

//============================================================================
//                                n2_disjoint
//============================================================================
inline static int n2_disjoint(const Pid_t i0, const Pid_t i1, 
                              const Pid_t j0, const Pid_t j1)
{
    Pid_t i,j;

    for (i=i0; i <= i1; i++)
    {
        const dist_t rx = rx(i), 
                     ry = ry(i), 
                     rz = rz(i);
        const mass_t M  =  M(i);
        dist_t ax = 0, ay = 0, az = 0;

        for (j=j0; j <= j1; j++)
        {
            const dist_t dx = rx(j)-rx;
            const dist_t dy = ry(j)-ry;
            const dist_t dz = rz(j)-rz;
            const dist_t Rinv = 1.0 / DIST(dx,dy,dz);

            ax    += (((M(j) * dx) * Rinv) * Rinv) * Rinv;
            ay    += (((M(j) * dy) * Rinv) * Rinv) * Rinv;
            az    += (((M(j) * dz) * Rinv) * Rinv) * Rinv;

            ax(j) -= (((M    * dx) * Rinv) * Rinv) * Rinv;
            ay(j) -= (((M    * dy) * Rinv) * Rinv) * Rinv;
            az(j) -= (((M    * dz) * Rinv) * Rinv) * Rinv;
        }

        ax(i) += ax;
        ay(i) += ay;
        az(i) += az;
    }

    return 0;
}

//============================================================================
//                                can_interact
//============================================================================
#if 0
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
#else
inline int well_separated(const tree_node_t *nA, const tree_node_t *nB)
{
    const float R = DIST(nA->cm.x - nB->cm.x, 
                         nA->cm.y - nB->cm.y, 
                         nA->cm.z - nB->cm.z);

    return R > ((nA->rmax + nB->rmax) / env.opening_angle);
}
#endif

//============================================================================
//                              interact_dehnen
//============================================================================
int interact_dehnen(tree_t *tree)
{
    int a,b;
    tree_node_t *child_A, *child_B;

    ANNOUNCE_BEGIN(__FUNCTION__);

    MAKE_STACK(tree_node_t *)

    tree_node_t *root = root_node(tree);

    PUSH(root);
    PUSH(root);

    //fprintf(stderr, "---------------\n");
    //momPrintLocr(&node[9].L);

    while (!STACK_ISEMPTY())
    {
        tree_node_t *A = POP();
        tree_node_t *B = POP();

        //--------------------------------------------------------------------
        // Determine if we must split the cell or if we can perform a 
        // direct gravity calculation on particles.
        //--------------------------------------------------------------------
        if (A == B)
        {
            if (A->size <= env.icrit.N_cs)
            {
                n2_mutual(A->l, A->u);
                continue;
            }
        }
        else if (A->size*B->size <= env.icrit.N_pre_cc)
        {
            n2_disjoint(A->l, A->u, 
                        B->l, B->u);
            continue;
        }
        else if (well_separated(A,B))
        {
            //printf("%6i %6i\n", node[B].id, node[A].id);
            double df0, df1, df2;
            momFloat dx = A->cm.x - B->cm.x;
            momFloat dy = A->cm.y - B->cm.y;
            momFloat dz = A->cm.z - B->cm.z;
            momFloat Rinv = 1.0 / DIST(dx, dy, dz);
            momLocrAddMomr5(&(A->L), &(B->M), Rinv,  dx,  dy,  dz, &df0, &df1, &df2);
            momLocrAddMomr5(&(B->L), &(A->M), Rinv, -dx, -dy, -dz, &df0, &df1, &df2);   
            continue;
        }
        else if (A->size*B->size <= env.icrit.N_post_cc)
        {
            n2_disjoint(A->l, A->u, 
                        B->l, B->u);
            continue;
        }

        //--------------------------------------------------------------------
        // Split the cell
        //--------------------------------------------------------------------

        if (A == B)
        {
            forall_tree_node_child_pairs(A, a,b, child_A,child_B)
            {
                PUSH(child_B);
                PUSH(child_A);
            }
        }
        else if (A->rmax > B->rmax)
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
    int a;
    tree_node_t *child_A;

    typedef struct
    {
        LOCR L;
        tree_node_t *node;
        pos_t r;
    } eval_t;

    ANNOUNCE_BEGIN(__FUNCTION__);

    MAKE_STACK(eval_t)

    tree_node_t *root = root_node(tree);

    eval_t T0;

    //------------------------------------------------------------------------
    // Start the evaluation at the root node with an empty expansion.
    //------------------------------------------------------------------------

    T0.node = root;
    T0.r = root->r;
    momClearLocr(&T0.L);

    PUSH(T0);

    //------------------------------------------------------------------------
    // Walk the tree.
    //------------------------------------------------------------------------

    while (!STACK_ISEMPTY())
    {
        T0 = POP();

        tree_node_t *A = T0.node;
        //fprintf(stderr, "CELL %i\n", (uint32_t)A);

        momFloat dx = A->cm.x - T0.r.x;
        momFloat dy = A->cm.y - T0.r.y;
        momFloat dz = A->cm.z - T0.r.z;

        //--------------------------------------------------------------------
        // Accumulate the local expansion.
        //--------------------------------------------------------------------

        LOCR TA = A->L;
        momShiftLocr(&T0.L, dx, dy, dz);
        momAddLocr(&TA, &T0.L);

        //--------------------------------------------------------------------
        // Open up the cell and recurse, or evaluate the expansion at the
        // particles.
        //--------------------------------------------------------------------

        if (A->size > tree->bucket_size)
        {
            forall_tree_node_children(A, a, child_A)
            {
                T0.node = child_A;
                T0.L    = TA;
                T0.r    = A->cm;
                PUSH(T0);
            }
        }
        else
        {
#if 1
            for (i=A->l; i <= A->u; i++)
            {
                dx = rx(i) - A->cm.x;
                dy = ry(i) - A->cm.y;
                dz = rz(i) - A->cm.z;
                momEvalLocr(&TA, dx, dy, dz, 
                    (momFloat *)&pot(i), 
                    (momFloat *)&ax(i), (momFloat *)&ay(i), (momFloat *)&az(i));
            }
#endif
        }
    }

    ANNOUNCE_END(__FUNCTION__);
    
    return 0;
}
