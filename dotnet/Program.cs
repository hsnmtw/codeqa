using System;
using System.Linq;
using System.IO;
using System.Collections;
using System.Diagnostics;

Dictionary<string,int> map = [];
long mss = 0;
int its = 31;
for(int it=0;it<its;++it){
    using (var fs = File.OpenRead("../data/t8.shakespeare.txt")) {
        Stopwatch sw = Stopwatch.StartNew();
        byte[] buffer = [0];
        char[] word = new char[256];
        int p=0;
        while(fs.Read(buffer,0,1) > 0) {
            char c = (char)buffer[0];
            if (char.IsWhiteSpace(c)) {
                if (p>0) {
                    string key = new(word[..p]);
                    map.TryGetValue(key, out int count);
                    map[key] = count + 1;
                    p=0;
                }            
                continue;
            }
            if (p<256) {
                word[p++] = c;
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

    System.Console.WriteLine("Finished in {0} ms", mss/its);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","in" ,  9576*its == map["in" ] ? "PASS" : "FAIL", map["in" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","The",  3965*its == map["The"] ? "PASS" : "FAIL", map["The"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","a"  ,  12532*its == map["a"  ] ? "PASS" : "FAIL", map["a"  ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","to" ,  15623*its == map["to" ] ? "PASS" : "FAIL", map["to" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","and",  18297*its == map["and"] ? "PASS" : "FAIL", map["and"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","of" ,  15544*its == map["of" ] ? "PASS" : "FAIL", map["of" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","the",  23242*its == map["the"] ? "PASS" : "FAIL", map["the"]);
