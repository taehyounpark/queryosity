#pragma once

#include <map>

#include "dataflow.h"
#include "systematic.h"

namespace queryosity
{

namespace dataset
{

template <typename DS> class loaded
{

  public:
    loaded(dataflow &df, DS &ds);
    ~loaded() = default;

    template <typename Val> auto _read(const std::string &name)
    {
        return m_df->_read<DS, Val>(*m_ds, name);
    }

    template <typename Val> auto read(dataset::column<Val> const &col) -> lazy<queryosity::column::valued<Val>>;

    template <typename... Vals> auto read(dataset::column<Vals> const &...cols);

    template <typename Val> auto vary(dataset::column<Val> const &col, std::map<std::string, std::string> const &vars);

  protected:
    dataflow *m_df;
    dataset::reader<DS> *m_ds;
};

} // namespace dataset

} // namespace queryosity

#include "dataset_column.h"
#include "lazy.h"
#include "lazy_varied.h"
#include "systematic_variation.h"

template <typename DS> queryosity::dataset::loaded<DS>::loaded(queryosity::dataflow &df, DS &ds) : m_df(&df), m_ds(&ds)
{
}

template <typename DS>
template <typename Val>
auto queryosity::dataset::loaded<DS>::read(dataset::column<Val> const &col) -> lazy<queryosity::column::valued<Val>>
{
    return col.template _read(*this);
}

template <typename DS>
template <typename... Vals>
auto queryosity::dataset::loaded<DS>::read(dataset::column<Vals> const &...cols)
{
    return std::make_tuple(cols.template _read(*this)...);
}

template <typename DS>
template <typename Val>
auto queryosity::dataset::loaded<DS>::vary(dataset::column<Val> const &col,
                                           std::map<std::string, std::string> const &vars)
{
    auto nom = this->read(col);
    typename decltype(nom)::varied varied_column(std::move(nom));
    for (auto const &var : vars)
    {
        varied_column.set_variation(var.first, this->read(dataset::column<Val>(var.second)));
    }
    return varied_column;
}