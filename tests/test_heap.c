#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "heap.h"
#include "logger.h"
#include "tests.h"

// -- helpers ------------------------------------------------------------------




// =============================================================================
// MALLOC
// =============================================================================

TEST(malloc_returns_non_null_for_valid_size) {
    reset_memory();
    void* p = MALLOC(16);
    EXPECT(p != NULL);
}

TEST(malloc_returns_null_for_zero_size) {
    reset_memory();
    void* p = MALLOC(0);
    EXPECT_NULL(p);
}

TEST(malloc_pointer_is_inside_heap) {
    reset_memory();
    void* p = MALLOC(32);
    EXPECT(in_heap(p));
}

TEST(malloc_multiple_pointers_are_distinct) {
    reset_memory();
    void* p1 = MALLOC(16);
    void* p2 = MALLOC(16);
    void* p3 = MALLOC(16);
    EXPECT(p1 != p2 && p2 != p3 && p1 != p3);
}

TEST(malloc_multiple_pointers_no_overlap) {
    reset_memory();
    MALLOC(16);
    MALLOC(32);
    MALLOC(64);
    EXPECT(free_no_overlap());
}

TEST(malloc_pointers_are_contiguous_on_fresh_heap) {
    reset_memory();
    uint8_t* p1 = MALLOC(16);
    uint8_t* p2 = MALLOC(16);
    EXPECT(p2 == p1 + 16);
}

TEST(malloc_chunk_count_increments) {
    reset_memory();
    MALLOC(8);
    MALLOC(8);
    MALLOC(8);
    EXPECT_INT((int)heap_used_count(), 3);
}

TEST(malloc_written_data_survives) {
    reset_memory();
    char* p = MALLOC(32);
    memcpy(p, "hello world", 12);
    EXPECT_STR(p, "hello world");
}

TEST(malloc_large_allocation) {
    reset_memory();
    void* p = MALLOC(MEMORY_SIZE / 2);
    EXPECT(p != NULL);
    EXPECT(in_heap(p));
}

TEST(malloc_fills_heap_exactly) {
    reset_memory();
    void* p = MALLOC(MEMORY_SIZE);
    EXPECT(p != NULL);
}

TEST(malloc_returns_null_when_oom) {
    reset_memory();
    MALLOC(MEMORY_SIZE);               // fill heap
    void* p = MALLOC(1);               // should fail
    EXPECT_NULL(p);
}

TEST(malloc_returns_null_when_chunks_full) {
    reset_memory();
    // fill chunk table with 1-byte allocs
    for (size_t i = 0; i < CHUNKS_SIZE; i++) MALLOC(1);
    void* p = MALLOC(1);
    EXPECT_NULL(p);
}

TEST(malloc_independent_regions_dont_corrupt) {
    reset_memory();
    char* a = MALLOC(64);
    char* b = MALLOC(64);
    memset(a, 'A', 64);
    memset(b, 'B', 64);
    bool ok = true;
    for (int i = 0; i < 64; i++) if (a[i] != 'A') ok = false;
    for (int i = 0; i < 64; i++) if (b[i] != 'B') ok = false;
    EXPECT(ok);
}

TEST(malloc_size_1_allocation) {
    reset_memory();
    void* p = MALLOC(1);
    EXPECT(p != NULL);
    EXPECT(in_heap(p));
}

TEST(malloc_reuses_freed_slot) {
    reset_memory();
    void* p1 = MALLOC(32);
    FREE(p1);
    void* p2 = MALLOC(32);
    EXPECT(p1 == p2);   // same address reused
}

TEST(malloc_reuses_freed_slot_smaller_size) {
    reset_memory();
    void* p1 = MALLOC(64);
    FREE(p1);
    void* p2 = MALLOC(32);   // smaller fits in freed slot
    EXPECT(p2 != NULL);
    EXPECT(in_heap(p2));
}

TEST(malloc_does_not_reuse_slot_too_small) {
    reset_memory();
    void* p1 = MALLOC(16);
    FREE(p1);
    void* p2 = MALLOC(32);   // larger than freed slot
    // p2 should be a fresh allocation, not p1
    EXPECT_NOT_NULL(p1);

    EXPECT(p2 != p1);
    EXPECT(p2 != NULL);
}

