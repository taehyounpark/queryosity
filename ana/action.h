#pragma once

#include <string>

#include "ana/routine.h"

namespace ana 
{

class action : public routine
{

public:
	action(const std::string& name);
	virtual ~action() = default;

	const std::string& get_name() const;

protected:
	std::string m_name;

};

}
