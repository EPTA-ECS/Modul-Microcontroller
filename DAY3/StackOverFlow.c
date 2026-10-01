// #####1. UNDERSTANDING STACK MEMORY WORKS
#include <stdio.h>

void f2(int a, int b) {
    printf("f2: a = %d, b = %d\n", a, b);
}   /* f2 returns -> its frame is popped */

void f1(int x, int y) {
    f2(x, y);          /* push f2 frame */
}   /* f1 returns -> its frame is popped */

int main(void) {
    int r = 8;
    int t = 3;
    f1(r, t);          /* push f1 frame */
    return 0;
}

//CALL STACK PROGRESSION (OVER TIME)
//                     f2
//                     |
//            f1       f1       f1
//            |        |        |
//  main --> main --> main --> main --> main
//  (1)      (2)      (3)      (4)      (5)

// (1) program starts, only main
// (2) main calls f1      -> push f1
// (3) f1 calls f2        -> push f2  (deepest point)
// (4) f2 returns         -> pop f2
// (5) f1 returns         -> pop f1, back in main

//SNAPSHOT
// Snapshot at step (2): after f1() is called
//                        top of stack, where the
//           <----------  next stack frame will go
//  ------------
//  |  x = 8   |
//  |  y = 3   |   <---  stack frame (f1)
//  |  f1      |
//  |----------|
//  |  r = 8   |
//  |  t = 3   |   <---  stack frame (main)
//  |  main    |
//  ------------
// Snapshot at step (3): after f2() is called
//  ------------   <---  new top
//  |  a = 8   |
//  |  b = 3   |   <---  stack frame (f2)
//  |  f2      |
//  |----------|
//  |  x = 8   |
//  |  y = 3   |   <---  stack frame (f1)
//  |  f1      |
//  |----------|
//  |  r = 8   |
//  |  t = 3   |   <---  stack frame (main)
//  |  main    |
//  ------------

// #####2. RECURSION PROBLEM (STACK OVERFLOW)
#include <stdio.h>

#define LENGTH 1000000

/* Global array: stored in the data/BSS segment, NOT on the stack.
   This way the only thing that can overflow the stack is the recursion. */
int a[LENGTH];

int sum(int arr[], int length, int i) {
    if (i == length)
        return 0;                              /* base case */
    else
        return arr[i] + sum(arr, length, i + 1);  /* recursive case */
}

int main(void) {
    for (int i = 0; i < LENGTH; i++)
        a[i] = 1;                              /* expected sum = 1000000 */

    printf("Recursive sum = %d\n", sum(a, LENGTH, 0));
    return 0;
}

// What happens on the stack
//  ------------   <---  top keeps growing...
//  | i=999999 |
//  | sum      |
//  |----------|
//  |   ...    |   <---  ~1,000,000 frames
//  |----------|
//  | i = 2    |
//  | sum      |
//  |----------|
//  | i = 1    |
//  | sum      |
//  |----------|
//  | i = 0    |
//  | sum      |   <---  first call
//  |----------|
//  | main     |
//  ------------

//  Each sum frame (arr, length, i, return address, saved frame pointer)
//  is roughly 32-48 bytes at -O0.
//  1,000,000 frames x ~48 B  ~= 48 MB  >  8 MB stack limit
//  -> the stack runs past its guard page -> SIGSEGV

// ##########2.1 SOLUTION FOR THIS CASE
  #include <stdio.h>
  
  #define LENGTH 1000000
  
  int a[LENGTH];
  
  int main(void) {
      int length = LENGTH;
      long total = 0;
  
      for (int i = 0; i < length; i++)
          a[i] = 1;
  
      for (int i = 0; i < length; i++)
          total += a[i];          /* same work as sum(), but no new frames */
  
      printf("Loop sum = %ld\n", total);   /* prints 1000000 */
      return 0;
  }

// #####3. OTHER CASE CAUSING STACK OVERFLOQW
#include <stdio.h>
#define LENGTH 10000000              /* 10 million ints = 40 MB */

long fill_and_sum(void) {
    int local[LENGTH];               /* 40 MB local array, ON THE STACK */
    long total = 0;
    for (int i = 0; i < LENGTH; i++) {
        local[i] = 1;                /* crashes when it touches past the stack limit */
        total += local[i];
    }
    return total;
}

int main(void) {
    printf("sum = %ld\n", fill_and_sum());
    return 0;
}

// ------------
// |          |
// | local[]  |   <---  ONE frame, but 40 MB  >  8 MB stack limit
// | 40 MB    |
// |          |
// |----------|
// | main     |
// ------------
// Depth = 2 (small), size = huge -> overflow

// ##########3.1 SOLUTION FOR THIS CASE
#include <stdio.h>
#define LENGTH 1000000

int a[LENGTH];                       /* global, not on the stack */

long sum_loop(int arr[], int length) {
    long total = 0;
    for (int i = 0; i < length; i++)
        total += arr[i];             /* same frame, reused 1,000,000 times */
    return total;
}

int main(void) {
    for (int i = 0; i < LENGTH; i++) a[i] = 1;
    printf("sum = %ld\n", sum_loop(a, LENGTH));   /* prints 1000000 */
    return 0;
}









