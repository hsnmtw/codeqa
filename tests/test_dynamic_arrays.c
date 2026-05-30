
#include "heap.h"
#include "common.h"
#include "collections.h"
#include "tests.h"


static char* to_upper(const char* s, size_t index) {
    (void)index;
    char* out = STRDUP(s);
    for (char* p = out; *p; p++) *p = (char)toupper((unsigned char)*p);
    return out;
}

static bool is_long(const char* s, size_t index) {
    (void)index;
    return strlen(s) > 3;
}

static bool is_even_index(const char* s, size_t index) {
    (void)s;
    return index % 2 == 0;
}

// =============================================================================
// da_init
// =============================================================================

TEST(da_init_sets_defaults) {
    DynamicArray da;
    da_init(&da);
    EXPECT_NULL(da.items);
    EXPECT_INT((int)da.len,       0);
    EXPECT_INT((int)da.capacity,  0);
    EXPECT(da.stack == false);
    da_free(&da);
}

// =============================================================================
// da_push / da_pop
// =============================================================================

TEST(da_push_increments_len) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "a");
    da_push(&da, "b");
    EXPECT_INT((int)da.len, 2);
    da_free(&da);
}

TEST(da_pop_returns_last) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "first");
    da_push(&da, "second");
    char* val = da_pop(&da);
    EXPECT_STR(val, "second");
    EXPECT_INT((int)da.len, 1);
    FREE(val);
    da_free(&da);
}

TEST(da_pop_empty_returns_null) {
    DynamicArray da;
    da_init(&da);
    EXPECT_NULL(da_pop(&da));
    da_free(&da);
}

TEST(da_push_grows_beyond_initial_capacity) {
    DynamicArray da;
    da_init(&da);
    for (int i = 0; i < DA_INITIAL_CAPACITY * 2; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "item_%d", i);
        da_push(&da, buf);
    }
    EXPECT_INT((int)da.len, DA_INITIAL_CAPACITY * 2);
    EXPECT(da.capacity >= (size_t)(DA_INITIAL_CAPACITY * 2));
    da_free(&da);
}

// =============================================================================
// da_queue / da_dequeue
// =============================================================================

TEST(da_queue_then_dequeue_fifo) {
    DynamicArray da;
    // ___enable_debug = true;
    // int d=0;
    /*printf("<%d>\n",d++);*/ da_init(&da);
    /*printf("<%d>\n",d++);*/ da_queue(&da, "first");
    /*printf("<%d>\n",d++);*/ da_queue(&da, "second");
    /*printf("<%d>\n",d++);*/ da_queue(&da, "third");
    /*printf("<%d>\n",d++);*/ char* a = da_dequeue(&da);
    /*printf("<%d>\n",d++);*/ char* b = da_dequeue(&da);
    /*printf("<%d>\n",d++);*/ EXPECT_STR(a, "first");
    /*printf("<%d>\n",d++);*/ EXPECT_STR(b, "second");
    /*printf("<%d>\n",d++);*/ EXPECT_INT((int)da.len, 1);
    /*printf("<%d>\n",d++);*/ FREE(a);
    /*printf("<%d>\n",d++);*/ FREE(b);
    da_free(&da);
    ___enable_debug = false;
}

TEST(da_dequeue_empty_returns_null) {
    DynamicArray da;
    da_init(&da);
    EXPECT_NULL(da_dequeue(&da));
    da_free(&da);
}

// =============================================================================
// da_locate
// =============================================================================

TEST(da_locate_existing_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "apple");
    da_push(&da, "banana");
    da_push(&da, "cherry");
    EXPECT_INT((int)da_locate(&da, "banana"), 1);
    da_free(&da);
}

TEST(da_locate_missing_returns_minus_one) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "apple");
    EXPECT_INT((int)da_locate(&da, "ghost"), -1);
    da_free(&da);
}

TEST(da_locate_first_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "only");
    EXPECT_INT((int)da_locate(&da, "only"), 0);
    da_free(&da);
}

// =============================================================================
// da_remove
// =============================================================================

TEST(da_remove_middle_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "a");
    da_push(&da, "b");
    da_push(&da, "c");
    bool ok = da_remove(&da, 1);
    EXPECT(ok);
    EXPECT_INT((int)da.len, 2);
    EXPECT_STR(da.items[0], "a");
    EXPECT_STR(da.items[1], "c");
    da_free(&da);
}

