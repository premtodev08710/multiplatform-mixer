// Minimal dependency-free test harness.
#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace mpstest
{
struct TestCase { const char* name; std::function<void()> fn; };

inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& failures() { static int f = 0; return f; }
inline int& checks()   { static int c = 0; return c; }

struct Registrar { Registrar (const char* n, std::function<void()> f) { registry().push_back ({ n, std::move (f) }); } };

inline int runAll (const char* suiteName)
{
    int failedTests = 0;
    std::printf ("=== %s: %d tests ===\n", suiteName, (int) registry().size());
    for (auto& t : registry())
    {
        const int before = failures();
        t.fn();
        const bool ok = failures() == before;
        if (! ok) ++failedTests;
        std::printf ("[%s] %s\n", ok ? " OK " : "FAIL", t.name);
    }
    std::printf ("=== %s: %d/%d tests passed, %d checks, %d failed checks ===\n", suiteName,
                 (int) registry().size() - failedTests, (int) registry().size(), checks(), failures());
    return failedTests == 0 ? 0 : 1;
}
} // namespace mpstest

#define MPS_CAT2(a, b) a##b
#define MPS_CAT(a, b) MPS_CAT2(a, b)
#define TEST(name) \
    static void name(); \
    static mpstest::Registrar MPS_CAT(reg_, name) (#name, name); \
    static void name()

#define CHECK(cond) do { ++mpstest::checks(); if (! (cond)) { ++mpstest::failures(); \
    std::printf ("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_NEAR(a, b, eps) do { ++mpstest::checks(); const double va_ = (double) (a), vb_ = (double) (b); \
    if (! (std::abs (va_ - vb_) <= (double) (eps))) { ++mpstest::failures(); \
    std::printf ("    FAIL %s:%d: %s = %.9g, expected %s = %.9g (eps %g)\n", __FILE__, __LINE__, #a, va_, #b, vb_, (double) (eps)); } } while (0)
