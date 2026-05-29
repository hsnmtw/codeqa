using System;
using System.Linq;
using System.IO;
using System.Collections;
using System.Diagnostics;

Dictionary<string,int> map = [];
long mss = 0;
int its = 3;
const int BUF_SIZE = 224;
char[] word = new char[128];
byte[] buffer = new byte[BUF_SIZE];
for(int it=0;it<its;++it){
    using (var fs = File.OpenRead("../data/t8.shakespeare.txt")) {
        Stopwatch sw = Stopwatch.StartNew();
        int p=0;
        int r = 0;
        while((r = fs.Read(buffer,0,BUF_SIZE)) > 0) {
            for (int i=0;i<r;++i){
                char chr = (char)buffer[i];
                if (char.IsWhiteSpace(chr)) {
                    if (p>0) {
                        string key = new(word[..p]);
                        map.TryGetValue(key, out int count);
                        map[key] = count + 1;
                        p=0;
                    }            
                    continue;
                }
                if (p<256) {
                    word[p++] = chr;
                }
            }
        }
        sw.Stop();
        mss += sw.ElapsedMilliseconds;
        sw.Reset();
    }
}
/*
Testing for word in     = FAIL : 9576
Testing for word The    = FAIL : 3965
Testing for word a      = FAIL : 12532
Testing for word to     = FAIL : 15623
Testing for word and    = FAIL : 18297
Testing for word of     = FAIL : 15544
Testing for word the    = FAIL : 23242
*/

    System.Console.WriteLine("Finished in {0} ms [{1}]", mss/its, mss/1000.0);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","in" ,  9576*its == map["in" ] ? "PASS" : "FAIL", map["in" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","The",  3965*its == map["The"] ? "PASS" : "FAIL", map["The"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","a"  ,  12532*its == map["a"  ] ? "PASS" : "FAIL", map["a"  ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","to" ,  15623*its == map["to" ] ? "PASS" : "FAIL", map["to" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","it" ,  4912*its == map["it" ] ? "PASS" : "FAIL", map["it" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","and",  18297*its == map["and"] ? "PASS" : "FAIL", map["and"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","of" ,  15544*its == map["of" ] ? "PASS" : "FAIL", map["of" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","the",  23242*its == map["the"] ? "PASS" : "FAIL", map["the"]);
