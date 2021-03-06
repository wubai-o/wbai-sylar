#ifndef __WUBAI_HASHUTILS_H__
#define __WUBAI_HASHUTILS_H__

#include<stdint.h>
#include<string>

namespace wubai {

std::string base64decode(const std::string& src);
std::string base64encode(const std::string& data);
std::string base64encode(const void* data, size_t len);

std::string sha1sum(const std::string& data);
std::string sha1sum(const void* data, size_t len);

std::string random_string(size_t len, const std::string& chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

}   // namespace wubai




#endif // __WUBAI_HASHUTILS_H__