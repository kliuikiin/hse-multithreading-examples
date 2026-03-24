# Сборка и запуск тестов

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/buffered_channel_test
```

# Результат
```
[==========] Running 6 tests from 1 test suite.
[----------] 6 tests from Correctness
[ RUN      ] Correctness.Simple
[       OK ] Correctness.Simple (305 ms)
[ RUN      ] Correctness.Senders
[       OK ] Correctness.Senders (308 ms)
[ RUN      ] Correctness.Receivers
[       OK ] Correctness.Receivers (306 ms)
[ RUN      ] Correctness.SmallBuf
[       OK ] Correctness.SmallBuf (307 ms)
[ RUN      ] Correctness.BigBuf
[       OK ] Correctness.BigBuf (343 ms)
[ RUN      ] Correctness.Random
[       OK ] Correctness.Random (304 ms)
[----------] 6 tests from Correctness (1877 ms total)
[  PASSED  ] 6 tests.
```
