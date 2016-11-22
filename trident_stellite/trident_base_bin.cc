//
// 2016 write by snibug@linecorp.com
//

#include <memory>

#include "trident_stellite/trident_base.h"

int main(int argc, char *argv[]) {
  std::unique_ptr<trident::TridentBase> trident_base(
      new trident::TridentBase());
  trident_base->Initialize(argc, argv);
  return 0;
}
