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


TEST_CASE("propagation of systematic variations")
{
    std::ifstream data_file("data.json");
    nlohmann::json data_json(nlohmann::json::parse(data_file));

    double wsumx_nom = 0;
    double wsumx_xvar = 0;
    double wsumx_wvar = 0;
    for (unsigned int i = 0; i < data_json.size(); ++i)
    {
        auto x_nom = data_json[i]["x"].template get<double>();
        auto x_var = data_json[i]["x_smear"].template get<double>();
        auto w_nom = data_json[i]["w"].template get<unsigned int>();
        auto w_var = data_json[i]["w_toy"].template get<unsigned int>();
        wsumx_nom += x_nom * w_nom;
        wsumx_xvar += x_var * w_nom;
        wsumx_wvar += x_nom * w_var;
    }

    qty::dataflow df;
    auto ds = df.load(dataset::input<qty::json>(data_json));
    auto x = ds.vary(dataset::column<double>("x"), {"vary_x", "x_smear"});
    auto w = ds.vary(dataset::column<unsigned int>("w"), {"vary_w", "w_toy"});
    auto weighted = df.weight(w);
    auto wsumx = df.make(query::plan<qty::wsum>()).fill(x).book(weighted);

    auto one = df.define(column::constant<double>(1));
    auto two = df.define(column::constant<double>(2));
    auto one_or_two = systematic::vary(systematic::nominal(one), systematic::variation("two", two));

    auto zero_or_two = df.vary(column::expression([](float a, double b) { return a - b; }),
                               systematic::variation("two", [](double a, float b) { return a + b; }))(one, one);

    auto no_or_yes = df.all().get(column::series(one_or_two == zero_or_two));
    auto always_no = no_or_yes.nominal().result();
    auto always_yes = no_or_yes["two"].result();

    SUBCASE("dependent columns") {
        CHECK(std::accumulate(always_no.begin(),always_no.end(),0) == 0);
        CHECK(std::accumulate(always_yes.begin(),always_yes.end(),0) == data_json.size());
    }

    SUBCASE("transparency") {
        CHECK(wsumx_nom == wsumx.nominal().result());
        CHECK(wsumx_xvar == wsumx["vary_x"].result());
        CHECK(wsumx_wvar == wsumx["vary_w"].result());
    }
}