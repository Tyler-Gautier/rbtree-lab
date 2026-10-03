#include "rbtree.h"
#include <stdlib.h>
#include <string.h>

typedef enum rb_color {BLACK, RED} rb_color_t;

static void *rb_malloc(size_t t) {return malloc(t);}
static void rb_free(void *p) {free(p);}

typedef struct rbnode 
{
    void *value;
    char *key;
    rb_color_t color;
    struct rbnode *left, *right, *parent;
} rbnode_t;

struct rbtree
{
    rbnode_t *root;
    size_t size;
    rb_value_free_fn free_fn;
};

static rbnode_t NIL_NODE = { .color = BLACK };
static rbnode_t *const NIL = &NIL_NODE;

static rbnode_t *get_node(rbtree_t *t, const char *key)
{
    rbnode_t *current = t->root;
    while (current != NIL) {
        int cmp = strcmp(key, current->key);
        if (cmp == 0) {
            return current;
        }
        current = (cmp < 0) ? current->left : current->right;
    }
    return NULL; //Node not found
}

static void recolor(rbnode_t *node)
{
    node->color = (node->color == BLACK) ? RED : BLACK;
}

/* returns 0 if node/parent form a line (same side of grandparent),
 * 1 if node is a left-kink (node is parent's left child),
 * 2 if node is a right-kink (node is parent's right child) */
static int is_kink(rbnode_t *node, rbnode_t *parent)
{
    rbnode_t *grandparent = parent->parent;
    int parent_is_left = (parent == grandparent->left);
    int node_is_left = (node == parent->left);
    if (parent_is_left == node_is_left) {
        return 0;
    }
    return node_is_left ? 1 : 2;
}

static int is_leaf(rbnode_t *node)
{
    return (node->left == NIL && node->right == NIL) ? 1 : 0;
}

[[maybe_unused]] static rbnode_t *minimum(rbnode_t *node)
{
    if(node->left != NIL) {return minimum(node->left);}
    else {return node;}
}

static int delete_node(rbtree_t *t, rbnode_t *node)
{
    rb_free(node->key);
    if (t->free_fn != NULL) {
    t->free_fn(node->value);
    }
    rb_free(node);
    t->size--;
    return 0;
}

