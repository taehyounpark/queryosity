#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <nlohmann/json.hpp>
#include <random>
#include <unordered_map>

#include "queryosity.h"

#include "queryosity/json.h"
#include "queryosity/wsum.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;

// generate random data
nlohmann::json generate_random_data(unsigned int nentries = 100) {
  nlohmann::json random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> random_signal(100, 10);
  std::normal_distribution<double> random_variation(1.05, 0.1);
  std::poisson_distribution<unsigned int> random_weight(1);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_signal(gen);
    auto v = random_variation(gen);
    auto w = random_weight(gen);
    auto w_var = random_weight(gen);
    random_data.emplace_back<nlohmann::json>({{"i", i},
                                              {"w_nom", w},
                                              {"w_var", w_var},
                                              {"x_nom", x},
                                              {"x_var", x * v}});
  }
  return random_data;
}

std::vector<double> get_correct_result(const nlohmann::json &random_data) {
  double wsumx_nom = 0;
  double wsumx_xvar = 0;
  double wsumx_wvar = 0;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x_nom = random_data[i]["x_nom"].template get<double>();
    auto x_var = random_data[i]["x_var"].template get<double>();
    auto w_nom = random_data[i]["w_nom"].template get<unsigned int>();
    auto w_var = random_data[i]["w_var"].template get<unsigned int>();
    wsumx_nom += x_nom * w_nom;
    wsumx_xvar += x_var * w_nom;
    wsumx_wvar += x_nom * w_var;
  }
  return std::vector<double>{wsumx_nom, wsumx_xvar, wsumx_wvar};
}

std::vector<double> get_queryosity_result(const nlohmann::json &random_data) {
  qty::dataflow df;
  auto ds = df.open<Json>(random_data);
  auto x = ds.read<double>("x_nom", {{"vary_x", "x_var"}});
  auto w = ds.read<unsigned int>("w_nom", {{"vary_w", "w_var"}});
  auto wsumx = df.get<WeightedSum>().fill(x).at(df.weight("weight")(w));
  auto wsumx_nom = wsumx.nominal().result();
  auto wsumx_xvar = wsumx["vary_x"].result();
  auto wsumx_wvar = wsumx["vary_w"].result();
  return std::vector<double>{wsumx_nom, wsumx_xvar, wsumx_wvar};
}

TEST_CASE("compute weighted sum") {

  // get answers
  auto random_data = generate_random_data(1000);
  auto correct_result = get_correct_result(random_data);
  auto queryosity_result = get_queryosity_result(random_data);

  // compare answers
  CHECK(queryosity_result[0] == correct_result[0]);
  CHECK(queryosity_result[1] == correct_result[1]);
  CHECK(queryosity_result[2] == correct_result[2]);
}