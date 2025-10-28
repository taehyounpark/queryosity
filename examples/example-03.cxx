#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/boost/histogram.hpp>
#include <queryosity/nlohmann/json.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::nlohmann::json;
using h1d = qty::boost::histogram::histogram<double>;
using linax = qty::boost::histogram::axis::regular;

using ull_t = unsigned long long;
auto factorial(column::observable<ull_t> n) {
  ull_t result = 1;
  auto n_factorial = n.value();
  while (n_factorial > 1)
    result *= n_factorial--;
  return result;
}
auto stirling = [](column::observable<ull_t> n) {
  return std::round(std::sqrt(2 * M_PI * n.value()) * std::pow(n.value() / std::exp(1), n.value()));
};

class Factorial : public column::definition<double(ull_t, double, ull_t)> {
public:
  Factorial(ull_t threshold = 20) : m_threshold(threshold) {}
  virtual ~Factorial() = default;
  virtual double evaluate(column::observable<ull_t> n, column::observable<double> fast,
                 column::observable<ull_t> full) const override {
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
  auto n_f_best =
      df.define(column::definition<Factorial>(/*20*/))(n, n_f_fast, n_f_full);

  // advanced: access per-thread instance
  ull_t n_threshold = 10;
  dataflow::node::invoke([n_threshold](Factorial *n_f) { n_f->adjust_threshold(n_threshold); },
                         n_f_best);
}