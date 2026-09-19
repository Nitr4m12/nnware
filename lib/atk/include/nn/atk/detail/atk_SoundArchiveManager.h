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
    class SnapShot {
    public:
        SnapShot(const SoundArchive& mainSoundArchive, const SoundDataManager& mainSoundDataManager,
                 const SoundArchive& currentSoundArchive,
                 const SoundDataManager& currentSoundDataManager)
            : m_MainSoundArchive{mainSoundArchive}, m_MainSoundDataManager{mainSoundDataManager},
              m_CurrentSoundArchive{currentSoundArchive},
              m_CurrentSoundDataManager{currentSoundDataManager} {}

        const SoundArchive& GetMainSoundArchive() const { return m_MainSoundArchive; }

        const SoundDataManager& GetMainSoundDataManager() const { return m_MainSoundDataManager; }

        const SoundArchive& GetCurrentSoundArchive() const { return m_CurrentSoundArchive; }

        const SoundDataManager& GetCurrentSoundDataManager() const {
            return m_CurrentSoundDataManager;
        }

    private:
        const SoundArchive& m_MainSoundArchive;
        const SoundDataManager& m_MainSoundDataManager;
        const SoundArchive& m_CurrentSoundArchive;
        const SoundDataManager& m_CurrentSoundDataManager;
    };

    SoundArchiveManager();
    ~SoundArchiveManager();

    void Initialize(const SoundArchive* pSoundArchive, const SoundDataManager* pSoundDataManager);
    void Finalize();

    void Add(AddonSoundArchiveContainer& container);
    void Remove(AddonSoundArchiveContainer& container);
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

    void ChangeTargetArchive(const char* soundArchiveName);

    SnapShot GetSnapShot() const {
        SnapShot snapShot{*m_pMainSoundArchive, *m_pMainSoundDataManager, *m_pCurrentSoundArchive,
                          *m_pCurrentSoundDataManager};
        return snapShot;
    }

    const SoundArchive* GetMainSoundArchive() const { return m_pMainSoundArchive; }
    const SoundDataManager* GetMainSoundDataManager() const { return m_pMainSoundDataManager; }

    const SoundArchive* GetCurrentSoundArchive() const { return m_pCurrentSoundArchive; }
    const SoundDataManager* GetCurrentSoundDataManager() const {
        return m_pCurrentSoundDataManager;
    }

    const AddonSoundArchive* GetAddonSoundArchive(const char* soundArchiveName) const;
    int GetAddonSoundArchiveCount() const { return m_ContainerList.Count(); }

    const AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(int index) const;
    AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(int index);

    const SoundDataManager* GetAddonSoundDataManager(const char* soundArchiveName) const;

    void SetParametersHook(SoundArchiveParametersHook* parametersHook);
    SoundArchiveParametersHook* GetParametersHook() const { return m_pParametersHook; }

private:
    using ContainerList = IntrusiveList<AddonSoundArchiveContainer>;

    const SoundArchive* m_pMainSoundArchive{};
    const SoundDataManager* m_pMainSoundDataManager{};
    ContainerList m_ContainerList;
    const SoundArchive* m_pCurrentSoundArchive{};
    const SoundDataManager* m_pCurrentSoundDataManager{};
    SoundArchiveParametersHook* m_pParametersHook;
};
static_assert(sizeof(SoundArchiveManager) == 0x38);

}  // namespace nn::atk::detail
>>>>>>> b0607f0 (atk: Redefine `detail::SoundArchiveManager`):include/nn/atk/detail/atk_SoundArchiveManager.h