TEST(da_remove_first_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "x");
    da_push(&da, "y");
    da_remove(&da, 0);
    EXPECT_INT((int)da.len, 1);
    EXPECT_STR(da.items[0], "y");
    da_free(&da);
}

TEST(da_remove_last_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "x");
    da_push(&da, "y");
    da_remove(&da, 1);
    EXPECT_INT((int)da.len, 1);
    EXPECT_STR(da.items[0], "x");
    da_free(&da);
}

TEST(da_remove_invalid_index_returns_false) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "a");
    EXPECT(!da_remove(&da, 5));
    EXPECT(!da_remove(&da, -1));
    da_free(&da);
}

// =============================================================================
// da_sort
// =============================================================================

TEST(da_sort_alphabetically) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "banana");
    da_push(&da, "apple");
    da_push(&da, "cherry");
    da_sort(&da);
    EXPECT_STR(da.items[0], "apple");
    EXPECT_STR(da.items[1], "banana");
    EXPECT_STR(da.items[2], "cherry");
    da_free(&da);
}

TEST(da_sort_already_sorted) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "a");
    da_push(&da, "b");
    da_push(&da, "c");
    da_sort(&da);
    EXPECT_STR(da.items[0], "a");
    EXPECT_STR(da.items[2], "c");
    da_free(&da);
}

TEST(da_sort_single_item) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "only");
    da_sort(&da);
    EXPECT_STR(da.items[0], "only");
    da_free(&da);
}

// =============================================================================
// da_map
// =============================================================================

TEST(da_map_transforms_all_items) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "hello");
    da_push(&src, "world");
    EXPECT_NULL(dest.items);
    EXPECT_NOT_NULL(src.items);
    EXPECT_INT((int)src.len, 2);
    EXPECT_INT((int)dest.len, 0);
    da_map(&src, &dest, to_upper);
    EXPECT_INT((int)dest.len, 2);
    EXPECT_STR(dest.items[0], "HELLO");
    EXPECT_STR(dest.items[1], "WORLD");
    da_free(&src);
    da_free(&dest);
}

TEST(da_map_empty_src) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_map(&src, &dest, to_upper);
    EXPECT_INT((int)dest.len, 0);
    da_free(&src);
    da_free(&dest);
}

TEST(da_map_does_not_mutate_src) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "hello");
    da_map(&src, &dest, to_upper);
    EXPECT_STR(src.items[0], "hello");  // src unchanged
    da_free(&src);
    da_free(&dest);
}

// =============================================================================
// da_filter
// =============================================================================

TEST(da_filter_by_length) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "hi");
    da_push(&src, "hello");
    da_push(&src, "hey");
    da_push(&src, "world");
    da_filter(&src, &dest, is_long);
    EXPECT_INT((int)dest.len, 2);
    EXPECT_STR(dest.items[0], "hello");
    EXPECT_STR(dest.items[1], "world");
    da_free(&src);
    da_free(&dest);
}

TEST(da_filter_by_index) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "a");
    da_push(&src, "b");
    da_push(&src, "c");
    da_push(&src, "d");
    da_filter(&src, &dest, is_even_index);
    EXPECT_INT((int)dest.len, 2);
    EXPECT_STR(dest.items[0], "a");
    EXPECT_STR(dest.items[1], "c");
    da_free(&src);
    da_free(&dest);
}

TEST(da_filter_none_match) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "hi");
    da_push(&src, "yo");
    da_filter(&src, &dest, is_long);
    EXPECT_INT((int)dest.len, 0);
    da_free(&src);
    da_free(&dest);
}

TEST(da_filter_all_match) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "hello");
    da_push(&src, "world");
    da_filter(&src, &dest, is_long);
    EXPECT_INT((int)dest.len, 2);
    da_free(&src);
    da_free(&dest);
}

// =============================================================================
// da_distinct
// =============================================================================

TEST(da_distinct_removes_duplicates) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "apple");
    da_push(&src, "banana");
    da_push(&src, "apple");
    da_push(&src, "cherry");
    da_push(&src, "banana");
    da_distinct(&src, &dest);
    EXPECT_INT((int)dest.len, 3);
    da_free(&src);
    da_free(&dest);
}

