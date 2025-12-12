#pragma once

#include <random>

namespace llvm {

class Rng {
public:
  // Get the thread-local RNG instance
  static Rng &getInstance();

  // Generate a double in [0,1)
  double next01();

  // Generate a random 32-bit unsigned integer
  uint32_t nextUint32();

private:
  Rng();
  std::mt19937 engine;
  std::uniform_real_distribution<double> dist01;
  std::uniform_int_distribution<uint32_t> distUint32;
};

}
