<<<<<<< HEAD:lib/atk/include/nn/atk/detail/atk_SoundArchiveManager.h
/**
 * @brief Sound archive manager implementation.
 */

#pragma once

#include <cstdint>

namespace nn::atk {

class SoundHandle;
class SoundArchive;
class SoundDataManager;

namespace detail {
class AddonSoundArchiveContainer;
=======
#pragma once

#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/detail/atk_AddonSoundArchiveContainer.h>
#include <nn/atk/detail/atk_IntrusiveList.h>
>>>>>>> b0607f0 (atk: Redefine `detail::SoundArchiveManager`):include/nn/atk/detail/atk_SoundArchiveManager.h

namespace nn::atk::detail {
class SoundArchiveManager {
public:
    using ContainerList = IntrusiveList<AddonSoundArchiveContainer>;

    class Snapshot {
    public:
    private:
        SoundArchive* m_MainSoundArchive;
        SoundDataManager* m_MainSoundDataManager;
        SoundArchive* m_CurrentSoundArchive;
        SoundDataManager* m_CurrentSoundDataManager;
    };

    SoundArchiveManager();
    ~SoundArchiveManager();

    void Initialize(const SoundArchive* pSoundArchive, const SoundDataManager* pSoundDataManager);

    void ChangeTargetArchive(const char* soundArchiveName);

    void Finalize();

<<<<<<< HEAD:lib/atk/include/nn/atk/detail/atk_SoundArchiveManager.h
    uint64_t _8;
    uint64_t* _10;
    nn::atk::detail::AddonSoundArchiveContainer* _18;
    uint64_t* _20;
    nn::atk::SoundArchive* mSoundArchive;  // _28
    uint64_t _30;
    uint64_t _38;
    uint64_t _40;
};
}  // namespace detail
}  // namespace nn::atk
=======
    void Add(AddonSoundArchiveContainer&);
    void Remove(AddonSoundArchiveContainer&);

    bool IsAvailable() const;

    AddonSoundArchive* GetAddonSoundArchive(const char*) const;
    SoundDataManager* GetAddonSoundDataManager(const char*) const;
    AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(s32) const;
    AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(s32);

    void SetParametersHook(SoundArchiveParametersHook*);

private:
    SoundArchive* m_pMainSoundArchive;
    SoundDataManager* m_pMainSoundDataManager;
    ContainerList m_ContainerList;
    SoundArchive* m_pCurrentSoundArchive;
    SoundDataManager* m_pCurrentSoundDataManager;
    SoundArchiveParametersHook* m_pParametersHook;
};
static_assert(sizeof(SoundArchiveManager) == 0x38);
}  // namespace nn::atk::detail
>>>>>>> b0607f0 (atk: Redefine `detail::SoundArchiveManager`):include/nn/atk/detail/atk_SoundArchiveManager.h
