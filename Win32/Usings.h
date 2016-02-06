#pragma once

using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::array;
using std::function;

using std::pair;
using std::make_pair;
using std::tuple;
using std::get;

namespace chrono = std::chrono;
using namespace std::chrono_literals;

namespace mpl = boost::mpl;
namespace fibers = boost::fibers;

using fibers::future;
using fibers::promise;

//using glm::vec2;
//using glm::vec3;

//namespace gl = oglplus;
namespace fs = boost::filesystem;