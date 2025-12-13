#include "llvm/Transforms/Obfuscation/Utils.h"

namespace llvm {

thread_local Rng *threadLocalRng = nullptr;

Rng::Rng() : engine(std::random_device()()), dist01(0.0, 1.0), distUint32() {}

Rng &Rng::getInstance() {
    if (!threadLocalRng) {
        threadLocalRng = new Rng();
    }
    return *threadLocalRng;
}

double Rng::next01() { return dist01(engine); }

uint32_t Rng::nextUint32() { return distUint32(engine); }

} // namespace llvm
