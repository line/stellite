//
// 2016 write by snibug@linecorp.com
//

#include "trident_stellite/trident_base.h"

#include "base/command_line.h"

namespace trident {

TridentBase::TridentBase() {}

TridentBase::~TridentBase() {}

void TridentBase::Initialize(int argc, char *argv[]) {
  base::CommandLine::Init(argc, argv);
}

}  // namespace trident
