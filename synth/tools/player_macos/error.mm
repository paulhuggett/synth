#include "error.h"

std::string osstatus_error_category::message (int error) const {
  return description_from_osstatus (static_cast<OSStatus> (error));
}

std::string osstatus_error_category::description_from_osstatus (OSStatus const err) {
  NSError const* const error = [NSError errorWithDomain:NSOSStatusErrorDomain
                                                   code:err
                                               userInfo:nil];
  return error.description.UTF8String;
}

std::error_category const& get_osstatus_error_category () {
  static osstatus_error_category cat;
  return cat;
}

std::error_code make_osstatus_error_code (OSStatus const erc) {
  return {static_cast<int> (erc), get_osstatus_error_category ()};
}
