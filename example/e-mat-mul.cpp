// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <vector>
#include <iostream>
#include <span>
#include <random>
#include <chrono>

class square_matrix {
    size_t m_size;
    std::vector<float> m_data;
public:
    explicit square_matrix(size_t size)
        : m_size(size)
        , m_data(size * size, 0.0f)
    {}

    float& at(size_t row, size_t col) {
        return m_data[row * m_size + col];
    }
    float at(size_t row, size_t col) const {
        return m_data[row * m_size + col];
    }

    size_t size() const {
        return m_size;
    }

    size_t num_elements() const {
        return m_size * m_size;
    }

    std::span<float> modify_data() {
        return m_data;
    }
    const std::vector<float>& data() const {
        return m_data;
    }
};

square_matrix mat_mul_seq(const square_matrix& a, const square_matrix& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("matrix size mismatch");
    }
    size_t n = a.size();
    square_matrix result(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            float sum = 0.0f;
            for (size_t k = 0; k < n; ++k) {
                sum += a.at(i, k) * b.at(k, j);
            }
            result.at(i, j) = sum;
        }
    }
    return result;
}

square_matrix mat_mul_par(const square_matrix& a, const square_matrix& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("matrix size mismatch");
    }
    size_t n = a.size();
    square_matrix result(n);
    par::pfor({}, size_t(0), n, [&](size_t i) {
        for (size_t j = 0; j < n; ++j) {
            float sum = 0.0f;
            for (size_t k = 0; k < n; ++k) {
                sum += a.at(i, k) * b.at(k, j);
            }
            result.at(i, j) = sum;
        }
    });
    return result;
}

square_matrix generate_random_matrix(size_t size, uint32_t seed) {
    std::mt19937 rng(seed); // fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-1, 1);

    square_matrix mat(size);
    for (auto& val : mat.modify_data()) {
        val = dist(rng);
    }
    return mat;
}

int main() {
    static constexpr std::size_t matrix_size = 500;
    auto a = generate_random_matrix(matrix_size, 42);
    auto b = generate_random_matrix(matrix_size, 1337);

    std::cout << "Multiplying two " << matrix_size << "x" << matrix_size << " matrices\n";

    auto start = std::chrono::high_resolution_clock::now();
    auto s = mat_mul_seq(a, b);
    auto seq_duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Sequential: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(seq_duration).count()
        << " ms\n";

    start = std::chrono::high_resolution_clock::now();
    auto p = mat_mul_par(a, b);
    auto par_duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "par(allel): "
        << std::chrono::duration_cast<std::chrono::milliseconds>(par_duration).count()
        << " ms\n";

    // verify correctness
    std::cout << std::boolalpha << "Sanity check: " << (s.data() == p.data()) << "\n";

    return 0;
}
