| Phase       | Java (ms) | C++ (ms) | Winner              | Why?                                                                                  |
|-------------|-----------|----------|---------------------|---------------------------------------------------------------------------------------|
| Monomorphic | 2362      | 3482     | Java (~1.5× faster) | Inlining. Java eliminated the function call; C++ performed a virtual lookup and call. |
| Bimorphic   | 6499      | 7390     | Java (slight edge)  | Inline cache enabled efficient dispatch based on observed types.                      |
| Polymorphic | 13,450    | 9,556    | C++ (~1.5× faster)  | Java fell back to a slower generic lookup, while C++ remained consistent.             |
