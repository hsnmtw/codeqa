# codeqa

A moderate attempt to analyse source code and look for
potential code smells and detect areas of improvements

the tool expects the target language to be of the C 
programming family style. The targeted languages include
so far:
    1. CSharp [C#]
    3. Javascript

```bash
    # --------------------------------------------------------
    # CODE/QA
    # --------------------------------------------------------
    # usage: scan a file or a directory (recursively if needed)
    $ ./codeqa --f=[path: FILE_FULL_PATH]
    # or
    $ ./codeqa --d=[path: DIRECTORY_FULL_PATH] --p=[pattern: *.ext] --r=[is_recurive: true/false] 
```