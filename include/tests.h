#ifndef TESTS_H_
#define TESTS_H_

    static int passed = 0;
    static int failed = 0;

    // #define TEST_RESET() do { passed = 0; failed = 0;  } while(0)

    #define TEST(t_unit) static void test_##t_unit(void)
    #define RUN(t_unit)  do { printf("\n  [%-45s] ", #t_unit); test_##t_unit(); } while(0)

    #define EXPECT(cond) do {                                                     \
        if (cond) { printf(ANSI_INF" PASS "RESET" "); passed++;                    \
        } else    { printf(ANSI_ERR" FAIL "RESET" (%s:%d)", __FILE__, __LINE__);\
                    failed++; }                                                   \
    } while(0)

    #define EXPECT_STR(a, b) EXPECT((a) && (b) && strcmp((a),(b)) == 0)
    #define EXPECT_NULL(p)   EXPECT((p) == NULL)
    #define EXPECT_INT(a, b) EXPECT((a) == (b))
#endif//TESTS_H_