TEST(da_distinct_preserves_order) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "c");
    da_push(&src, "a");
    da_push(&src, "b");
    da_push(&src, "a");
    da_distinct(&src, &dest);
    EXPECT_STR(dest.items[0], "c");
    EXPECT_STR(dest.items[1], "a");
    EXPECT_STR(dest.items[2], "b");
    da_free(&src);
    da_free(&dest);
}

TEST(da_distinct_no_duplicates) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "x");
    da_push(&src, "y");
    da_push(&src, "z");
    da_distinct(&src, &dest);
    EXPECT_INT((int)dest.len, 3);
    da_free(&src);
    da_free(&dest);
}

TEST(da_distinct_all_duplicates) {
    DynamicArray src, dest = {0};
    da_init(&src);
    da_push(&src, "same");
    da_push(&src, "same");
    da_push(&src, "same");
    da_distinct(&src, &dest);
    EXPECT_INT((int)dest.len, 1);
    EXPECT_STR(dest.items[0], "same");
    da_free(&src);
    da_free(&dest);
}

// =============================================================================
// da_count
// =============================================================================

TEST(da_count_matching_predicate) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "hi");
    da_push(&da, "hello");
    da_push(&da, "hey");
    da_push(&da, "world");
    EXPECT_INT((int)da_count(&da, is_long), 2);
    da_free(&da);
}

TEST(da_count_none_match) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "hi");
    da_push(&da, "yo");
    EXPECT_INT((int)da_count(&da, is_long), 0);
    da_free(&da);
}

TEST(da_count_all_match) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "hello");
    da_push(&da, "world");
    EXPECT_INT((int)da_count(&da, is_long), 2);
    da_free(&da);
}

// =============================================================================
// da_clear
// =============================================================================

TEST(da_clear_resets_len) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "a");
    da_push(&da, "b");
    da_clear(&da);
    EXPECT_INT((int)da.len, 0);
    da_free(&da);
}

TEST(da_clear_then_push) {
    DynamicArray da;
    da_init(&da);
    da_push(&da, "old");
    da_clear(&da);
    da_push(&da, "new");
    EXPECT_INT((int)da.len, 1);
    EXPECT_STR(da.items[0], "new");
    da_free(&da);
}

// =============================================================================
// DA_OF macro
// =============================================================================

TEST(da_da_of_stack_array) {
    DynamicArray da = DA_OF("x", "y", "z");
    EXPECT_INT((int)da.len,      3);
    EXPECT_INT((int)da.capacity, 3);
    EXPECT(da.stack == true);
    EXPECT_STR(da.items[0], "x");
    EXPECT_STR(da.items[2], "z");
    // stack=true: no heap alloc, no da_free needed
}

TEST(da_da_of_single_item) {
    DynamicArray da = DA_OF("only");
    EXPECT_INT((int)da.len, 1);
    EXPECT_STR(da.items[0], "only");
}

// =============================================================================
// runner
// =============================================================================

int test_dynamic_arrays(void) {
    printf("DynamicArray tests");
    printf("\n==================");

    RUN(da_init_sets_defaults);
    RUN(da_push_increments_len);
    RUN(da_pop_returns_last);
    RUN(da_pop_empty_returns_null);
    RUN(da_push_grows_beyond_initial_capacity);
    RUN(da_queue_then_dequeue_fifo);
    RUN(da_dequeue_empty_returns_null);
    RUN(da_locate_existing_item);
    RUN(da_locate_missing_returns_minus_one);
    RUN(da_locate_first_item);
    RUN(da_remove_middle_item);
    RUN(da_remove_first_item);
    RUN(da_remove_last_item);
    RUN(da_remove_invalid_index_returns_false);
    RUN(da_sort_alphabetically);
    RUN(da_sort_already_sorted);
    RUN(da_sort_single_item);
    RUN(da_map_transforms_all_items);
    RUN(da_map_empty_src);
    RUN(da_map_does_not_mutate_src);
    RUN(da_filter_by_length);
    RUN(da_filter_by_index);
    RUN(da_filter_none_match);
    RUN(da_filter_all_match);
    RUN(da_distinct_removes_duplicates);
    RUN(da_distinct_preserves_order);
    RUN(da_distinct_no_duplicates);
    RUN(da_distinct_all_duplicates);
    RUN(da_count_matching_predicate);
    RUN(da_count_none_match);
    RUN(da_count_all_match);
    RUN(da_clear_resets_len);
    RUN(da_clear_then_push);
    RUN(da_da_of_stack_array);
    RUN(da_da_of_single_item);

    printf("\n\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
