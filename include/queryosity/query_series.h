#pragma once

#include "query_definition.h"

#include <vector>

namespace queryosity
{

namespace query
{

template <typename T> class series : public queryosity::query::definition<std::vector<T>(T)>
{

  public:
    series() = default;
    ~series() = default;

    virtual void initialize(unsigned int, unsigned long long, unsigned long long) final override;
    virtual void fill(column::observable<T>, double) final override;
    virtual void finalize(unsigned int) final override;
    virtual std::vector<T> result() const final override;
    virtual std::vector<T> merge(std::vector<std::vector<T>> const &results) const final override;

  protected:
    std::vector<T> m_result;
};

} // namespace query

} // namespace queryosity

template <typename T>
void queryosity::query::series<T>::initialize(unsigned int, unsigned long long begin, unsigned long long end)
{
    m_result.reserve(end - begin);
}

template <typename T> void queryosity::query::series<T>::fill(column::observable<T> x, double)
{
    m_result.push_back(x.value());
}

template <typename T> void queryosity::query::series<T>::finalize(unsigned int)
{
    m_result.resize(m_result.size());
}

template <typename T> std::vector<T> queryosity::query::series<T>::result() const
{
    return m_result;
}

template <typename T>
std::vector<T> queryosity::query::series<T>::merge(std::vector<std::vector<T>> const &results) const
{
    std::vector<T> merged;
    size_t merged_size = 0;
    for (auto const &result : results)
    {
        merged_size += result.size();
    }
    merged.reserve(merged_size);
    for (auto const &result : results)
    {
        merged.insert(merged.end(), result.begin(), result.end());
    }
    return merged;
}