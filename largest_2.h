
#include <stdint.h>
#include <stdio.h>

void largest_two(int numbers[], uint8_t value[2], int score[2], int len) {

    int max = 0,
        smax = 0;
    uint8_t m_idx = 0,
            sm_idx = 0;
    for (int i = 0; i < len; i++) {
        if (numbers[i] >= max) {
            smax = max;
            max = numbers[i];
            sm_idx = m_idx;
            m_idx = i;
        } else if (numbers[i] >= smax) {
            smax = numbers[i];
            sm_idx = i;
        }
    }
    value[0] = m_idx;
    value[1] = sm_idx;
    score[0] = max;
    score[1] = smax;

    // for (int k = 0; k < len; k++){
    //   printf("%d: %d\t", k, numbers[k]);
    // }

    // printf("\n");
}
