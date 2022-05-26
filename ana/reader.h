#include "ana/column.h"

namespace ana
{

template <typename T>
class column<T>::reader : public column<T>
{

public:
  reader(const std::string& name);
  virtual ~reader() = default;

  virtual const T& value() const override;
  void read(const T& val);

protected:
	const T* m_addr;

};

}

template <typename T>
 ana::column<T>::reader::reader(const std::string& name) :
  column<T>(name),
  m_addr(nullptr)
{}

template <typename T>
void ana::column<T>::reader::read(const T& val)
{
  m_addr = &val;
}

template <typename T>
const T& ana::column<T>::reader::value() const
{
  return *m_addr;
}