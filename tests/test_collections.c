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


TEST(t_map__init) {
    Map m;
    map_init(&m);
    EXPECT(m.items != NULL && m.len == 0 && m.capacity == 11);
    map_free(&m);
}

TEST(t_map__set_and_get) {
    Map m;
    map_init(&m);
    char* val = "world";
    map_set(&m, "hello", val);
    EXPECT_STR((char*)map_get(&m, "hello"), "world");
    map_free(&m);
}

TEST(t_map__get_missing_key) {
    Map m;
    map_init(&m);
    EXPECT_NULL(map_get(&m, "ghost"));
    map_free(&m);
}

TEST(t_map__overwrite_key) {
    Map m;
    map_init(&m);
    int a = 1, b = 2;
    map_set(&m, "x", &a);
    map_set(&m, "x", &b);
    EXPECT_INT(*(int*)map_get(&m, "x"), 2);
    EXPECT_INT((int)m.len, 1);   // no duplicate
    map_free(&m);
}

TEST(t_map__multiple_keys) {
    Map m;
    map_init(&m);
    int vals[5] = {10, 20, 30, 40, 50};
    const char* keys[5] = {"alpha","beta","gamma","delta","epsilon"};
    for (int i = 0; i < 5; i++) map_set(&m, keys[i], &vals[i]);
    for (int i = 0; i < 5; i++)
        EXPECT_INT(*(int*)map_get(&m, keys[i]), vals[i]);
    map_free(&m);
}

TEST(t_map__len_tracking) {
    Map m;
    map_init(&m);
    int v = 0;
    map_set(&m, "a", &v);
    map_set(&m, "b", &v);
    map_set(&m, "a", &v);  // duplicate
    EXPECT_INT((int)m.len, 2);
    map_free(&m);
}

TEST(t_map__rehash_on_growth) {
    Map m;
    map_init(&m);
    char key[16];
    int vals[64];
    // insert enough to trigger at least one rehash (cap=11, load=0.65 → at 8)
    for (int i = 0; i < 64; i++) {
        vals[i] = i * 3;
        snprintf(key, sizeof(key), "key_%d", i);
        map_set(&m, key, &vals[i]);
    }
    // verify all values survived the rehash
    bool ok = true;
    for (int i = 0; i < 64; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        int* got = map_get(&m, key);
        if (!got || *got != i * 3) { ok = false; break; }
    }
    EXPECT(ok);
    map_free(&m);
}

TEST(t_map__empty_string_key) {
    Map m;
    map_init(&m);
    int v = 99;
    map_set(&m, "", &v);
    EXPECT_INT(*(int*)map_get(&m, ""), 99);
    map_free(&m);
}

TEST(t_map__long_key) {
    Map m;
    map_init(&m);
    char long_key[1024];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[1023] = '\0';
    int v = 42;
    map_set(&m, long_key, &v);
    EXPECT_INT(*(int*)map_get(&m, long_key), 42);
    map_free(&m);
}

TEST(t_map__collision_keys) {
    // keys designed to land on the same initial slot for cap=11
    Map m;
    map_init(&m);
    int a = 1, b = 2, c = 3;
    map_set(&m, "Aa", &a);
    map_set(&m, "BB", &b);  // "Aa" and "BB" have same ASCII sum
    map_set(&m, "Cc", &c);
    EXPECT_INT(*(int*)map_get(&m, "Aa"), 1);
    EXPECT_INT(*(int*)map_get(&m, "BB"), 2);
    EXPECT_INT(*(int*)map_get(&m, "Cc"), 3);
    map_free(&m);
}

TEST(t_map__null_value) {
    Map m;
    map_init(&m);
    map_set(&m, "nullkey", NULL);
    // key exists but value is NULL — get returns NULL either way,
    // so we verify via len
    EXPECT_INT((int)m.len, 1);
    map_free(&m);
}

TEST(t_map__free_reuse) {
    Map m;
    map_init(&m);
    int v = 7;
    map_set(&m, "temp", &v);
    map_free(&m);
    // re-init and confirm clean slate
    map_init(&m);
    EXPECT_INT((int)m.len, 0);
    EXPECT_NULL(map_get(&m, "temp"));
    map_free(&m);
}

