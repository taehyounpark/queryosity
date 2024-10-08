#include <fstream>
#include <sstream>
#include <vector>

#include <queryosity.hpp>
#include <queryosity/boost/histogram.hpp>
#include <queryosity/nlohmann/json.hpp>
#include <queryosity/rapidcsv/csv.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::nlohmann::json;
using csv = qty::csv;
using h1d = qty::boost::histogram::histogram<double>;
using linax = qty::boost::histogram::axis::regular;

int main() {
  dataflow df(multithread::enable(10));

  std::ifstream data_json("data.json");
  auto x = df.load(dataset::input<json>(data_json))
               .read(dataset::column<double>("x"));

  std::ifstream data_csv("data.csv");
  auto y =
      df.load(dataset::input<csv>(data_csv)).read(dataset::column<double>("y"));

  auto z = x + y;

  auto all = df.filter(column::constant(true));

  auto h_z = df.get(query::output<h1d>(linax(20, 90.0, 110.0)))
                 .fill(z)
                 .at(all)
                 .result();

  std::ostringstream os;
  os << *h_z;
  std::cout << os.str() << std::endl;
}