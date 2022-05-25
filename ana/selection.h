#pragma once

#include <memory>

#include "ana/action.h"
#include "ana/cell.h"
#include "ana/concurrent.h"

namespace ana
{

template <typename T>
class column;

class selection : public action
{

public:
	class cut;
	class weight;

	class cutflow;

public:
	selection(const std::string& name);
  virtual ~selection() = default;

public:
	void set_initial();
	void set_previous(const selection& preselection);

	bool is_initial() const;
	const selection* get_previous() const;

	void set_channel(bool channel=true);
	bool is_channel() const noexcept;

	std::string path() const;
	std::string full_path() const;

	template <typename T>
	void set_decision(std::shared_ptr<column<T>> decision);

	virtual bool   passed_cut() const = 0;
	virtual double get_weight() const = 0;

public:
	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

private:
	const selection* m_preselection;

	std::shared_ptr<variable>     m_decision;
	std::shared_ptr<cell<double>> m_value;

	bool m_channel;

};

}

#include "ana/cell.h"
#include "ana/column.h"
#include "ana/counter.h"

template <typename T>
void ana::selection::set_decision(std::shared_ptr<column<T>> decision)
{
	// selection is responsible for resetting its own decision: keep variable-upcast pointer
	m_decision = decision;
	// selection value is boolean/numerical: link to cell<double>
	m_value = ana::value_as<double>(*decision);
}