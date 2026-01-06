| Strategy             | Time (ns/op) |
|----------------------|--------------|
| Java (TLAB enabled)  | 11.8         |
| C++ (standard `new`) | 17.61        |
| Java (TLAB disabled) | 53.04        |
| C++ (Object Pool)    | 2.73         |
