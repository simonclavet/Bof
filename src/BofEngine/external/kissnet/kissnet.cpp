#include "kissnet.hpp"

int main_non_pas_besoin()
{
  #if defined(KISSNET_USE_OPENSSL)
    printf("kissnet is using OpenSSL.");
  #else
    printf("kissnet is not using OpenSSL.");
  #endif
    return 0;
}
