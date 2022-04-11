#ifndef ERROR_H
#define ERROR_H

#import <Foundation/Foundation.h>

#include <string>
#include <system_error>

class osstatus_error_category final : public std::error_category {
public:
  char const* name () const noexcept override {
    return "OSStatus error category";
  }
  std::string message (int error) const override;

private:
  static std::string description_from_osstatus (OSStatus const err);
};

std::error_category const& get_osstatus_error_category ();
std::error_code make_osstatus_error_code (OSStatus const erc);

#endif  // ERROR_H