TEST(malloc_chunk_file_and_line_recorded) {
    reset_memory();
    MALLOC(8);
    bool ok = memory.chunks[0].file != NULL && memory.chunks[0].line > 0;
    EXPECT(ok);
}

TEST(malloc_sequential_large_then_small) {
    reset_memory();
    void* big   = MALLOC(1024);
    void* small = MALLOC(8);
    EXPECT_NOT_NULL(big);
    EXPECT(small != NULL);
    EXPECT(free_no_overlap());
}

TEST(malloc_all_bytes_writable) {
    reset_memory();
    size_t sz = 256;
    uint8_t* p = MALLOC(sz);
    bool ok = true;
    for (size_t i = 0; i < sz; i++) { p[i] = (uint8_t)i; }
    for (size_t i = 0; i < sz; i++) { if (p[i] != (uint8_t)i) ok = false; }
    EXPECT(ok);
}

// =============================================================================
// FREE
// =============================================================================

TEST(free___free_null_does_not_crash) {
    reset_memory();
    void* null_value = NULL;
    FREE(null_value);   // must not crash or assert
    EXPECT(true);
}

TEST(free___free_marks_chunk_as_free) {
    reset_memory();
    void* p = MALLOC(32);
    FREE(p);
    EXPECT_INT((int)heap_used_count(), 0);
}

TEST(free___used_count_decreases_after_free) {
    reset_memory();
    void* p1 = MALLOC(16);
    void* p2 = MALLOC(16);
    (void)p1;
    FREE(p2);
    EXPECT_INT((int)heap_used_count(), 1);
}

TEST(free___free_first_of_three) {
    reset_memory();
    void* p1 = MALLOC(23);
    void* p2 = MALLOC(23);
    void* p3 = MALLOC(23);
    (void)p2; (void)p3;
    FREE(p1);
    EXPECT_INT((int)heap_used_count(), 2);
    EXPECT_INT((int)heap_used_count(), 2);
}

TEST(free___free_middle_of_three) {
    reset_memory();
    void* p1 = MALLOC(23);
    void* p2 = MALLOC(23);
    void* p3 = MALLOC(23);
    (void)p1; (void)p3;
    FREE(p2);
    EXPECT_INT((int)heap_used_count(), 2);
}

TEST(free___free_last_of_three) {
    reset_memory();
    void* p1 = MALLOC(23);
    void* p2 = MALLOC(23);
    void* p3 = MALLOC(23);
    (void)p1; (void)p2;
    EXPECT_INT((int)heap_used_count(), 3);
    FREE(p3);
    EXPECT_INT((int)heap_used_count(), 2);
}

TEST(free___free_all_then_mallocsucceeds) {
    reset_memory();
    void* p1 = MALLOC(16);
    void* p2 = MALLOC(16);
    void* p3 = MALLOC(16);
    FREE(p1);
    FREE(p2);
    FREE(p3);
    void* p4 = MALLOC(16);
    EXPECT(p4 != NULL);
}

TEST(free___freed_region_is_zeroed) {
    reset_memory();
    char* p = MALLOC(32);
    memset(p, 0xFF, 32);
    FREE(p);
    bool zeroed = true;
    for (size_t i = 0; i < 32; i++)
        if (p[i] != 0) { zeroed = false; break; }
    EXPECT(zeroed);
}

TEST(free___sequential_free_p2_then_p3) {
    reset_memory();
    char* p1 = MALLOC(23);
    char* p2 = MALLOC(23);
    char* p3 = MALLOC(23);
    (void)p1;
    EXPECT_INT((int)heap_used_count(), 3);
    FREE(p2);
    EXPECT_INT((int)heap_used_count(), 2);
    FREE(p3);   // must not fail — core bug from original implementation
    EXPECT_INT((int)heap_used_count(), 1);
}

TEST(free___no_overlap_after_mixed_free) {
    reset_memory();
    void* p1 = MALLOC(32);
    void* p2 = MALLOC(64);
    void* p3 = MALLOC(32);
    (void)p1; (void)p3;
    FREE(p2);
    EXPECT(free_no_overlap());
}

TEST(free___unknown_pointer_does_not_corrupt) {
    reset_memory();
    void* p1 = MALLOC(32);
    uint8_t*  garbage = NULL;
    FREE((void*)garbage);   // unknown ptr — should warn, not corrupt
    EXPECT(in_heap(p1));
    EXPECT_INT((int)heap_used_count(), 1);
}