/*
  Replaces node u with node v. Replaces parent but not children.
  Updating the children is the calling functions job.
  U=Node being replaced
  V=Node replacing
*/
static void transplant(rbtree_t *t, rbnode_t *u, rbnode_t *v)
{
    if(u->parent == NIL)
    {
        t->root = v;
    }
    else if(u->parent->left == u)
    {
        u->parent->left = v;
    }
    else
    {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

static int count_children(rbnode_t *node)
{
    int count = 0;
    if(node->left != NIL) {count++;}
    if(node->right != NIL) {count++;}
    return count;
}

static rbnode_t *node_create(const char *key, void *value, rbnode_t *parent)
{
    rbnode_t *node = rb_malloc(sizeof(*node));
    if (node == NULL) {
        goto fail_node;
    }

    size_t key_len = strlen(key) + 1;
    node->key = rb_malloc(key_len);
    if (node->key == NULL) {
        goto fail_key;
    }
    memcpy(node->key, key, key_len);

    node->value = value;
    node->color = RED;
    node->left = node->right = NIL;
    node->parent = parent;
    return node;

fail_key:
    rb_free(node);
fail_node:
    return NULL;
}

rbtree_t *rb_create(rb_value_free_fn value_free)
{
    rbtree_t *t = rb_malloc(sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->root = NIL;
    t->size = 0;
    t->free_fn = value_free;
    return t;
}

size_t rb_size(const rbtree_t *t)
{
    return t->size;
}

void *rb_find(const rbtree_t *t, const char *key)
{
    rbnode_t *current = t->root;
    while (current != NIL) {
        int cmp = strcmp(key, current->key);
        if (cmp == 0) {
            return current->value;
        }
        current = (cmp < 0) ? current->left : current->right;
    }
    return NULL;
}

static void rotate_left(rbtree_t *t, rbnode_t *parent)
{
    rbnode_t *child = parent->right;
    rbnode_t *grandparent = parent->parent;

    parent->right = child->left;
    if (child->left != NIL) {
        child->left->parent = parent;
    }

    child->parent = grandparent;
    if (grandparent == NIL) {
        t->root = child;
    } else if (parent == grandparent->left) {
        grandparent->left = child;
    } else {
        grandparent->right = child;
    }

    child->left = parent;
    parent->parent = child;
}

static void rotate_right(rbtree_t *t, rbnode_t *parent)
{
    rbnode_t *child = parent->left;
    rbnode_t *grandparent = parent->parent;

    parent->left = child->right;
    if (child->right != NIL) {
        child->right->parent = parent;
    }

    child->parent = grandparent;
    if (grandparent == NIL) {
        t->root = child;
    } else if (parent == grandparent->left) {
        grandparent->left = child;
    } else {
        grandparent->right = child;
    }

    child->right = parent;
    parent->parent = child;
}

static void insert_fixup_rotate(rbtree_t *t, rbnode_t *node, int kink)
{
    rbnode_t *parent = node->parent;
    rbnode_t *grandparent = parent->parent;
    rbnode_t *promoted = parent;
    int rotate_grandparent_right = (parent == grandparent->left);

    if (kink == 1) {
        rotate_right(t, parent);
        promoted = node;
        rotate_grandparent_right = 0;
    } else if (kink == 2) {
        rotate_left(t, parent);
        promoted = node;
        rotate_grandparent_right = 1;
    }

    recolor(promoted);
    recolor(grandparent);

    if (rotate_grandparent_right) {
        rotate_right(t, grandparent);
    } else {
        rotate_left(t, grandparent);
    }
}

static void insert_fixup(rbtree_t *t, rbnode_t *node)
{
    /* invariant: node is RED on every entry to this loop */
    while (node->parent->color == RED) {
        rbnode_t *parent = node->parent;
        rbnode_t *grandparent = parent->parent;
        rbnode_t *uncle = (parent == grandparent->left) ? grandparent->right : grandparent->left;

        if (uncle->color == RED) {
            recolor(parent);
            recolor(uncle);
            recolor(grandparent);
            node = grandparent;
        } else {
            insert_fixup_rotate(t, node, is_kink(node, parent));
            break; // Rotation fixup only runs once 
        }
    }
    t->root->color = BLACK;
}

static rbnode_t *insert_recursive(rbtree_t *t, rbnode_t *current, const char *key, void *value)
{
    int cmp = strcmp(key, current->key);
    if (cmp == 0) {
        if (t->free_fn != NULL) {
            t->free_fn(current->value);
        }
        current->value = value;
        return NIL;
    }

    /* child holds the address of current->left/right itself, not a copy of the
     * node pointer stored there - a plain rbnode_t * would only copy that value,
     * so writing through it later couldn't change current->left/right at all. */
    rbnode_t **child = (cmp < 0) ? &current->left : &current->right;
    if (*child != NIL) {
        return insert_recursive(t, *child, key, value);
    }

    rbnode_t *node = node_create(key, value, current);
    if (node == NULL) {
        return NULL;
    }
    *child = node;
    t->size++;
    return node;
}

int rb_insert(rbtree_t *t, const char *key, void *value)
{
    if (t->root == NIL) {
        rbnode_t *node = node_create(key, value, NIL);
        if (node == NULL) {
            return -1;
        }
        t->root = node;
        t->size++;
        insert_fixup(t, node);
        return 0;
    }

    rbnode_t *result = insert_recursive(t, t->root, key, value);
    if (result == NULL) {
        return -1;
    }
    if (result != NIL) {
        insert_fixup(t, result);
    }
    return 0;
}

static void fixup_red_sibling(rbtree_t *t, rbnode_t *node, rbnode_t **sibling, int direction)
{
    (*sibling)->color = BLACK;
    node->parent->color = RED;
    if(direction == 1)
    {
        rotate_left(t, node->parent);
        *sibling = node->parent->right;
    }
    else
    {
        rotate_right(t, node->parent);
        *sibling = node->parent->left;
    }
}

static void fixup_black_sibling_black_children(rbnode_t **node, rbnode_t **sibling)
{
    (*sibling)->color = RED;
    *node = (*node)->parent;
}

static void fixup_far_child_black(rbtree_t *t, rbnode_t *node, rbnode_t **sibling, int direction)
{
    if(direction == 1) //Diretion of far_child points in same direction as sibling
    {
        (*sibling)->left->color = BLACK;
        (*sibling)->color = RED;
        rotate_right(t, *sibling);
        (*sibling) = node->parent->right;
    }
    else
    {
        (*sibling)->right->color = BLACK;
        (*sibling)->color = RED;
        rotate_left(t, *sibling);
        (*sibling) = node->parent->left;
    }
}
static void fixup_far_child_red(rbtree_t *t, rbnode_t **node, rbnode_t **sibling, int direction)
{
    if(direction == 1) //Diretion of close child points in opposite direction of sibling
    {
        (*sibling)->color = (*node)->parent->color;
        (*node)->parent->color = BLACK;
        (*sibling)->right->color = BLACK;
        rotate_left(t, (*node)->parent);
    }
    else
    {
        (*sibling)->color = (*node)->parent->color;    
        (*node)->parent->color = BLACK;
        (*sibling)->left->color = BLACK;
        rotate_right(t, (*node)->parent);
    }
    *node = t->root;
}

[[maybe_unused]] static int delete_fixup(rbtree_t *t, rbnode_t *node)
{
    while(node != t->root && node->color == BLACK)
    {
        rbnode_t *sibling = (node == node->parent->left) ? node->parent->right : node->parent->left;
        int direction = (node == node->parent->left) ? 1 : 0; //1=Sibling right child, 0=Sibling left child
        //Case 1: Red Sibling
        if(sibling->color == RED) 
        {
            fixup_red_sibling(t, node, &sibling, direction);
        }
        //Case 2: Black Sibling with Black Children
        if(sibling->color == BLACK && (sibling->left->color == BLACK && sibling->right->color == BLACK))
        {
            fixup_black_sibling_black_children(&node, &sibling);
        }
        else
        {
            rbnode_t *far_child = (direction == 1) ? sibling->right : sibling->left; //child farthest from node
            //Case 3: Close Child is Red
            if(far_child->color == BLACK)
            {
                fixup_far_child_black(t, node, &sibling, direction);
            }
            //Case 4: Far Child (or both Children) are Red
            fixup_far_child_red(t, &node, &sibling, direction);
        }
    }
    node->color = BLACK; 
    return 0;
}

static int delete_two_children(rbtree_t *t, rbnode_t *node)
{
    rbnode_t *successor = minimum(node->right);
    rbnode_t *x = successor->right;
    rb_color_t original_color = successor->color;

    transplant(t, successor, successor->right);
    successor->left = node->left;
    successor->right = node->right;
    node->left->parent = successor;
    node->right->parent = successor;

    transplant(t, node, successor);
    successor->color = node->color;

    if(original_color == BLACK)
    {
        delete_fixup(t, x);
    }

    return delete_node(t, node);
}

static int delete_leaf(rbtree_t *t, rbnode_t *node)
{
    transplant(t, node, NIL);
    NIL->parent = NULL; //Not strictly necessary but done for bookkeeping
    return delete_node(t, node);
}

static int delete_leaf_black(rbtree_t *t, rbnode_t *node)
{
    transplant(t, node, NIL);
    delete_fixup(t, NIL);
    NIL->parent = NULL;
    return delete_node(t, node);
}

static int delete_one_child(rbtree_t *t, rbnode_t *node)
{
    rbnode_t *child = (node->left != NIL) ? node->left : node->right;
    transplant(t, node, child);
    recolor(child); 
    return delete_node(t, node);
}

int rb_delete(rbtree_t *t, const char *key)
{
    rbnode_t *node = get_node(t, key);
    if(node == NULL) {return -1;}
    if(is_leaf(node)) 
    {
        return (node->color == RED) ? delete_leaf(t, node) : delete_leaf_black(t, node);
    }
    else if(count_children(node) == 1)
    {
        return delete_one_child(t, node);
    }
    else
    {
        return delete_two_children(t, node);
    }

    return -1;
}

static void destroy_recursive(rbtree_t *t, rbnode_t *node)
{
    if (node == NIL) {
        return;
    }
    destroy_recursive(t, node->left);
    destroy_recursive(t, node->right);
    if (t->free_fn != NULL) {
        t->free_fn(node->value);
    }
    rb_free(node->key);
    rb_free(node);
}

void rb_destroy(rbtree_t *t)
{
    if (t == NULL) {
        return;
    }
    destroy_recursive(t, t->root);
    rb_free(t);
}

static int rb_black_height(const rbnode_t *node)
{
    if(node == NIL) {return 1;}

    int leftBlackHeight = rb_black_height(node->left);
    if(leftBlackHeight == -1)
    {
        return -1;
    }
    int rightBlackHeight = rb_black_height(node->right);
    if(rightBlackHeight == -1)
    {
        return -1;
    }
    
    if (leftBlackHeight == rightBlackHeight)
    {
        return leftBlackHeight + (node->color == BLACK ? 1 : 0);
    }
    return -1;
}

static int rb_red_red_check(const rbnode_t *node)
{
    if(node == NIL) {return 1;}
    
    if(node->left != NIL && !rb_red_red_check(node->left))
    {
        return 0;
    }
    if(node->color == RED && node->parent->color == RED)
    {
        return 0;
    }
    if(node->right != NIL && !rb_red_red_check(node->right))
    {
        return 0;
    }
    return 1;
}

static int rb_order_recursive(rbnode_t *node, const char **prev)
{
    if (node->left != NIL && !rb_order_recursive(node->left, prev)) {
        return 0;
    }
    if (*prev != NULL && strcmp(*prev, node->key) >= 0) {
        return 0;
    }
    *prev = node->key;
    if (node->right != NIL && !rb_order_recursive(node->right, prev)) {
        return 0;
    }
    return 1;
}

static int rb_order(const rbtree_t *t)
{
    const char *prev = NULL;
    if (t->root == NIL) {
        return 1;
    }
    return rb_order_recursive(t->root, &prev);
}

static size_t rb_size_check_recursive(const rbnode_t *node, size_t *size)
{
    if(node != NIL)
    {
        (*size)++;
        rb_size_check_recursive(node->left, size);
        rb_size_check_recursive(node->right, size);
    }
    return *size;
}

static size_t rb_size_check(const rbtree_t *t)
{
    size_t size = 0;
    if(t->root == NIL)
    {
        return 0;
    }
    return rb_size_check_recursive(t->root, &size);
}

int rb_validate(const rbtree_t *t)
{
    if(t->root->color != BLACK) {
        return 1;
    }
    if(!rb_red_red_check(t->root))
    {
        return 1;
    }
    if(rb_black_height(t->root) == -1)
    {
        return 1;
    }
    if(!rb_order(t)) {
        return 1;
    }
    if(rb_size_check(t) != t->size)
    {
        return 1;
    }
    return 0;
}

static void rb_foreach_recursive(const rbnode_t *node,
    void (*fn)(const char *key, void *value, void *ctx),
    void *ctx)
{
    if (node->left != NIL) {
        rb_foreach_recursive(node->left, fn, ctx);
    }
    fn(node->key, node->value, ctx);
    if (node->right != NIL) {
        rb_foreach_recursive(node->right, fn, ctx);
    }
    return;
}

void rb_foreach(const rbtree_t *t,
    void (*fn)(const char *key, void *value, void *ctx),
    void *ctx)
{
    if (t->root == NIL) {
        return;
    }
    return rb_foreach_recursive(t->root, fn, ctx);
}
