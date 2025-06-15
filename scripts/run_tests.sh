#!/bin/bash

# Navigate to the tests directory
cd ../tests

# Run the test suite using the appropriate test runner
# Assuming you are using a test framework like Google Test
./test_main

# Check the exit status of the test runner
if [ $? -eq 0 ]; then
    echo "All tests passed successfully."
else
    echo "Some tests failed. Please check the output above."
fi