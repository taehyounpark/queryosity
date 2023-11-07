#include <matplot/matplot.h>
#include <nlohmann/json.hpp>

#include "ana/analogical.h"

#include "ana/Histogram.h"
#include "ana/Json.h"

using json = nlohmann::json;

using dataflow = ana::dataflow;
namespace systematic = ana::systematic;

namespace bh = boost::histogram;
namespace mp = matplot;

// Function to plot a boost::histogram using matplotplusplus on an existing plot
template <typename T> void plot_histogram(const T &hist) {
  // Extract the bin counts and edges
  std::vector<double> counts, edges;
  for (auto &&bin : bh::indexed(hist)) {
    counts.push_back(*bin);
    edges.push_back(bin.bin().lower());
  }
  // Add the upper edge of the last bin
  edges.push_back(hist.axis(0).bin(hist.axis(0).size() - 1).upper());

  // Plot the histogram as stairs on the given axes
  mp::stairs(edges, counts);
}

int main() {

  std::ifstream data_file("data/data.json");
  auto data = json::parse(data_file);

  ana::dataflow df;
  auto ds = ds.open<Json>(data);

  auto x = ds.vary(ds.read<double>("x_nom"),
                   systematic::variation("scale", "x_scale"),
                   systematic::variation("smear", "x_smear"));
  auto w =
      df.vary(ds.read<double>("w_nom"), systematic::variation("toy", "w_toy"));

  auto weight = df.weight("weight")(w);

  auto hx =
      df.agg<Histogram<float>>(LinearAxis(50, 120, 130)).fill(x).book(weight);

  auto hx_nom = hx.nominal().result();
  auto hx_scale = hx["scale"].result();
  auto hx_smear = hx["smear"].result();
  auto hx_toy = hx["toy"].result();

  // Set up the plot (without displaying it yet)
  mp::hold(true);

  // Plot the histogram
  plot_histogram(*hx_nom);
  plot_histogram(*hx_scale);
  plot_histogram(*hx_smear);
  plot_histogram(*hx_toy);

  mp::save("example-histograms.pdf");

  return 0;
}