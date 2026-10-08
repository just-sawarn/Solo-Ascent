#ifndef SA_TESTUTIL_H
#define SA_TESTUTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static int t_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); t_fail++; } } while (0)
#define CHECK_NEAR(a, b, eps) CHECK(fabs((double)(a) - (double)(b)) <= (eps))
#define TEST_DONE() do { printf(t_fail ? "FAILED (%d)\n" : "OK\n", t_fail); return t_fail ? 1 : 0; } while (0)
#endif
