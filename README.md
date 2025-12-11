# benchmark

C++23 clock wrapper for easy runtime measurement. Not thread-safe.

## Usage

#### Basic benchmarking

```cpp
ucpp::benchmark bm{};
bm.start();
// some code...
bm.end();
std::println("Runtime: {}", bm.runtime_ms());
```

or...

```cpp
ucpp::benchmark bm{ ucpp::auto_start };
// some code...
bm.end();
std::println("Runtime: {}", bm.runtime_ms());
```

It would be pointless to write snippets for each utility method in the class, so i'm just going to list all of them at the end of the README.

#### Scoped benchmark

Scoped benchmarks start automatically and end once they go out of scope, invoking the specified callback.

```cpp
{
    ucpp::scoped_benchmark bm{ [](const ucpp::benchmark& b) {
        std::println("Runtime: {}", b.runtime_us());
    } };
    // some code...
    // 'bm' dies here and calls the function we passed it
}
```

#### Scoped halt

Once **ucpp::scoped_halt** object goes out of scope, the benchmark is resumed.

```cpp
{
    ucpp::scoped_halt halt{ bm };
    // some code...
}
```

#### Function benchmarking

```cpp
// int some_fn(int, int);
auto [ret, bm] = ucpp::benchmark::run(some_fn, 6, 7);
// 'bm.runtime_us()' is same as 'bm.runtime<std::chrono::microseconds>()'
std::println("Returned: {}, Runtime: {}", ret, bm.runtime_us());
```

Because the passed lambda returns **void**, **ucpp::benchmark::run()** returns only **ucpp::benchmark**.

```cpp
std::println("{}", ucpp::benchmark::run([]() {
    std::println("Hello World");
    return;
}).runtime_ms());
```

#### Resetting the benchmark

Resetting a benchmark effectively just re-constructs the object.

```cpp
ucpp::benchmark bm{ ucpp::auto_start };
// some code...
bm.reset();
// 'bm' is now in the same state it was in line 1
bm.start();
```

You can also use **ucpp::benchmark::reset()** with **ucpp::auto_start**.

```cpp
bm.reset(ucpp::auto_start);
```

You can't reset a **ucpp::scoped_benchmark** because i think it defeats the point.

#### Utility

-   **ucpp::benchmark::has_started()** <br/> _Until 'ucpp::benchmark::start()' is called, the benchmark remains in an unstarted ('idle') state. Resetting the benchmark also puts it in that state._
-   **ucpp::benchmark::has_ended()** <br/> _Self-explanatory._
-   **ucpp::benchmark::is_halted()** <br/> _True if the benchmark was started, wasn't ended and is halted._
-   **ucpp::benchmark::is_running()** <br/> _True if the benchmark was started, wasn't ended and is not halted._
-   **ucpp::benchmark::halt_time\<T>()** <br/> _Total halt time, if the benchmark is currently halted, that's also accounted for. 'T' is a duration._
-   **ucpp::benchmark::runtime\<T>()** <br/> _Total runtime without time spent halted. 'T' is a duration._
-   **ucpp::benchmark::runtime_us()** <br/> _Same as ucpp::benchmark::runtime\<std::chrono::microseconds>()_
-   **ucpp::benchmark::runtime_ms()** <br/> _Same as ucpp::benchmark::runtime\<std::chrono::milliseconds>()_
-   **ucpp::benchmark::start_timestamp()** <br/> _Returns 'std::nullopt', if benchmark has not yet started._
-   **ucpp::benchmark::end_timestamp()** <br/> _Returns 'std::nullopt', if benchmark has not yet ended._
-   **ucpp::benchmark::halt_start_timestamp()** <br/> _If not currently halted, returns 'std::nullopt'._
-   **ucpp::benchmark::state()** <br/> _Enum: 'idle' (unstarted), 'halted', 'running' or 'ended'._
