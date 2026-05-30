#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "io.h"
#include "collections.h"
#include "tests.h"

TEST(t_sb_sv__init) {
    StringBuilder sb;
    sb_init(&sb);
    EXPECT_NOT_NULL(sb.items);
    EXPECT(sb.len      == 0);
    EXPECT(sb.capacity == SB_INIT_CAP);
    sb_free(&sb);
}

TEST(t_sb_sv__append_plain) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpush(&sb, "hello %d",1);
    EXPECT_INT((int)sb.len, 1);
    EXPECT_STR(sb.items[0], "hello 1");
    sb_free(&sb);
}

TEST(t_sb_sv__append_format) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpush(&sb, "%s = %d", "answer", 42);
    EXPECT_STR(sb.items[0], "answer = 42");
    sb_free(&sb);
}

TEST(t_sb_sv__append_float) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpush(&sb, "pi = %.2f", 3.14159);
    EXPECT_STR(sb.items[0], "pi = 3.14");
    sb_free(&sb);
}

TEST(t_sb_sv__appendln_adds_newline) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpushln(&sb, "line %d",0);
    EXPECT_STR(sb.items[0], "line 0\n");
    sb_free(&sb);
}

TEST(t_sb_sv__appendln_format) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpushln(&sb, "x = %d", 7);
    EXPECT_STR(sb.items[0], "x = 7\n");
    sb_free(&sb);
}

TEST(t_sb_sv__multiple_appends) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "foo");
    sb_push(&sb, "bar");
    sb_push(&sb, "baz");
    EXPECT_INT((int)sb.len, 3);
    EXPECT_STR(sb.items[0], "foo");
    EXPECT_STR(sb.items[1], "bar");
    EXPECT_STR(sb.items[2], "baz");
    sb_free(&sb);
}

TEST(t_sb_sv__to_sv_joins_fragments) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "hello");
    sb_push(&sb, ", ");
    sb_push(&sb, "world");

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_STR(sv.buffer, "hello, world");
    EXPECT_INT((int)sv.len, 12);

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__to_sv_clears_sb) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "data");

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_INT((int)sb.len, 0);

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__to_sv_empty_sb) {
    StringBuilder sb;
    sb_init(&sb);

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_STR(sv.buffer, "");
    EXPECT_INT((int)sv.len, 0);

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__to_sv_with_newlines) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpushln(&sb, "line%d",1);
    sb_fpushln(&sb, "line%d",2);

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_STR(sv.buffer, "line1\nline2\n");

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__set_length_truncates_mid_chunk) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "hello");
    sb_push(&sb, "world");
    sb_set_length(&sb, 7);   // "hellowo"

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_STR(sv.buffer, "hellowo");
    EXPECT_INT((int)sv.len, 7);

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__set_length_drops_trailing_chunks) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "abcde");
    sb_push(&sb, "fghij");
    sb_push(&sb, "klmno");
    sb_set_length(&sb, 5);
    EXPECT_INT((int)sb.len, 1);  // only first chunk survives

    StringView sv;
    sb_to_sv_and_clear_sb(&sb, &sv);
    EXPECT_STR(sv.buffer, "abcde");

    sv_free(&sv);
    sb_free(&sb);
}

TEST(t_sb_sv__set_length_noop_when_longer) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "hi");
    sb_set_length(&sb, 999);   // longer than content: no-op
    EXPECT_INT((int)sb.len, 1);
    EXPECT_STR(sb.items[0], "hi");
    sb_free(&sb);
}

TEST(t_sb_sv__grow_beyond_init_cap) {
    StringBuilder sb;
    sb_init(&sb);
    // push 2x initial capacity to force at least one realloc
    for (int i = 0; i < SB_INIT_CAP * 2; i++) {
        sb_fpush(&sb, "%d", i);
    }
    EXPECT_INT((int)sb.len, (SB_INIT_CAP * 2));
    EXPECT(sb.capacity >= (SB_INIT_CAP * 2));

    // spot-check a few values
    EXPECT_STR(sb.items[0],             "0");
    EXPECT_STR(sb.items[SB_INIT_CAP],  "16");
    sb_free(&sb);
}

TEST(t_sb_sv__free_reuse) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "first");
    sb_free(&sb);
    sb_init(&sb);
    sb_push(&sb, "second");
    EXPECT_INT((int)sb.len, 1);
    EXPECT_STR(sb.items[0], "second");
    sb_free(&sb);
}

TEST(t_sb_sv__append_empty_string) {
    StringBuilder sb;
    sb_init(&sb);
    sb_fpush(&sb, "%s", "");
    EXPECT_INT((int)sb.len, 1);
    EXPECT_STR(sb.items[0], "");
    sb_free(&sb);
}

TEST(t_sb_sv__to_sv_reusable_after_clear) {
    StringBuilder sb;
    sb_init(&sb);
    sb_push(&sb, "round1");
    StringView sv1;
    sb_to_sv_and_clear_sb(&sb, &sv1);

    sb_push(&sb, "round2");
    StringView sv2;
    sb_to_sv_and_clear_sb(&sb, &sv2);

    EXPECT_STR(sv1.buffer, "round1");
    EXPECT_STR(sv2.buffer, "round2");

    FREE(sv1.buffer);
    FREE(sv2.buffer);
    sb_free(&sb);
}

int test_sb_sv(void) {
    printf("StringBuilder tests");
    printf("\n===================");

    reset_memory();

    RUN(t_sb_sv__init);
    RUN(t_sb_sv__append_plain);
    RUN(t_sb_sv__append_format);
    RUN(t_sb_sv__append_float);
    RUN(t_sb_sv__appendln_adds_newline);
    RUN(t_sb_sv__appendln_format);
    RUN(t_sb_sv__multiple_appends);
    RUN(t_sb_sv__to_sv_joins_fragments);
    RUN(t_sb_sv__to_sv_clears_sb);
    RUN(t_sb_sv__to_sv_empty_sb);
    RUN(t_sb_sv__to_sv_with_newlines);
    RUN(t_sb_sv__set_length_truncates_mid_chunk);
    RUN(t_sb_sv__set_length_drops_trailing_chunks);
    RUN(t_sb_sv__set_length_noop_when_longer);
    RUN(t_sb_sv__grow_beyond_init_cap);
    RUN(t_sb_sv__free_reuse);
    RUN(t_sb_sv__append_empty_string);
    RUN(t_sb_sv__to_sv_reusable_after_clear);

    printf("\n=========================================================");
    return failed ? 1 : 0;    
}
