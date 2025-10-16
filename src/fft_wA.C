#include "fft_wA.h"

// Simple FFT for power-of-2 sizes using Cooley-Tukey
// Returns magnitude in output array

void fft_compute(float* input, float* output, int size) {
  
    float real[size];
    float imag[size];
    for (int i = 0; i < size; i++) {
        real[i] = input[i];
        imag[i] = 0.0f;
    }

    int j = 0;
    for (int i = 0; i < size; i++) {
        if (i < j) {
            float tmp = real[i]; real[i] = real[j]; real[j] = tmp;
            tmp = imag[i]; imag[i] = imag[j]; imag[j] = tmp;
        }
        int m = size >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    // Cooley-Tukey FFT
    for (int len = 2; len <= size; len <<= 1) {
        float angle = -2.0f * M_PI / len;
        float wlen_real = cosf(angle);
        float wlen_imag = sinf(angle);
        for (int i = 0; i < size; i += len) {
            float w_real = 1.0f;
            float w_imag = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                int u = i + j;
                int v = i + j + len / 2;
                float t_real = w_real * real[v] - w_imag * imag[v];
                float t_imag = w_real * imag[v] + w_imag * real[v];

                real[v] = real[u] - t_real;
                imag[v] = imag[u] - t_imag;

                real[u] += t_real;
                imag[u] += t_imag;

                float tmp_w_real = w_real * wlen_real - w_imag * wlen_imag;
                w_imag = w_real * wlen_imag + w_imag * wlen_real;
                w_real = tmp_w_real;
            }
        }
    }

    for (int i = 0; i < size; i++) {
        output[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]);
    }
}
