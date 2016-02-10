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

#include <boost/filesystem.hpp>
#include <boost/pool/pool.hpp>

#include <boost/serialization/strong_typedef.hpp>

#include <boost/fiber/all.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/accumulate.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/remove_if.hpp>

#include <windows.h>

#include <Box2D\Box2D.h>

#include <cml/cml.h>

#include "Usings.h"
#include "Math.h"
#include "Utils.h"
#include "Tuning.h"