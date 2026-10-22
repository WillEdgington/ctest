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
- [**Author**](#author)
- [**LICENSE**](#license)

---

## Features

<!-- To be completed -->

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

> We will assume `ctest` is located at `./deps/` and has been built.

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

<!-- Link to configurations section in this note -->
> `ctest` will search `tests/` by default, but you can change this through configurations

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

Tests are defined using the `CTEST(suite, name)` macro. Grouping related tests under the same `suite` name allows them to share setup/teardown logic and run together under a common banner. By convention, I define a SUITE_NAME macro at the top of the test file (usually the same name as the file) just to keep things clean:

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

// Pointer to share test resources across setup and tests
static int *test_context = NULL;

CTEST_SETUP(SUITE_NAME) {
  // Allocate memory or initialise resources before each test
  test_context = malloc(sizeof(int));
  *test_context = 10;
}

CTEST_TEARDOWN(SUITE_NAME) {
  // Clean up resources after each test
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
  // Mute stdout during the risky operation
  int saved_stdout = ctest_mute_output(STDOUT_FILENO);

  int result = calculator_divide_with_log(10, 0);
  ASSERT_INT_EQ(result, 0, "Division by zero should return 0 safely");

  // Restore stdout back to normal
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

  // Begin intercepting stdout
  ctest_capture_stdout_start();

  int result = calculator_divide_with_log(10, 2);

  // Stop intercepting, restore stdout, and read captured bytes into buffer
  ctest_capture_stdout_end(output_buf, sizeof(output_buf));

  ASSERT_INT_EQ(result, 5, "10 / 2 should equal 5");
    
  // Assert that the expected log message was printed
  ASSERT_STR_EQ(
    buf, "Info: 10 / 2 equals 5\n"
    "Calculator should log successful division to stdout"
  );
}
```

---

## Mock Items

`ctest` provides a built-in mock management subsystem that tracks temporary files, directories, and compiled binaries, automatically cleaning them up when tests finish or even if the process crashes.

### Creating Mock Files and Directories

To test code that reads from disk, you can take advantage of the mock directory and file helper methods.

You can use ctest_setup_mock_file(const char *path, const char *content). This creates a temporary file at the given path and populates it with your specified string contents. You can then call `ctest_teardown_mock_file(const char *path)` to manually teardown the mock file if you choose to do so.

To create a mock directory you can use `ctest_setup_mock_dir(const char *path)`. It is important to remember that if you wish to setup a mock item inside a mock directory then you should setup the directory beforehand. Also, if you wish to manually teardown a mock directory using `ctest_teardown_mock_dir(const char *path)` then you should teardown all of the mock items inside of that directory first.

To show this, lets imagine we want to create a method for our calculator that can solve an equation that is stored in a `.txt` file. We will define this method as `int calculator_solve_file(const char *path)` where `const char *path` is the path to the `.txt` file.

`./src/calculator.h`:
```C
// rest of ./src/calculator.h here

int calculator_solve_file(const char *path);
```

To keep the example simple and focused on `ctest`, I will not show the implementation of `int calculator_solve_file(const char *path)`. If you wish to see the test below pass then feel free to implement it in `./src/calculator.c` (and flesh out more test cases for better coverage).

In the test below we will create a mock `.txt` file in a mock `./equations/` directory to test whether the equation `5 + 3 - 4` can be resolved correctly.

`./tests/test_calculator.c`:
```C
#include <ctest/ctest.h>
#include "calculator.h"

#define SUITE_NAME test_calculator

CTEST(SUITE_NAME, test_calculator_solve_file_resolves_equation_from_path) {
  const char *dir_path = "equations";
  const char *file_path = "equations/equation_0.txt";
  const char equation_content = "5 + 3 - 4";

  // setup the mock equation file
  ctest_setup_mock_dir(dir_path);
  ctest_setup_mock_file(file_path, equation_content);

  // assert that the equation is solved correctly
  ASSERT_INT_EQ(
    calculator_solve_file(file_path), 4, 
    "Equation loaded from file should resolve to the corect value"
  );

  // Optional manual teardown (must teardown inner files before outer directory)
  ctest_teardown_mock_file(file_path);
  ctest_teardown_mock_dir(dir_path);
}

```

### Compiling Mock Binaries

For advanced testing scenarios where your code spawns or interacts with external executables, helper scripts, or plugins, `ctest_setup_mock_binary(const char *path, const char *c_code)` writes a temporary `.c` source file, compiles it via `gcc` into an executable binary at the target path, and configures executable permissions (`0755`). If you wish to manually teardown the mock binary and its respective `.c` source file, you can use `ctest_teardown_mock_binary(const char *path)`.

<!--
Example that involves binary mock:
Difficult to integrate this into the overall calculator example so it does not have to be.
Any simple example would work just to convey use.

Some sort of method that executes a binary and returns something easy to test.
Some sort of method that parses an IPC that is given by the mock bin.
Could just show an example from ./tests/ctest_cli/test_executor.c?
Just need to explain the method we are testing simply and well so that it does not diminish from the tutorial and main focus.
-->

<!-- Introduce the example -->

### Setup collision safety

When calling a mock setup method, `ctest` will check if an instance of that file/directory does not already exist on the system. This avoids overwriting non-mock files, but should fail tests involving the mock item if a collision occurs so this is important to keep in mind.

Also, it is impossible for the mock teardown methods to teardown a file that is not registered as a mock to `ctest` so this avoids a collided path from tearing down the non-mock item on manual teardown.

### Automated Teardown & Crash Safety

While you can manually call teardown functions like `ctest_teardown_mock_file()`, `ctest_teardown_mock_binary()`, `ctest_teardown_mock_dir()`, and `ctest_teardown_all_mocks()`, `ctest` will automatically handle cleanup for you.

At the end of each suite registry and at the end of a test binary all mock items are cleaned automatically. Also, in the case of a termination/crash, ctest intercepts the signal and invokes `ctest_teardown_all_mocks()`.

This guarentees that your working directory (or wherever you spawn your mock items) is never left polluted with orphaned test files or compiled binaries

---

## Multi-Suite Organisation & Execution



---

## Command-Line Flags & Configuration

---

## Result Caching

---

## Author

Created by [**WillEdgington**](https://github.com/WillEdgington)

📧 [**willedge037@gmail.com**](mailto:willedge037@gmail.com) &nbsp;|&nbsp; 🔗 [**LinkedIn**](https://www.linkedin.com/in/williamedgington/)

---

## License

This project is licensed under the [MIT License](LICENSE).