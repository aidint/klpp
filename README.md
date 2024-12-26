# Kl++ (Kaleidoscope++)

Kl++ (Kaleidoscope++) is a minimal functional programming language designed to extend the LLVM-based toy language [Kaleidoscope](https://github.com/llvm/llvm-project/tree/main/llvm/examples/Kaleidoscope). It offers both OCRJIT through a REPL interface and Ahead-of-Time (AOT) compilation. Additionally, Kl++ supports debugging, all while maintaining the simple, elegant design of the original Kaleidoscope.



## Features

- **Standard REPL**: Kl++ provides a REPL interface for running JIT'd code on the fly.
- **Compiler**: Kl++ comes with a compiler, capable of building executable binaries for Unix-like systems.
- **Debugging**: Kl++ can provide debug information for the binary files it compiles.
- **Standard Library**: Kl++ includes a standard library, offering simple yet useful functions and operators.
- **Method Replacement**: Kl++ syntax permits overwriting functions as long as the prototype stays the same.

## Getting Started

### Requirements

- C++ compiler with C++20 support
- LLVM-19 development libraries
- CMake (3.20 or higher)

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/aidint/klpp
   cd klpp
   ```

2. Configure and build the project using CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   make # or ninja
   ```

3. Use `kl++` executable in `build` directory to compile or spin up the standard REPL.

## Language specifications

### Data Type

Continuing the minimalist design philosophy of LLVM Kaleidoscope, Kl++ exclusively uses double-precision floating-point numbers as its sole data type.
However, the `main` function is an exception: it always returns a 32-bit signed integer with the value `0`, adhering to standard conventions for executable programs.

### Language syntax

Kl++ features an abstract syntax consisting of the following elements:

#### Function Definition


```
def <function name>(<space separated parameter list>) <function body>;
```
- function body is a single well-formed Kl++ expression
- Example:
        ```
        def foo(x) if x > 2 then 1 else 0;
        ```
#### External Declaration

```
extern <function name>(<space separated parameter list>);
```
- Example:
    ```
    extern sin(x);
    ```

#### Expressions

1. Numbers and Variables
2. `if` expressions
    ```
    if <condition expression> then <expression> [else <expression>];
    ```
    - condition expression is true if the value is not 0.00
    - Example:
        ```
        if x > 0 then 1 else -1;
        ```
3. `for` expression
    ```
    for <inner variable> = <initial value expression>,
        <condition expression>,
        <step expression> do
            <expression>
        end;
    ```
    - The `for` expression performs iteration, always returning `0` as its value. 
    - Example:
        ```
        for i = 0, i < 10, 1 do
            putchard(42)
        end;
        ```
4. `with` expression (block-specific variable definition)
    ```
    with <variable name> = <expression>[, <variable_name> = <expression>]* do
        <inner expression>
    end;
    ```
    - Defines temporary variables within a block. The expression returns the value of the inner expression.  
    - Example:
        ```
        with a = 10, b = 20 do
            a + b
        end;
        ```
5. `#` which starts a comment block until end of the line.

## Standard Library and Builtin Functions

Check the Kaleidoscope header and source files for the standard library in the [lib/std directory](./lib/std).

## Example Program

Here is a simple Kl++ program that prints a Christmas tree:

### Input (`christmastree.kl`):
```
# Print a Christmas tree with 20 rows
def putstar() putchard(42);

def putspace(n)
  for i = 0, i < n, 1 do
    putchard(32)
  end;

def newline()
  putchard(10);

def putline(n)
  for i = 0, i < n, 1 do
    putstar():putspace(1)
  end;

def christmastree(n base)
  if n == 1 then
    putspace(base - 1):
    putstar():
    putspace(base - 1):
    newline()
  else
    christmastree(n - 1, base):
    putspace(base - n):
    putline(n):
    putspace(base - n):
    newline();

def main()
  christmastree(20, 20);
```

### Compilation:

Use the following command to compile (and link) with debug info
```bash
./kl++ -d christmastree.kl christmastree.out

```

### Output:
```
> ./christmastree.out
                   *                   
                  * *                   
                 * * *                  
                * * * *                 
               * * * * *                
              * * * * * *               
             * * * * * * *              
            * * * * * * * *             
           * * * * * * * * *            
          * * * * * * * * * *           
         * * * * * * * * * * *          
        * * * * * * * * * * * *         
       * * * * * * * * * * * * *        
      * * * * * * * * * * * * * *       
     * * * * * * * * * * * * * * *      
    * * * * * * * * * * * * * * * *     
   * * * * * * * * * * * * * * * * *    
  * * * * * * * * * * * * * * * * * *   
 * * * * * * * * * * * * * * * * * * *  
* * * * * * * * * * * * * * * * * * * *
```

### Debugging:
Use `gdb` or `lldb` to debug the code:
```bash
> lldb christmastree.out
(lldb) target create "christmastree.out"
Current executable set to 'christmastree.out'.
(lldb) break set -f christmastree.kl -l 18
Breakpoint 1: where = christmastree.out`christmastree + 16 at christmastree.kl:18:8, address = 0x0000000100003b54
(lldb) r
Process 72768 launched: 'christmastree.out'.
Process 72768 stopped
* thread #1, queue = 'main-thread', stop reason = breakpoint 1.1
    frame #0: 0x0000000100003b54 christmastree.out`christmastree(n=20, base=20) at christmastree.kl:18:8
   15  	 end;
   16  	
   17  	def christmastree(n base)
-> 18  	 if n == 1 then
   19  	   putspace(base - 1):
   20  	   putstar():
   21  	   putspace(base - 1):
(lldb) p n
(double) 20
(lldb) p n == 1
(bool) false
(lldb) c
Process 72768 resuming
Process 72768 stopped
* thread #1, queue = 'main-thread', stop reason = breakpoint 1.1
    frame #0: 0x0000000100003b54 christmastree.out`christmastree(n=19, base=20) at christmastree.kl:18:8
   15  	 end;
   16  	
   17  	def christmastree(n base)
-> 18  	 if n == 1 then
   19  	   putspace(base - 1):
   20  	   putstar():
   21  	   putspace(base - 1):
(lldb) p n
(double) 19

```

## Documentation

For more details on the original Kaleidoscope language, refer to the [Kaleidoscope example in LLVM](https://github.com/llvm/llvm-project/tree/main/llvm/examples/Kaleidoscope).

## License

[MIT License](./LICENSE)

