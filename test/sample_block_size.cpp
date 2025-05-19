constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalTime = 4096;
constexpr int totalRound = 1;

int main(int argc, char **argv) {
    auto step = ((unsigned) argv[0]);
    int sum = 0;
    unsigned max_index=totalTime*step;

    for (int i = 0; i < totalRound; i++)
        for (unsigned j = 0; j < max_index; j += step) sum += arr[j];

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
