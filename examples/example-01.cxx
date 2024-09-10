#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/hist.hpp>
#include <queryosity/json.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main() {
  dataflow df(multithread::enable(10));

  std::ifstream data("data.json");
  auto [x, v, w] = df.read(
      dataset::input<json>(data), dataset::column<double>("x"),
      dataset::column<std::vector<double>>("v"), dataset::column<double>("w"));

  auto zero = df.define(column::constant(0));
  auto v0 = v[zero];

  auto sel =
      df.weight(w)
          .filter(column::expression(
              [](std::vector<double> const &v) { return v.size(); }))(v)
          .filter(column::expression([](double x) { return x > 100.0; }))(x);

  auto h_x0_w = df.get(query::result<h1d>(linax(20, 0.0, 200.0)))
                    .fill(v0)
                    .at(sel)
                    .result();

  std::ostringstream os;
  os << *h_x0_w;
  std::cout << os.str() << std::endl;
}