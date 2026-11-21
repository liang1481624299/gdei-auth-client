// src/core/SecretStore.cpp
// 后端选择 / Backend selection.
#include "core/SecretStore.hpp"

#ifdef Q_OS_WIN
namespace gdei { SecretStore& winSecretStore(); }
#elif defined(Q_OS_MACOS)
namespace gdei { SecretStore& macSecretStore(); }
#else
namespace gdei { SecretStore& fallbackSecretStore(); }
#endif

namespace gdei {

SecretStore& SecretStore::instance() {
#ifdef Q_OS_WIN
    return winSecretStore();
#elif defined(Q_OS_MACOS)
    return macSecretStore();
#else
    return fallbackSecretStore();
#endif
}

} // namespace gdei
