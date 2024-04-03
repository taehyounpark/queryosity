#include <fstream>
#include <sstream>
#include <vector>

#include "queryosity.h"
#include "queryosity/hist.h"
#include "queryosity/json.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

using ull_t = unsigned long long;
auto factorial(ull_t n) {
  ull_t result = 1;
  while (n > 1)
    result *= n--;
  return result;
}

auto stirling = [](ull_t n) {
  return std::round(std::sqrt(2 * M_PI * n) * std::pow(n / std::exp(1), n));
};

class Factorial : public column::definition<double(ull_t, double, ull_t)> {
public:
  Factorial(ull_t threshold = 20) : m_threshold(threshold) {}
  virtual ~Factorial() = default;
  virtual double evaluate(column::observable<ull_t> n, column::observable<double> fast,
                 column::observable<ull_t> full) const override {
    // 1. if n! is too small & can fit inside ull_t, use full calculation
    // 2. if n is large enough, use approximation
    return (n.value() >= std::min<ull_t>(m_threshold, 20)) ? fast.value()
                                                    : full.value();
  }
  void adjust_threshold(ull_t threshold) { m_threshold = threshold; }

protected:
  ull_t m_threshold;
};

int main() {
  dataflow df;

  std::ifstream data_json("data.json");
  auto n = df.load(dataset::input<json>(data_json))
               .read(dataset::column<ull_t>("n"));

  auto n_f_fast = df.define(column::expression(stirling))(n);
  auto n_f_full = df.define(column::expression(factorial))(n);

  ull_t n_threshold = 10;
  auto n_f_slow = df.define(
      column::expression([n_threshold](ull_t n, double fast, ull_t slow) -> double {
        return n >= std::min<ull_t>(n_threshold,20) ? fast : slow;
      }))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + t(fast) + t(slow)
  // :(

  auto n_f_best =
      df.define(column::definition<Factorial>(/*20*/))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + { t(n_fast) if n >= 10, t(n_slow) if n < 10 }
  // :)

  // advanced: access per-thread instance
  dataflow::node::invoke([n_threshold](Factorial *n_f) { n_f->adjust_threshold(n_threshold); },
                         n_f_best);
}
