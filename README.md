# balloc

A small C library providing three memory allocation strategies from a single
header. No external dependencies. Single-threaded.

## Strategies

- `ba_arena` — bump allocator. Fastest possible allocation (pointer bump).
  Free all at once with `ba_arena_reset`. Use for scratch memory, per-frame
  state, request lifecycles, AST nodes.
- `ba_pool` — fixed-size object pool. O(1) alloc and free. Use for known
  hot-path objects: AST nodes, particles, request structs.
- `ba_heap` — general-purpose first-fit free list with immediate coalescing
  and splitting. Use when you need real malloc/free semantics inside a
  bounded region.

All three are pre-allocated: you pass a capacity to `_create`, and
allocation never calls the system allocator again. Useful for embedded
targets, real-time systems, and predictable performance.

## Build

```sh
make              # builds build/libballoc.a
make test         # runs the test suite
make bench        # runs the benchmark
make demo         # builds and runs examples/demo
```

Tested with `cc` on Linux. Requires POSIX (`posix_memalign`).

## Usage

### Arena

```c
#include "balloc.h"

ba_arena *scratch = ba_arena_create(64 * 1024);

char *buf = ba_arena_alloc(scratch, 256);
int  *arr = ba_arena_calloc(scratch, 100, sizeof(int));

ba_arena_reset(scratch);   // bulk free

ba_arena_destroy(scratch);
```

`ba_arena_alloc_aligned(a, size, align)` for custom alignment.

### Pool

```c
ba_pool *nodes = ba_pool_create(sizeof(node_t), 1024);

node_t *n = ba_pool_alloc(nodes);
ba_pool_free(nodes, n);

ba_pool_destroy(nodes);
```

### Heap

```c
ba_heap *h = ba_heap_create(1 << 20);   // 1 MiB

void *p = ba_heap_alloc(h, 256);
p = ba_heap_realloc(h, p, 1024);
ba_heap_free(h, p);

size_t peak = ba_heap_peak(h);
ba_heap_destroy(h);
```

## API

See `include/balloc.h`. All three strategies return `NULL` on failure.
All functions accept and tolerate `NULL` for queries (return 0) and
destruction (no-op).

## Benchmark

Linux x86_64, `-O2`, glibc 2.x. Lower is better.

```
small alloc/free (16 B, 200000 iters):
  balloc heap                        7.22 ns/op
  glibc malloc                      12.43 ns/op

large alloc/free (512 B, 200000 iters):
  balloc heap                       69.44 ns/op
  glibc malloc                      72.08 ns/op

mixed sizes (8-256 B), random free order:
  balloc heap                       34.09 ns/op
  glibc malloc                      55.94 ns/op

fixed-size object pool (64 B x 1024):
  balloc pool                        3.46 ns/op
  glibc malloc                       9.86 ns/op

arena bulk alloc (64 B x 1024):
  balloc arena                      14.44 ns/op
```

Run `make bench` to reproduce. The relative gap depends on workload;
glibc's malloc is highly tuned for long-running server workloads with
large arenas and tcache.

## Design notes

- All blocks are aligned to 16 bytes by default; `ba_arena_alloc_aligned`
  accepts any power-of-two alignment.
- The heap uses a doubly-linked free list with first-fit search and
  immediate forward coalescing. LIFO free patterns are fastest; best
  for short-lived or pooled allocations.
- `ba_heap_realloc` allocates, copies, frees. It does not attempt to
  grow in place.
- The pool reuses the freed slot's first `sizeof(void*)` bytes as the
  free-list link, so a pool can store objects as small as one pointer.
- All buffers are allocated with 64-byte alignment so that arena
  requests for high alignment are satisfied without internal slack.

## Limitations

- Single-threaded. No locking. Wrap with your own mutex if needed.
- The heap does not implement prev-block coalescing. Forward coalescing
  is implemented; backward is not. This is fine for LIFO access and
  keeps the code compact. Block fragmentation will be higher for
  long-lived random-order free patterns than a dlmalloc-style
  implementation.
- `ba_heap_realloc` is alloc-copy-free. There is no in-place grow.
- The pool does not check for double-free. Same trust model as
  `free(3)`.
- No thread-local or arena-per-thread helpers. Bring your own.

## License

MIT.