TEST(free___double_free_detected) {
    reset_memory();
    void* p = MALLOC(32);
    EXPECT_INT(1, (int)heap_used_count());
    FREE(p);
    EXPECT_INT(0, (int)heap_used_count());
    FREE(p);   // double free — should warn, not change state
    EXPECT_INT(0, (int)heap_used_count());
}

TEST(free___mallocafter_free_sequence) {
    reset_memory();
    void* a = MALLOC(16);
    void* b = MALLOC(16);
    void* c = MALLOC(16);
    FREE(a);
    FREE(b);
    FREE(c);
    void* d = MALLOC(16);
    void* e = MALLOC(16);
    EXPECT(d != NULL);
    EXPECT(e != NULL);
    EXPECT(free_no_overlap());
}

TEST(free___data_in_other_chunks_intact_after_free) {
    reset_memory();
    char* a = MALLOC(32);
    char* b = MALLOC(32);
    char* c = MALLOC(32);
    memset(a, 'A', 32);
    memset(b, 'B', 32);
    memset(c, 'C', 32);
    FREE(b);
    bool ok = true;
    for (int i = 0; i < 32; i++) if (a[i] != 'A') ok = false;
    for (int i = 0; i < 32; i++) if (c[i] != 'C') ok = false;
    EXPECT(ok);
}


// =============================================================================
// CALLOC
// =============================================================================

TEST(calloc_returns_non_null_for_valid_args) {
    reset_memory();
    void* p = CALLOC(4, 8);
    EXPECT(p != NULL);
}

TEST(calloc_returns_null_for_zero_nmemb) {
    reset_memory();
    void* p = CALLOC(0, 8);
    EXPECT_NULL(p);
}

TEST(calloc_returns_null_for_zero_size) {
    reset_memory();
    void* p = CALLOC(4, 0);
    EXPECT_NULL(p);
}

TEST(calloc_memory_is_zeroed) {
    reset_memory();
    uint8_t* p = CALLOC(1, 64);
    bool zeroed = true;
    for (size_t i = 0; i < 64; i++) if (p[i] != 0) { zeroed = false; break; }
    EXPECT(zeroed);
}

TEST(calloc_pointer_inside_heap) {
    reset_memory();
    void* p = CALLOC(8, 4);
    EXPECT(in_heap(p));
}

TEST(calloc_multiple_callocs_no_overlap) {
    reset_memory();
    CALLOC(4, 16);
    CALLOC(4, 16);
    CALLOC(4, 16);
    EXPECT(free_no_overlap());
}

TEST(calloc_large_calloc) {
    reset_memory();
    void* p = CALLOC(1, MEMORY_SIZE / 2);
    EXPECT(p != NULL);
    EXPECT(in_heap(p));
}

TEST(calloc_overflow_protection) {
    reset_memory();
    // nmemb * size overflows size_t
    void* p = CALLOC(SIZE_MAX, 2);
    EXPECT_NULL(p);
}

TEST(calloc_returns_null_on_oom) {
    reset_memory();
    CALLOC(1, MEMORY_SIZE);
    void* p = CALLOC(1, 1);
    EXPECT_NULL(p);
}

TEST(calloc_data_written_survives) {
    reset_memory();
    int* arr = CALLOC(4, sizeof(int));
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40;
    EXPECT_INT(arr[0], 10);
    EXPECT_INT(arr[3], 40);
}

TEST(calloc_chunk_count_increments) {
    reset_memory();
    CALLOC(1, 8);
    CALLOC(1, 8);
    EXPECT_INT((int)heap_used_count(), 2);
}

TEST(calloc_zeroed_after_previous_dirty_free) {
    reset_memory();
    uint8_t* p1 = MALLOC(32);
    memset(p1, 0xFF, 32);
    FREE(p1);
    uint8_t* p2 = CALLOC(1, 32);   // reuses p1 slot
    bool zeroed = true;
    for (size_t i = 0; i < 32; i++) if (p2[i] != 0) { zeroed = false; break; }
    EXPECT(zeroed);
}

// =============================================================================
// REALLOC
// =============================================================================

