//
// 2016 write by snibug@linecorp.com
//
#ifndef TRIDENT_STELLITE_TRIDENT_BASE_H_
#define TRIDENT_STELLITE_TRIDENT_BASE_H_

#include <stddef.h>
#include <memory>

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TRIDENT_STELLITE_IMPLEMENTATION)
#define TRIDENT_STELLITE_EXPORT __declspec(dllexport)
#else
#define TRIDENT_STELLITE_EXPORT __declspec(dllimport)
#endif  // TRIDENT_STELLITE_IMPLEMENTATION

#else  // defined(WIN32)
#if defined(TRIDENT_STELLITE_IMPLEMENTATION)
#define TRIDENT_STELLITE_EXPORT __attribute__((visibility("default")))
#else
#define TRIDENT_STELLITE_EXPORT
#endif  // TRIDENT_STELLITE_IMPLEMENTATION
#endif

#else  // COMPONENT_BUILD
#define TRIDENT_STELLITE_EXPORT
#endif

namespace trident {

class TRIDENT_STELLITE_EXPORT TridentBase {
 public:
  TridentBase();
  ~TridentBase();
  void Initialize(int argc, char *argv[]);
};

}  // namespace trident

#endif  // TRIDENT_STELLITE_TRIDENT_BASE_H_
