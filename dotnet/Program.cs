using System;
using System.Linq;
using System.IO;
using System.Collections;
using System.Diagnostics;

Dictionary<string,int> map = [];
long mss = 0;
int its = 33;
for(int it=0;it<its;++it){
    using (var fs = File.OpenRead("../data/other.txt")) {
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

    System.Console.WriteLine("Finished in {0} ms", mss/its);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","in" ,  1247*its == map["in" ] ? "PASS" : "FAIL", map["in" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","The",  1155*its == map["The"] ? "PASS" : "FAIL", map["The"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","a"  ,  1980*its == map["a"  ] ? "PASS" : "FAIL", map["a"  ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","to" ,  3319*its == map["to" ] ? "PASS" : "FAIL", map["to" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","and",  2078*its == map["and"] ? "PASS" : "FAIL", map["and"]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","of" ,  2374*its == map["of" ] ? "PASS" : "FAIL", map["of" ]);
    Console.WriteLine("Testing for word {0,-6} = {1} : {2}","the",  6860*its == map["the"] ? "PASS" : "FAIL", map["the"]);