TEST(realloc_null_ptr_behaves_as_malloc) {
    reset_memory();
    void* p = REALLOC(NULL, 32);
    EXPECT(p != NULL);
    EXPECT(in_heap(p));
}

TEST(realloc_zero_size_frees_ptr) {
    reset_memory();
    void* p = MALLOC(32);
    void* r = REALLOC(p, 0);
    EXPECT_NULL(r);
    EXPECT_INT((int)heap_used_count(), 0);
}

TEST(realloc_shrink_in_place) {
    reset_memory();
    void* p  = MALLOC(64);
    void* p2 = REALLOC(p, 32);
    EXPECT(p == p2);   // same address — shrank in place
    EXPECT_INT((int)heap_used_count(), 1);
}

TEST(realloc_shrink_zeroes_tail) {
    reset_memory();
    uint8_t* p = MALLOC(64);
    memset(p, 0xFF, 64);
    (void)REALLOC(p, 32);
    bool tail_zeroed = true;
    for (size_t i = 32; i < 64; i++)
        if (p[i] != 0) { tail_zeroed = false; break; }
    EXPECT(tail_zeroed);
}

TEST(realloc_grow_returns_new_ptr) {
    reset_memory();
    void* p1 = MALLOC(32);
    void* p2 = REALLOC(p1, 64);
    EXPECT(p2 != NULL);
    EXPECT(in_heap(p2));
}

TEST(realloc_grow_preserves_data) {
    reset_memory();
    char* p = MALLOC(16);
    memcpy(p, "hello world!!", 14);
    char* r = REALLOC(p, 64);
    EXPECT_STR(r, "hello world!!");
}

TEST(realloc_grow_frees_old_slot) {
    reset_memory();
    void* p1 = MALLOC(16);
    (void)REALLOC(p1, 64);
    // old slot should be freed — free_count includes it
    EXPECT_INT((int)heap_free_count(), 1);
}

TEST(realloc_no_overlap_after_grow) {
    reset_memory();
    void* p1 = MALLOC(16);
    void* p2 = MALLOC(16);
    (void)p2;
    (void)REALLOC(p1, 64);
    EXPECT(free_no_overlap());
}

TEST(realloc_same_size_is_noop) {
    reset_memory();
    void* p1 = MALLOC(32);
    void* p2 = REALLOC(p1, 32);
    EXPECT(p1 == p2);
}

TEST(realloc_unknown_ptr_returns_null) {
    reset_memory();
    uint8_t* garbage = NULL;
    void* r = REALLOC(garbage, 64);
    EXPECT_NULL(r);
}

TEST(realloc_returns_null_on_oom) {
    reset_memory();
    void* p = MALLOC(16);
    MALLOC(MEMORY_SIZE - 16);  // fill rest of heap
    void* r = REALLOC(p, 64);
    EXPECT_NULL(r);
}

TEST(realloc_chunk_count_stable_on_shrink) {
    reset_memory();
    MALLOC(64);
    size_t before = memory.count;
    void* p = MALLOC(64);
    (void)REALLOC(p, 32);
    EXPECT_INT((int)memory.count, (int)before + 1);
}

// =============================================================================
// REALLOCARRAY
// =============================================================================

TEST(reallocarray_null_ptr_behaves_as_malloc) {
    reset_memory();
    void* p = REALLOCARRAY(NULL, 4, 8);
    EXPECT(p != NULL);
    EXPECT(in_heap(p));
}

TEST(reallocarray_zero_nmemb_frees_ptr) {
    reset_memory();
    void* p = MALLOC(32);
    void* r = REALLOCARRAY(p, 0, 8);
    EXPECT_NULL(r);
    EXPECT_INT((int)heap_used_count(), 0);
}

TEST(reallocarray_zero_size_frees_ptr) {
    reset_memory();
    void* p = MALLOC(32);
    void* r = REALLOCARRAY(p, 4, 0);
    EXPECT_NULL(r);
}

TEST(reallocarray_overflow_protection) {
    reset_memory();
    void* p = MALLOC(32);
    void* r = REALLOCARRAY(p, SIZE_MAX, 2);
    EXPECT_NULL(r);
}

TEST(reallocarray_grow_preserves_data) {
    reset_memory();
    int* arr = MALLOC(4 * sizeof(int));
    arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4;
    int* r = REALLOCARRAY(arr, 8, sizeof(int));
    EXPECT_INT(r[0], 1);
    EXPECT_INT(r[3], 4);
}

