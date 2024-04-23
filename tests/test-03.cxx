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
    // generate random data
    nlohmann::json test_data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<double> test_value(1 / 1.0);
    std::poisson_distribution<unsigned int> test_weight(1);
    std::uniform_int_distribution test_category(1, 3);
    unsigned int nentries = 100;
    for (unsigned int i = 0; i < nentries; ++i)
    {
        auto x = test_value(gen), x_var = test_value(gen);
        auto w = test_weight(gen), w_var = test_weight(gen);
        auto cat = test_category(gen), cat_var = test_category(gen);
        test_data.emplace_back<nlohmann::json>({{"i", i},
                                                {"x", x},
                                                {"x_var", x_var},
                                                {"w", w},
                                                {"w_var", w_var},
                                                {"cat", (cat == 1) ? ("a") : (cat == 2 ? "b" : "c")},
                                                {"cat_var", (cat_var == 1) ? ("a") : (cat_var == 2 ? "b" : "c")}});
    }

    double wsumx_nom = 0;
    double wsumx_xvar = 0;
    double wsumx_wvar = 0;
    for (unsigned int i = 0; i < test_data.size(); ++i)
    {
        auto x_nom = test_data[i]["x"].template get<double>();
        auto x_var = test_data[i]["x_var"].template get<double>();
        auto w_nom = test_data[i]["w"].template get<unsigned int>();
        auto w_var = test_data[i]["w_var"].template get<unsigned int>();
        wsumx_nom += x_nom * w_nom;
        wsumx_xvar += x_var * w_nom;
        wsumx_wvar += x_nom * w_var;
    }

    qty::dataflow df;
    auto ds = df.load(dataset::input<qty::json>(test_data));

    auto x = ds.vary(dataset::column<double>("x"), {{"vary_x", "x_var"}});
    auto w = ds.vary(dataset::column<unsigned int>("w"), {{"vary_w", "w_var"}, {"vary_test", "w"}});
    auto weighted = df.weight(w);
    auto wsumx = df.get(query::output<qty::wsum>()).fill(x).at(weighted);

    SUBCASE("transparency")
    {
        CHECK(wsumx_nom == wsumx.nominal().result());
        CHECK(wsumx_xvar == wsumx["vary_x"].result());
        CHECK(wsumx_wvar == wsumx["vary_w"].result());
    }

    auto one = df.define(column::constant<unsigned int>(1));
    auto two = df.define(column::constant<unsigned int>(2));
    auto one_or_two = df.vary(column::nominal(one), {{"two", two}});

    auto two_or_two = df.vary(column::expression([](unsigned long long a) { return a*2; }),
                               {{"two", [](long long a) { return a; }}})(one_or_two);

    auto all = df.filter(column::constant(true));
    auto no_or_yes = df.get(column::series(one_or_two == two_or_two)).at(all);
    auto always_no = no_or_yes.nominal().result();
    auto always_yes = no_or_yes["two"].result();

    SUBCASE("lockstep")
    {
        CHECK(std::accumulate(always_no.begin(), always_no.end(), 0) == 0);
        CHECK(std::accumulate(always_yes.begin(), always_yes.end(), 0) == test_data.size());
    }
}