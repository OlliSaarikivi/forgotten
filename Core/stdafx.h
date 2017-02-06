// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOMINMAX

#include "targetver.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <assert.h>
#include <limits>
#include <tuple>
#include <chrono>
#include <thread>
#include <atomic>
#include <variant>
#include <mutex>
#include <utility>
#include <random>

#include <boost/serialization/strong_typedef.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/accumulate.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/remove_if.hpp>

#include <fmt/format.h>

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>
#include <intrin.h>

#include <Box2D\Box2D.h>

#include <cml/cml.h>

using std::string;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::weak_ptr;
using std::array;
using std::function;

using std::pair;
using std::make_pair;
using std::tuple;
using std::get;

using std::variant;

namespace chrono = std::chrono;
using namespace std::chrono_literals;

namespace mpl = boost::mpl;

using namespace cml;