TEST(reallocarray_shrink_in_place) {
    reset_memory();
    void* p = MALLOC(8 * sizeof(int));
    void* r = REALLOCARRAY(p, 4, sizeof(int));
    EXPECT(p == r);
}

TEST(reallocarray_no_overlap_after_grow) {
    reset_memory();
    void* p1 = MALLOC(16);
    void* p2 = MALLOC(16);
    (void)p2;
    (void)REALLOCARRAY(p1, 8, 16);
    EXPECT(free_no_overlap());
}

// =============================================================================
// MEMMOVE
// =============================================================================

TEST(memmove_forward_copy_no_overlap) {
    reset_memory();
    char* buf = MALLOC(64);
    memcpy(buf, "hello", 6);
    MEMMOVE(buf + 10, buf, 6);
    EXPECT_STR(buf + 10, "hello");
}

TEST(memmove_backward_copy_overlap) {
    reset_memory();
    char* buf = MALLOC(64);
    memcpy(buf, "hello world", 12);
    // shift "world" two bytes right — overlapping
    MEMMOVE(buf + 8, buf + 6, 5);
    EXPECT(strncmp(buf + 8, "world", 5) == 0);
}

TEST(memmove_forward_overlap) {
    reset_memory();
    char* buf = MALLOC(64);
    memcpy(buf, "abcdefgh", 9);
    // shift left by 2 — overlapping forward
    MEMMOVE(buf, buf + 2, 6);
    EXPECT(strncmp(buf, "cdefgh", 6) == 0);
}

TEST(memmove_null_dest_does_not_crash) {
    reset_memory();
    char* src = MALLOC(16);
    void* a = NULL;
    MEMMOVE(a, src, 8);  // should warn, not crash
    EXPECT(true);
}

TEST(memmove_null_src_does_not_crash) {
    reset_memory();
    char* dest = MALLOC(16);
    void* a = NULL;
    MEMMOVE(dest, a, 8);  // should warn, not crash
    EXPECT(true);
}

TEST(memmove_zero_size_is_noop) {
    reset_memory();
    char* buf = MALLOC(32);
    memcpy(buf, "hello", 6);
    MEMMOVE(buf + 10, buf, 0);
    EXPECT(buf[10] == 0);   // nothing written
}

TEST(memmove_dest_outside_heap_rejected) {
    reset_memory();
    char  stack_buf[32] = {0};
    char* src = MALLOC(16);
    MEMMOVE(stack_buf, src, 16);  // dest not in heap — should warn
    EXPECT(true);
}

TEST(memmove_src_outside_heap_rejected) {
    reset_memory();
    char  stack_buf[32] = "hello";
    char* dest = MALLOC(32);
    MEMMOVE(dest, stack_buf, 16);  // src not in heap — should warn
    EXPECT(true);
}

TEST(memmove_dest_in_freed_chunk_rejected) {
    reset_memory();
    char* p1 = MALLOC(32);
    char* p2 = MALLOC(32);
    FREE(p1);
    MEMMOVE(p1, p2, 16);  // dest is freed — should warn
    EXPECT(true);
}

TEST(memmove_src_equals_dest_is_noop) {
    reset_memory();
    char* buf = MALLOC(32);
    memcpy(buf, "hello", 6);
    MEMMOVE(buf, buf, 6);
    EXPECT_STR(buf, "hello");
    FREE(buf);
}

TEST(memmove_large_move) {
    reset_memory();
    size_t sz  = 1024;
    uint8_t* a = MALLOC(sz * 2);
    for (size_t i = 0; i < sz; i++) a[i] = (uint8_t)i;
    MEMMOVE(a + sz, a, sz);
    bool ok = true;
    for (size_t i = 0; i < sz; i++) {
        if (a[sz + i] != (uint8_t)i) { ok = false; break; }
    }
    EXPECT(ok);
    FREE(a);
}


// =============================================================================
// runner
// =============================================================================

