# RaftCPP

Trying to build a Raft distributed consensus protocal in cpp.

# how to use (Mac os)
```bash
# Make sure install dependencies first. See dependencies file in root dir.
mkdir build
cd build
cmake ..
make

# To be continued about how to do next
```

# Run tests
```bash
cd build
tests/runBasicTest

# you could also use valgrind
# valgrind --leak-check=yes --trace-children=yes --track-origins=yes tests/runBasicTest
```
