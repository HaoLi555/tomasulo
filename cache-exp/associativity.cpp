#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

unsigned MeasureCacheAssociativity([[maybe_unused]] ProcessorAbstract *p,
                                   [[maybe_unused]] unsigned cacheSize,
                                   [[maybe_unused]] unsigned cacheBlockSize) {
    // TODO: Measure the associativiy of the cache in the given processor
    // The associativity will be ranged from 1 to 8, and must be a power of 2
    // Return the accurate value
    unsigned testSize[5] = {1, 2, 4, 8, 16};
    unsigned testTime[5];

    for (int i = 0; i < 5; i++) {
        testTime[i] = execute(p, "./test/sample_associativity", 3, cacheSize, cacheBlockSize, testSize[i]);
    }
    for (int i = 0; i < 5; i++) {
        Logger::Warn(
            "With test associativity = %u, program simulator ran %u cycles.",
            testSize[i],
            testTime[i]);
    }
    int mx = 1000;
    int idx = 0;

    for (int i = 1; i <=4; i++) {
        int delta = ((int) testTime[i]) - ((int) testTime[i-1]);
        if (delta > mx) {
            mx = delta;
            idx = i;
        }
    }

    return testSize[idx];
}