int test_heap(void) {
    printf("MALLOC / FREE tests");
    printf("\n===================");

    _heap_wrn = false;

    // malloc
    RUN(malloc_returns_non_null_for_valid_size);
    RUN(malloc_returns_null_for_zero_size);
    RUN(malloc_pointer_is_inside_heap);
    RUN(malloc_multiple_pointers_are_distinct);
    RUN(malloc_multiple_pointers_no_overlap);
    RUN(malloc_pointers_are_contiguous_on_fresh_heap);
    RUN(malloc_chunk_count_increments);
    RUN(malloc_written_data_survives);
    RUN(malloc_large_allocation);
    RUN(malloc_fills_heap_exactly);
    RUN(malloc_returns_null_when_oom);
    RUN(malloc_returns_null_when_chunks_full);
    RUN(malloc_independent_regions_dont_corrupt);
    RUN(malloc_size_1_allocation);
    RUN(malloc_reuses_freed_slot);
    RUN(malloc_reuses_freed_slot_smaller_size);
    RUN(malloc_does_not_reuse_slot_too_small);
    RUN(malloc_chunk_file_and_line_recorded);
    RUN(malloc_sequential_large_then_small);
    RUN(malloc_all_bytes_writable);

    // free
    RUN(free___free_null_does_not_crash);
    RUN(free___free_marks_chunk_as_free);
    RUN(free___used_count_decreases_after_free);
    RUN(free___free_first_of_three);
    RUN(free___free_middle_of_three);
    RUN(free___free_last_of_three);
    RUN(free___free_all_then_mallocsucceeds);
    RUN(free___freed_region_is_zeroed);
    RUN(free___sequential_free_p2_then_p3);
    RUN(free___no_overlap_after_mixed_free);
    RUN(free___unknown_pointer_does_not_corrupt);
    RUN(free___double_free_detected);
    RUN(free___mallocafter_free_sequence);
    RUN(free___data_in_other_chunks_intact_after_free);

    printf("CALLOC / REALLOC / REALLOCARRAY / MEMMOVE tests");
    printf("\n================================================");

    // calloc
    RUN(calloc_returns_non_null_for_valid_args);
    RUN(calloc_returns_null_for_zero_nmemb);
    RUN(calloc_returns_null_for_zero_size);
    RUN(calloc_memory_is_zeroed);
    RUN(calloc_pointer_inside_heap);
    RUN(calloc_multiple_callocs_no_overlap);
    RUN(calloc_large_calloc);
    RUN(calloc_overflow_protection);
    RUN(calloc_returns_null_on_oom);
    RUN(calloc_data_written_survives);
    RUN(calloc_chunk_count_increments);
    RUN(calloc_zeroed_after_previous_dirty_free);

    // realloc
    RUN(realloc_null_ptr_behaves_as_malloc);
    RUN(realloc_zero_size_frees_ptr);
    RUN(realloc_shrink_in_place);
    RUN(realloc_shrink_zeroes_tail);
    RUN(realloc_grow_returns_new_ptr);
    RUN(realloc_grow_preserves_data);
    RUN(realloc_grow_frees_old_slot);
    RUN(realloc_no_overlap_after_grow);
    RUN(realloc_same_size_is_noop);
    RUN(realloc_unknown_ptr_returns_null);
    RUN(realloc_returns_null_on_oom);
    RUN(realloc_chunk_count_stable_on_shrink);

    // reallocarray
    RUN(reallocarray_null_ptr_behaves_as_malloc);
    RUN(reallocarray_zero_nmemb_frees_ptr);
    RUN(reallocarray_zero_size_frees_ptr);
    RUN(reallocarray_overflow_protection);
    RUN(reallocarray_grow_preserves_data);
    RUN(reallocarray_shrink_in_place);
    RUN(reallocarray_no_overlap_after_grow);

    // memmove
    RUN(memmove_forward_copy_no_overlap);
    RUN(memmove_backward_copy_overlap);
    RUN(memmove_forward_overlap);
    RUN(memmove_null_dest_does_not_crash);
    RUN(memmove_null_src_does_not_crash);
    RUN(memmove_zero_size_is_noop);
    RUN(memmove_dest_outside_heap_rejected);
    RUN(memmove_src_outside_heap_rejected);
    RUN(memmove_dest_in_freed_chunk_rejected);
    RUN(memmove_src_equals_dest_is_noop);
    RUN(memmove_large_move);

    _heap_wrn = true;

    printf("\n\n%d passed, %d failed\n", passed, failed);
    return failed;
}