#pragma once

/**
 * @defgroup api API
 * @defgroup abc ABCs
 */

/**
 * @namespace queryosity
 * The root namespace of all queryostiy namespaces and classes.
 */
namespace queryosity;

#include "interface/multithread.h"

#include "interface/dataset_reader.h"

#include "interface/column_definition.h"
#include "interface/column_reader.h"

#include "interface/query_aggregation.h"
#include "interface/query_definition.h"

#include "interface/dataflow.h"

#include "interface/lazy.h"
#include "interface/lazy_varied.h"
#include "interface/todo.h"
#include "interface/todo_varied.h"

#include "interface/systematic_vary.h"