# ctest

`ctest` is a basic test framework for the C programming language.

An example of a simple test suite:
```C
# ./tests/test_example.c
#include <ctest/ctest.h>

#define SUITE_NAME test_example

CTEST(SUITE_NAME, test_one_equals_two) {
  ASSERT_INT_EQ(1, 2, "One should equal two");
}

CTEST(SUITE_NAME, test_one_equals_one) {
  const char *greeting = "hello";
  ASSERT_STR_EQ(greeting, "hello", "greeting variable should equal \"hello\"");
}
```

To build and execute it:
```sh
$ gcc ./tests/test_example.c -o ./tests/test_example
$ ./ctest # (or wherever your ctest binary is)
================================================================================================================================
                                               --- RUNNING CTEST ON: ./tests ---                                                
================================================================================================================================
[FAIL]  ./tests/test_example: F.
======================================================== FAILURE REPORT ========================================================

  [FAIL] One should equal two
         Expression: 1 == 2
         Location  : Line 7 in tests/test_example.c

================================================================================================================================
                        SUITES: 1 (0.00s)  TESTS: 2  PASSED: 1  FAILED: 1  CRASHED: 0  TIMEOUTS: 0                         
================================================================================================================================
```

---

## Contents

- [**Features**](#features)
- [**Build**](#build)
- [**First Tests**](#first-tests)
- [**Assertions**](#assertions)
- [**Test Assignment & Lifecycle Hooks**](#test-assignment--lifecycle-hooks)
- [**Stdout Capture & Output Control (Mute/Unmute)**](#stdout-capture--output-control-muteunmute)
- [**Mock Items**](#mock-items)
- [**Multi-Suite Organisation & Execution**](#multi-suite-organisation--execution)
- [**Command-Line Flags & Configuration**](#command-line-flags--configuration)
- [**Result Caching**](#result-caching)
- [**Contact**](#contact)
- [**LICENSE**](#license)

---

## Features

- Automatic discovery of test suite binaries
- Process isolation for signals, crashes, and timeouts
- Built-in mock subsystem for files, directories, and binaries ([Mock Items](#mock-items))
- Result caching engine based on binary modification time ([Result Caching](#result-caching))
- Stream muting and stdout capture utilities ([Stdout Capture & Output Control (Mute/Unmute)](#stdout-capture--output-control-muteunmute))

---

## Build

Requires `gcc` and `make`. Once the repository is cloned from the remote, you can run these build commands at the `ctest` directory location:

**runtime builds:**
```sh
make setup_deps # Clone the dependency repos
make            # Compiles the executable cshell binary
```

**dev builds:**
```sh
make test       # Compiles and runs the test suite (./tests)
make debug      # Builds with debug symbols and sanitisers
```

**clean:**
```sh
make clean_deps # Cleans compiled binaries, .o, .a, and .d files (including compiled dependencies)
make clean      # Cleans compiled binaries, .o, and .d files (just for ctest)
```

---

## First Tests

To see `ctest` in action, let's walk through a basic calculator project. Here is what the structure for this project will be like:

```
deps/ctest/
└── ...
src/
├── calculator.c
└── calculator.h
tests/
└── test_calculator.c
```

> We will assume `ctest` is located at `./deps/` and has been [built](#build).

First, define your source module with a header (`calculator.h`) and its implementation (`calculator.c`):

`./src/calculator.h`:
```C
#ifndef CALCULATOR_H
#define CALCULATOR_H

int calculator_add(int a, int b);

#endif
```

`./src/calculator.c`:
```C
#include "calculator.h"

int calculator_add(int a, int b) {
  return a + b;
}
```

> With TDD workflow you would implement after writing the tests. For simplicity matters, we will do it before.

Next, create your test file inside the `tests/` directory. Include the `ctest` framework header and your local module header:

`./tests/test_calculator.c`:
```C
#include <ctest/ctest.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

CTEST(SUITE_NAME, test_add_one_and_one) {
  ASSERT_INT_EQ(calculator_add(1, 1), 2, "1 + 1 should equal 2");
}
```

> `ctest` will search `tests/` by default, but you can change this through [configurations](#command-line-flags--configuration).

### Compiling and Running

Compile the test binary with `gcc`, using `-I` flags to point to your local source directory and `ctest` dependencies. Then, invoke `ctest` to automatically discover and execute it:

Compile and execute tests:
```sh
$ gcc tests/test_calculator.c src/calculator.c -Isrc -Ideps -o tests/test_calculator
$ ./deps/ctest/ctest
================================================================================================================================
                                               --- RUNNING CTEST ON: ./tests ---                                                
================================================================================================================================
[PASS]  ./tests/test_calculator: .
================================================================================================================================
                                                  ALL 1 SUITES PASSED (0.00s)                                                  
================================================================================================================================
```

### Automating with a `Makefile`

To avoid running manual compilation commands during active development, you can manage your build and test flow with a lightweight `Makefile`. Here is a basic example:

```Makefile
CC = gcc
CFLAGS = -Isrc -Ideps
CTEST_PATH = ./deps/ctest/ctest

test: tests/test_calculator
        $(CTEST_PATH)

tests/test_calculator: tests/test_calculator.c src/calculator.c src/calculator.h
        $(CC) $(CFLAGS) tests/test_calculator.c src/calculator.c -o $@

clean:
        rm -f tests/test_calculator .ctest_cache
```

Now, running your entire test suite is just a single command:

```sh
$ make test
```

---

## Assertions

`ctest` provides a suite of assertion macros designed to validate test conditions and generate detailed failure reports with exact file and line numbers.

|Macro|Description|
|---|---|
|`ASSERT(condition, message)`|Asserts that a generic boolean condition evaluates to true.|
|`ASSERT_INT_EQ(actual, expected, message)`|Asserts that two integer values are equal (`actual == expected`).|
|`ASSERT_STR_EQ(actual, expected, message)`|Asserts that two strings contain identical character sequences (`strcmp` equality).|
|`ASSERT_DOUBLE_EQ(actual, expected, epsilon, message)`|Asserts that a floating-point value equals an expected value within a given error tolerance (`epsilon`).|
|`ASSERT_PTR_NOT_NULL(ptr, message)`|Asserts that a given pointer is not `NULL`.|
|`ASSERT_PTR_NULL(ptr, message)`|Asserts that a given pointer is `NULL`.|
|`ASSERT_SIGNAL(func, arg, expected_sig, message)`|Asserts that executing `func(arg)` in an isolated child process raises the expected signal (e.g., `SIGSEGV`, `SIGABRT`).|
|`ASSERT_EXIT_CODE(func, arg, expected_code, message)`|Asserts that executing `func(arg)` in an isolated child process terminates with the expected exit code.|

We have seen examples of assertions already but we will see some more throughout the rest of the document.

---

## Test Assignment & Lifecycle Hooks

To organise your tests and manage shared resources cleanly, `ctest` uses suite groupings and lifecycle hooks.

### Registering Tests

Tests are defined using the `CTEST(suite, name)` macro. Grouping related tests under the same `suite` name allows them to share setup/teardown logic and run together under a common banner. By convention, I define a `SUITE_NAME` macro at the top of the test file (usually the same name as the file) just to keep things clean:

```C
#include <ctest/ctest.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

CTEST(SUITE_NAME, test_add_one_and_one) {
    ASSERT_INT_EQ(calculator_add(1, 1), 2, "1 + 1 should equal 2");
}
```

### Lifecycle Hooks

If your tests require initialisation or cleanup, `ctest` provides four distinct hook macros depending on whether the code should run once per suite or before/after every individual test:
- **Suite-Level Hooks:** `CTEST_SETUP_SUITE(suite)` and `CTEST_TEARDOWN_SUITE(suite)` run **once** before and after all tests in the given suite.
- **Test-Level Hooks:** `CTEST_SETUP(suite)` and `CTEST_TEARDOWN(suite)` run **before and after every single test** in the suite.

Expanding our calculator example to use test-level hooks for resource management looks like this:
```C
#include <ctest/ctest.h>
#include <stdlib.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

// pointer to share test resources across setup and tests
static int *test_context = NULL;

CTEST_SETUP(SUITE_NAME) {
  // allocate memory or initialise resources before each test
  test_context = malloc(sizeof(int));
  *test_context = 10;
}

CTEST_TEARDOWN(SUITE_NAME) {
  // clean up resources after each test
  free(test_context);
  test_context = NULL;
}

CTEST(SUITE_NAME, test_addition_with_context) {
  ASSERT_INT_EQ(calculator_add(*test_context, 5), 15, "10 + 5 should equal 15");
}

CTEST(SUITE_NAME, test_another_calculator_operation) {
  ASSERT_INT_EQ(calculator_add(*test_context, -2), 8, "10 - 2 should equal 8");
}
```

---

## Stdout Capture & Output Control (Mute/Unmute)

Sometimes you need to test code that writes directly to standard output (`stdout`) or suppress noisy logs during testing. `ctest` provides built-in stream control functions to mute output descriptors and capture stdout buffers programmatically.

### Adding a Logging Feature to Our Calculator

Imagine our calculator has a division function that prints diagnostic messages or warnings to `stdout`.

`./src/calculator.h`:
```C
// rest of ./src/calculator.h here

int calculator_divide_with_log(int a, int b);
```
`./src/calculator.c`:
```C
// rest of ./src/calculator.c here

#include <stdio.h>

int calculator_divide_with_log(int a, int b) {
  if (b == 0) {
    printf("Error: Division by zero attempted!\n");
    return 0;
  }

  int ans = a / b;
  printf("Info: %d / %d equals %d\n", a, b, ans);
  return a / b;
}
```

### Muting & Unmuting Streams

If a test triggers expected error logs or noisy diagnostic output that you don't want polluting your test runner's terminal, you can temporarily redirect standard streams (like `STDOUT_FILENO` or `STDERR_FILENO`) to `/dev/null`.

Here is how you use `ctest_mute_output()` and `ctest_unmute_output()` in a test:

`./tests/test_calculator.c`:
```C
#include <ctest/ctest.h>
#include <unistd.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

CTEST(SUITE_NAME, test_divide_by_zero_returns_zero) {
  // mute stdout during the risky operation
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);

  int result = calculator_divide_with_log(10, 0);
  ASSERT_INT_EQ(result, 0, "Division by zero should return 0 safely");

  // restore stdout back to normal
  ctest_unmute_output(saved_stdout, STDOUT_FILENO);
}
```

### Capturing `stdout`

When you want to **assert on what your code printed**, you can capture standard output between a start/end block using an internal POSIX pipe mechanism.

`ctest_capture_stdout_start(void)` redirects `stdout` into a pipe and `ctest_capture_stdout_end(char *buf, size_t buf_size)` flushes the stream, restores `stdout`, and reads the printed text into your string buffer.

Here is how we verify that out calculator logs the correct message:

`./tests/test_calculator.c`:
```C
CTEST(SUITE_NAME, test_successful_division_prints_log) {
  char output_buf[256];

  // begin intercepting stdout
  ctest_capture_stdout_start();

  int result = calculator_divide_with_log(10, 2);

  // stop intercepting, restore stdout, and read captured bytes into buffer
  ctest_capture_stdout_end(output_buf, sizeof(output_buf));

  ASSERT_INT_EQ(result, 5, "10 / 2 should equal 5");
    
  // assert that the expected log message was printed
  ASSERT_STR_EQ(
    buf, "Info: 10 / 2 equals 5\n"
    "Calculator should log successful division to stdout"
  );
}
```

---

## Mock Items

`ctest` provides a built-in mock management subsystem that tracks temporary files, directories, and compiled binaries, automatically cleaning them up when tests finish or even if the process crashes or times out.

### Creating Mock Files and Directories

To test code that reads from disk, you can take advantage of the mock directory and file helper methods.

You can use `ctest_setup_mock_file(const char *path, const char *content)`. This creates a temporary file at the given `path` and populates it with your specified string `content`. You can then call `ctest_teardown_mock_file(const char *path)` to manually teardown the mock file if you choose to do so.

To create a mock directory you can use `ctest_setup_mock_dir(const char *path)`. It is important to remember that if you wish to setup a mock item inside a mock directory then you should setup the directory beforehand. Also, if you wish to manually teardown a mock directory using `ctest_teardown_mock_dir(const char *path)` then you should teardown all of the mock items inside of that directory first.

To show this, let's imagine we want to create a method for our calculator that can solve an equation that is stored in a `.txt` file. We will define this method as `int calculator_solve_file(const char *path)` where `const char *path` is the path to the `.txt` file.

`./src/calculator.h`:
```C
// rest of ./src/calculator.h here

int calculator_solve_file(const char *path);
```

To keep the example simple and focused on `ctest`, I will not show the implementation of `int calculator_solve_file(const char *path)`. If you wish to see the test below pass, then feel free to implement it in `./src/calculator.c` (and flesh out more test cases for better coverage).

In the test below, we will create a mock `.txt` file in a mock `./equations/` directory to test whether the equation `5 + 3 - 4` can be resolved correctly.

`./tests/test_calculator.c`:
```C
#include <ctest/ctest.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

CTEST(SUITE_NAME, test_calculator_solve_file_resolves_equation_from_path) {
  const char *dir_path = "equations";
  const char *file_path = "equations/equation_0.txt";
  const char *equation_content = "5 + 3 - 4";

  // set up the mock equation file
  ctest_setup_mock_dir(dir_path);
  ctest_setup_mock_file(file_path, equation_content);

  // assert that the equation is solved correctly
  ASSERT_INT_EQ(
    calculator_solve_file(file_path), 4, 
    "Equation loaded from file should resolve to the correct value"
  );

  // optional manual teardown (must teardown inner files before outer directory)
  ctest_teardown_mock_file(file_path);
  ctest_teardown_mock_dir(dir_path);
}
```

### Compiling Mock Binaries

For advanced testing scenarios where your code spawns or interacts with external executables, helper scripts, or plugins, `ctest_setup_mock_binary(const char *path, const char *c_code)` writes a temporary `.c` source file, compiles it via `gcc` into an executable binary at the target path, and configures executable permissions (`0755`). If you wish to manually teardown the mock binary and its respective `.c` source file, you can use `ctest_teardown_mock_binary(const char *path)`.

Since mock binaries don't fit neatly into our calculator project, here is a real-world example from `ctest`'s own test suite.

This test verifies the CLI executor's `ctest_launch_suite()` method (found in `./src/ctest_cli/executor.c`), which spawns a suite binary as a managed process and populates a `WorkerSlot` tracking structure. The details are not so important for this example. What is important is that I am testing something that interacts with an executable binary. If you do wish to delve deeper into the details then check out `./src/ctest_cli/executor.h`, `./src/ctest_cli/executor.c` and `./tests/ctest_cli/test_executor.c`

`./tests/ctest_cli/test_executor.c`:
```C
#include "ctest_cli/executor.h"
#include <ctest/ctest.h>
#include <sys/wait.h>

#define SUITE_NAME test_executor

static const char *mock_passing_bin = "./mock_passing_bin_35893333";
static const char *mock_passing_c_code = ...; // c code that simulates a passing suite

CTEST_SETUP_SUITE(SUITE_NAME) {
  ctest_setup_mock_binary(mock_passing_bin, mock_passing_c_code);
  // other mock binary suite setups here...
}

CTEST(SUITE_NAME, test_launch_suite_successful_launch) {
  WorkerSlot slot = {0};

  ASSERT_INT_EQ(
      ctest_launch_suite(mock_passing_bin, &slot), 0,
      "ctest_launch_suite() call with valid binary path should return 0");
  ASSERT_INT_EQ(slot.is_active, 1,
                "WorkerSlot.is_active should be 1 after successful launch");
  ASSERT(
      slot.pid >= 0,
      "WorkerSlot.pid should be a positive process ID after successful launch");
  ASSERT(slot.read_fd >= 0, "WorkerSlot.read_fd should be a valid file "
                            "descriptor after successful launch");
  ASSERT_STR_EQ(slot.bin_path, mock_passing_bin,
                "WorkerSlot.bin_path should equal input binary_path");
  ASSERT_INT_EQ(
      slot.buf_pos, 0,
      "WorkerSlot.buf_pos should be set to 0 after successful launch");

  int status;
  waitpid(slot.pid, &status, 0);
  close(slot.read_fd);
}
```

In this example, you can see how mock item helpers have actually been used in testing for `ctest`. You may have noticed these preferences:
- **Collision avoidance:** Appending a random 8-digit suffix to mock paths prevents accidental file collisions. You do not have to do this exactly, but I find it is a low-energy way of guaranteeing you will not have collision problems.
- **Suite-level setup:** In this case, it was useful to have a normalised mock of a passing test bin, which I could refer to in test cases throughout the `test_executor` suite.
- **Omitting manual teardown:** In most cases, I will let `ctest` handle automatic mock cleanup for me. This reduces the logic per test, making them more dependable.

### Setup collision safety

When calling a mock setup method, `ctest` will check if an instance of that file/directory does not already exist on the system. This avoids overwriting non-mock files/directories, but may fail tests involving the mock item if a collision occurs, so this is important to keep in mind.

Also, it is impossible for the mock teardown methods to teardown a file that is not registered as a mock to `ctest`, so this avoids a collided path from tearing down the non-mock item on manual teardown.

### Automated Teardown & Crash Safety

While you can manually call teardown functions like `ctest_teardown_mock_file()`, `ctest_teardown_mock_binary()`, `ctest_teardown_mock_dir()`, and `ctest_teardown_all_mocks()`, `ctest` will automatically handle cleanup for you.

At the end of each suite registry and at the end of a test binary, all mock items are cleaned automatically. Also, in the case of a termination/crash, ctest intercepts the signal and invokes `ctest_teardown_all_mocks()`.

This guarantees that your working directory (or wherever you spawn your mock items) is never left polluted with orphaned test files or compiled binaries.

---

## Multi-Suite Organisation & Execution

As your project grows, you will naturally want to split your tests into multiple files rather than cramming everything into a single test file. `ctest` makes this effortless: it scans your test directory (defaults to `./tests/`), executes every compiled test binary it finds, and aggregates the results into a single clean summary.

### Adding a Second Test Suite

Let's expand our project structure by adding a second test file alongside our existing `test_calculator.c`:

`./tests/test_calculator_advanced.c`:
```C
#include <ctest/ctest.h>
#include "calculator.h"

#define SUITE_NAME test_calculator_advanced

CTEST(SUITE_NAME, test_subtract_numbers) {
    ASSERT_INT_EQ(calculator_subtract(10, 4), 6, "10 - 4 should equal 6");
}
```

### Updating the `Makefile` for Multi-Suite Builds

Instead of hardcoding individual test targets, we can update our Makefile to use wildcard pattern matching. This automatically compiles any source file in `tests/` starting with `test_` into its corresponding executable:

```Makefile
CC = gcc
CFLAGS = -Isrc -Ideps
CTEST_PATH = ./deps/ctest/ctest

# automatically find all test source files and derive their binary paths
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(TEST_SRCS:.c=)

test: $(TEST_BINS)
	$(CTEST_PATH)

tests/test_%: tests/test_%.c src/calculator.c src/calculator.h
	$(CC) $(CFLAGS) $< src/calculator.c -o $@

clean:
	rm -f tests/test_* .ctest_cache
```

### Running the Multi-Suite Test Run

Now, running `make test` compiles both test binaries and lets `ctest` discover and execute them concurrently:

```sh
$ make test
================================================================================================================================
                                            --- RUNNING CTEST ON: ./tests ---                                                   
================================================================================================================================
[PASS]  ./tests/test_calculator: .
[PASS]  ./tests/test_calculator_advanced: .
================================================================================================================================
                                                 ALL 2 SUITES PASSED (0.00s)                                                    
================================================================================================================================
```

---

## Command-Line Flags & Configuration

`ctest` can be configured via command-line arguments or through a project-level configuration file (`.ctestconfig`). `ctest` configurations have a hierarchy of assignment: CLI flags overwrite `.ctestconfig` configurations, which overwrite `ctest` default configurations.

### Overview of Configuration Options

|Option/Flag|`.ctestconfig` Key|Description|Default|
|---|---|---|---|
|`[test_directory]`|`target_dir`|Path to the directory containing compiled test binaries.|`./tests`|
|`-h, --help`|-|Show the usage help message and exit.|-|
|`-v, --verbose`|`verbosity = verbose`|Print detailed execution output per suite.|Normal|
|`-q, --quiet`|`verbosity = quiet`|Suppress per-suite progress and show summary only.|Normal|
|`--no-cache`|`use_cache = false`|Toggle the [caching engine](#result-caching) off|Enabled (`true`)|
|`-f, --filter <pattern>`|`filter = <pattern>`|Run only test suites matching the specified pattern (glob matching).|None|
|`-t, --timeout <sec>`|`timeout = <sec>`|Set an execution timeout per test suite in seconds.|No Timeout (`0`)|
|`-j, --jobs <N>`|`jobs = <N>`|Number of concurrent test suite worker processes|`1`|
|`--json <file>`|`json = <file>`|Export test metrics and failure ledger to a JSON report.|None|

### Using Command-Line Flags

You can pass options directly when running `ctest`. For example, to run tests with 4 parallel jobs, a 10-second timeout per suite, and filtering for suites matching `*_calculator`:
```sh
$ ./deps/ctest/ctest -j 4 -t 10 -f *_calculator
```

To run without [result caching](#result-caching) (useful during local debugging when modifying test runner behaviour):
```sh
$ ./deps/ctest/ctest --no-cache
```

### Using `.ctestconfig` Files

Instead of specifying CLI flags every time, you can place a `.ctestconfig` file in your project root. `ctest` parses key-value pairs (supporting `#` and `;` comments).

`./.ctestconfig`:

```.ctestconfig
# ctest project configuration
target_dir = ./tests
jobs = 4
timeout = 3
verbosity = normal
use_cache = true
json = build/ctest_report.json
```

---

## Result Caching

`ctest` features a built-in result caching engine designed to speed up test execution by avoiding redundant runs of unchanged test suites.

### How Caching Works

By default, passing test suites are cached locally in a `.ctest_cache` file. `ctest` tracks each binary using its file path modification time (`mtime`).

If a test binary remains unmodified since the last run, `ctest` skips execution and instantly loads the cached passing result.

### Automatic Invalidation & Re-Runs

Caching follows an implicit pass model, meaning updates and failures automatically trigger a fresh execution:
- **Recompilations:** If you modify code and recompile a test suite, its binary modification time changes. `ctest` detects this stale entry and automatically re-runs the suite.
- **Failures & Crashes:** Test suites that fail, time out, or crash bypass or purge existing cache entries, ensuring broken tests are never falsely cached as passing.
- **Disabling Caching:** If you ever need to bypass the cache entirely during debugging, you can use the `--no-cache` command-line flag or set `use_cache = false` in your `.ctestconfig` file. You can find more on this in the [Command Line Flags & Configuration](#command-line-flags--configuration) section.

---

## Contact

Created by [**WillEdgington**](https://github.com/WillEdgington)

📧 [**willedge037@gmail.com**](mailto:willedge037@gmail.com) &nbsp;|&nbsp; 🔗 [**LinkedIn**](https://www.linkedin.com/in/williamedgington/)

---

## License

This project is licensed under the [MIT License](LICENSE).
