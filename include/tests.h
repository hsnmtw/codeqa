#ifndef TESTS_H_
#define TESTS_H_

    static int passed = 0;
    static int failed = 0;

    // #define TEST_ANSI_RST() do { passed = 0; failed = 0;  } while(0)

    #define TEST(t_unit) static void test_##t_unit(void)
    #define RUN(t_unit)  do { printf("\n  [%-45s] ", #t_unit); test_##t_unit(); } while(0)

    #define EXPECT(cond)  __extension__ ({                                      \
        bool _passed = true;                                                     \
        if (cond) { printf(ANSI_INF" ✓ "ANSI_RST" "); { passed++; }             \
        } else    { printf(ANSI_ERR" FAIL "ANSI_RST" (%s:%d)", __FILE__, __LINE__);\
                    failed++; _passed = false; }                                 \
        _passed;                                                                 \
    })

    // #define EXPECT_(e,a) __extension__ ({                                                     \
    //     if (e == a) { printf(ANSI_INF" PASS "ANSI_RST" "); { passed++; true; }                    \
    //     } else    { printf(ANSI_ERR" FAIL (expected: %zu, actual: %zu) "ANSI_RST" (%s:%d)\n",e,a, __FILE__, __LINE__);\
    //                 failed++; false; }                                                   \
    // })

    #define _EXPECT_STR(a, b)   EXPECT((a) && (b) && strcmp((a),(b)) == 0)
    #define _EXPECT_NULL(p)     EXPECT((p) == NULL)
    #define _EXPECT_NOT_NULL(p) EXPECT((p) != NULL)
    #define _EXPECT_INT(a, b)   EXPECT((a) == (b))
    #define EXPECT_STR(a, b)    __extension__({ bool _pass = true; if(!_EXPECT_STR(a, b))     { _pass = false; wrn("Expected `%s` but got `%s`", a, b); } _pass; })
    #define EXPECT_NULL(p)      __extension__({ bool _pass = true; if(!_EXPECT_NULL(p)  )     { _pass = false; wrn("Expected NULL but got `%p`", p   ); } _pass; })
    #define EXPECT_NOT_NULL(p)  __extension__({ bool _pass = true; if(!_EXPECT_NOT_NULL(p)  ) { _pass = false; wrn("Expected NOT NULL but got NULL");   } _pass; })
    #define EXPECT_INT(a, b)    __extension__({ bool _pass = true; if(!_EXPECT_INT(a, b))     { _pass = false; wrn("Expected `%d` but got `%d`", a, b); } _pass; })

    // bool EXPECT(bool cond) {
    //     bool passed = true;
    //     if (cond) { printf(ANSI_INF" PASS "ANSI_RST" "); { passed++; }
    //     } else    { printf(ANSI_ERR" FAIL "ANSI_RST" (%s:%d)", __FILE__, __LINE__);
    //                 failed++; passed = false; }
    //     return passed;
    // }



#endif//TESTS_H_
