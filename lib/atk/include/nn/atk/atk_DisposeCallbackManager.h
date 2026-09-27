#pragma once

#include <cstddef>
#include <cstdint>

#include <nn/atk/atk_DisposeCallback.h>

namespace nn::atk::detail::driver {

class DisposeCallbackManager {
public:
    using CallbackList = util::IntrusiveList<
        DisposeCallback,
        util::IntrusiveListMemberNodeTraits<DisposeCallback, &DisposeCallback::m_DisposeLink>>;

    // TODO: Maybe there's a better way to prevent inlining. Needed to match
    // DisposeCallbackManager::Dispose
    __attribute__((noinline)) static DisposeCallbackManager& GetInstance();

    void Dispose(const void* mem, size_t size);

    void RegisterDisposeCallback(DisposeCallback* callback);
    void UnregisterDisposeCallback(DisposeCallback* callback);

    uint64_t GetCallbackCount() const;

    DisposeCallbackManager();

private:
    CallbackList m_CallbackList;
};
static_assert(sizeof(DisposeCallbackManager) == 0x10);

}  // namespace nn::atk::detail::driver
