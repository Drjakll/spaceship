/* Compile and execute the real external PufferLib CPU inference implementation. */
#include PUFFERCPU_SOURCE
int main(void) {
    float inputs[] = {1, 2, 3, 4};
    float weights[] = {2, 0, 0, 3};
    float bias[] = {1, -1};
    float output[4] = {0};
    const float expected[] = {3, 5, 7, 11};
    _linear(inputs, weights, bias, output, 2, 2, 2);
    for (int i = 0; i < 4; ++i) {
        if (fabsf(output[i] - expected[i]) > 1e-6f) return 1;
    }
    if (fabsf(_sigmoid(0) - 0.5f) > 1e-6f) return 1;
    puts("PASS real pinned PufferLib CPU inference smoke");
    return 0;
}
