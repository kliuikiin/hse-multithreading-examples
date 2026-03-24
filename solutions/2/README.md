# Сборка и запуск тестов

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/apply_function_test
```

# Результат

```
[==========] Running 4 tests from 1 test suite.
[----------] 4 tests from ApplyFunction
[ RUN      ] ApplyFunction.EmptyVector
[       OK ] ApplyFunction.EmptyVector (0 ms)
[ RUN      ] ApplyFunction.SingleThread
[       OK ] ApplyFunction.SingleThread (0 ms)
[ RUN      ] ApplyFunction.MultiThread
[       OK ] ApplyFunction.MultiThread (0 ms)
[ RUN      ] ApplyFunction.ThreadCountExceedsSize
[       OK ] ApplyFunction.ThreadCountExceedsSize (0 ms)
[----------] 4 tests from ApplyFunction (0 ms total)
[  PASSED  ] 4 tests.
```