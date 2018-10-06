#include <test.h>

int sum_for(int val, int iterations)
{
    int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += val;
    }

    return sum;
}