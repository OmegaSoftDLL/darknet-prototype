// Test entry point (doctest). Kept in the SEPARATE TU from the test cases
// because the doctest implementation includes <windows.h> (same with
// WIN32_LEAN_AND_MEAN), which declares CloseWindow/ShowCursor/Rectangle and
// conflicts with raylib. Here in the game header is included.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
