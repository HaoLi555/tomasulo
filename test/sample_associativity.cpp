constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalTime = 32;
constexpr int totalRound = 64;

int main(int argc, char **argv) {
    auto cacheSize = ((unsigned) argv[0]);
    auto cacheBlockSize = ((unsigned) argv[1]);
    auto testAssociativity = ((unsigned) argv[2]);
    unsigned cacheLines = cacheSize / cacheBlockSize;
    int sum = 0;
    unsigned step = cacheLines / testAssociativity;
    step = step * cacheBlockSize;

    unsigned index = 0;
    for (int i = 0; i < totalRound; i++)
        for (unsigned j = 0; j < totalTime; j += 1) {
            sum += arr[index];
            index += step;
            if (index >= 2 * cacheSize) {
                index = 0;
            }
        }

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