TEST(t_map__read_entire_file) {
    StringView sv = {0};
    read_entire_file("./data/long-text.txt",&sv);
    
                    //tokenize the text into words
                    StringBuilder sb = {0};
                    Map map;
                    char word[256];
                    size_t word_len = 0;
                    map_init(&map);
                    EXPECT_NOT_NULL(&map);
                    for (size_t i = 0; i < sv.len; ++i) {
                        if (isspace((unsigned char)sv.buffer[i])) {
                            if (word_len > 0) {
                                word[word_len] = '\0';
                                size_t count = (size_t)map_get(&map, word);
                                map_set(&map, word, (void*)(count + 1));
                                word_len = 0;
                            }
                            continue;
                        }
                        if (word_len < sizeof(word) - 1) {
                            word[word_len++] = sv.buffer[i];
                        }
                    }
                    // flush last word if file doesn't end with whitespace
                    if (word_len > 0) {
                        word[word_len] = '\0';
                        size_t count = (size_t)map_get(&map, word);
                        map_set(&map, word, (void*)(count + 1));
                    }

                    DynamicArray keys = {0};
                    map_keys(&map,&keys);
                    // for(size_t i=0;i<keys.len;++i){
                    //     size_t count = (size_t) map_get(&map,keys.items[i]);
                    //     inf("key = [%s] = %zu", keys.items[i], count);
                    // }
                    EXPECT_NOT_NULL(&map);

                    EXPECT_INT(2, (int)map.len);
                    EXPECT_INT(2, (int)keys.len);
                    // printf("\n\n%p : '%s'\n\n", keys.items[0], keys.items[0]);
                    void* p = map_get(&map, keys.items[0]);
                    // printf("%zu\n", (size_t)p);
                    EXPECT_INT(5, (int)(uintptr_t)p);

                    sb_free(&sb);
                    map_free(&map);
                    da_free(&keys);
                    
    sv_free(&sv);
}

TEST(t_map__read_stream_file) {
    size_t mss = 0;
    const size_t iterations = 3;
    unsigned char word[128] = {0};
    size_t word_len = 0;
    const size_t BUF_SIZE = 224;
    unsigned char* buffer = CALLOC(BUF_SIZE, sizeof(char));
    
    Map map;
    map_init(&map);
    for(size_t it = 0; it<iterations; it++){
        StopWatch sw = {0};
        sw_start(&sw);
        
            FILE* f = fopen("./data/t8.shakespeare.txt","rb");
            if (!f) {
                EXPECT(false);
                continue;
            }

            word_len = 0;
            
            size_t r = 0;
            while((r = fread(buffer,1,BUF_SIZE,f)) > 0) {
                for(size_t i=0;i<r;++i){
                    unsigned char chr = buffer[i];
                    if (isspace(chr)) {
                        if (word_len > 0) {
                            word[word_len] = '\0';
                            size_t count = (size_t)map_get(&map, (const char*)word);
                            map_set(&map, (const char*)word, (void*)(count + 1));
                            word_len = 0;
                        }
                        continue;
                    }
                    if (word_len < sizeof(word) - 1) {
                        word[word_len++] = chr;
                    }
                }
            }
            // flush last word if file doesn't end with whitespace
            if (word_len > 0) {
                word[word_len] = '\0';
                size_t count = (size_t)map_get(&map, (const char*)word);
                map_set(&map, (const char*)word, (void*)(count + 1));
            }
        
        sw_stop(&sw);
        
        mss += sw.ms;
        sw.ms = 0;
        fclose(f);

    }

    FREE(buffer);

    DynamicArray keys = {0};
    map_keys(&map,&keys);
    // println("");
    // for(size_t i=0;i<keys.len;++i){
    //     size_t count = (size_t) map_get(&map,keys.items[i]);
    //     inf("%5zu  [%s]", count, keys.items[i]);
    // }
    // println("");

    // EXPECT_INT(2, map.len);
    // EXPECT_INT(2, keys.len);
    // EXPECT_INT(5, (size_t)map_get(&map, keys.items[0]));

    EXPECT(  4912*iterations == (size_t)map_get(&map, "it" ));
    EXPECT(  9576*iterations == (size_t)map_get(&map, "in" ));
    EXPECT(  3965*iterations == (size_t)map_get(&map, "The"));
    EXPECT( 12532*iterations == (size_t)map_get(&map, "a"  ));
    EXPECT( 15623*iterations == (size_t)map_get(&map, "to" ));
    EXPECT( 18297*iterations == (size_t)map_get(&map, "and"));
    EXPECT( 15544*iterations == (size_t)map_get(&map, "of" ));
    EXPECT( 23242*iterations == (size_t)map_get(&map, "the"));

    EXPECT( 195 > mss/iterations );

    println("\n\n[%zu] %f\n\n", mss/iterations, mss/1000.0 ); 

    map_free(&map);
    da_free(&keys);
}

// -- runner -------------------------------------------------------------------

int test_map(void) {
    puts("Map tests");
    puts("=========");

    RUN(t_map__init);
    RUN(t_map__set_and_get);
    RUN(t_map__get_missing_key);
    RUN(t_map__overwrite_key);
    RUN(t_map__multiple_keys);
    RUN(t_map__len_tracking);
    RUN(t_map__rehash_on_growth);
    RUN(t_map__empty_string_key);
    RUN(t_map__long_key);
    RUN(t_map__collision_keys);
    RUN(t_map__null_value);
    RUN(t_map__free_reuse);
    RUN(t_map__read_entire_file);
    RUN(t_map__read_stream_file);
    printf("\n=========================================================");
    return failed ? 1 : 0;
}
