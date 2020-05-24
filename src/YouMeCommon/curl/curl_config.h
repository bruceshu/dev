#if defined WIN32

#elif defined __APPLE__
#include "curl_config_apple.h"
#elif defined __ANDROID__
#include "curl_config_android.h"
#elif defined OS_LINUX
#include "curl_config_linux.h"
#else
#error "curl_config_android.h"
#endif
