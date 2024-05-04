#pragma once

/**
 * @defgroup ext Extensions
 * @defgroup api API
 * @defgroup abc ABCs
 */

#include "queryosity/multithread.hpp"

#include "queryosity/dataset_reader.hpp"

#include "queryosity/column_definition.hpp"
#include "queryosity/column_equation.hpp"
#include "queryosity/column_reader.hpp"
#include "queryosity/column_series.hpp"

#include "queryosity/selection_yield.hpp"

#include "queryosity/query_aggregation.hpp"
#include "queryosity/query_definition.hpp"
#include "queryosity/query_series.hpp"

#include "queryosity/dataflow.hpp"

#include "queryosity/lazy.hpp"
#include "queryosity/lazy_varied.hpp"
#include "queryosity/todo.hpp"
#include "queryosity/todo_varied.hpp"

namespace qty = queryosity;