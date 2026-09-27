#pragma once

#include <nn/util.h>
#include <nn/util/util_BytePtr.h>

#include <nn/atk/atk_BinaryFileFormat.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_ItemType.h>

namespace nn::atk {

class SoundArchive;
class SoundArchivePlayer;
class OutputReceiver;

namespace detail {

class PlayerHeapDataManager;
class SoundArchiveLoader;
struct LoadItemInfo;
class Util {
public:
    static const int VolumeDbMin{-904};
    static const int VolumeDbMax{60};

    static const int PitchDivisionBit{8};
    static const int PitchDivisionRange{256};

    enum PanCurve {
        PanCurve_Sqrt,
        PanCurve_Sincos,
        PanCurve_Linear,
    };

    struct PanInfo {
        PanCurve curve;
        bool centerZeroFlag;
        bool zeroClampFlag;
        bool isEnableFrontBypass;
    };
    static_assert(sizeof(PanInfo) == 0x8);

    static uint16_t CalcLpfFreq(float scale);
    static BiquadFilterCoefficients CalcLowPassFilterCoefficients(int frequency, int sampleRate,
                                                                  bool isTableUsed);
    static int FindLpfFreqTableIndex(int frequency);
    static float CalcPanRatio(float pan, const PanInfo& info, OutputMode mode);
    static float CalcSurroundPanRatio(float surroundPan, const PanInfo& info);

    static float CalcPitchRatio(int pitch_);
    static float CalcVolumeRatio(float dB);
    static uint16_t CalcRandom();

    static size_t GetSampleByByte(size_t byte, SampleFormat format);
    static size_t GetByteBySample(size_t samples, SampleFormat format);

    static bool IsValidMemoryForDsp(const void* ptr, size_t size);

    static const int CalcLpfFreqTableSize{24};
    static const float CalcLpfFreqIntercept;
    static const float CalcLpfFreqThreshold;
    static const uint16_t CalcLpfFreqTable[CalcLpfFreqTableSize];
    static const BiquadFilterCoefficients LowPassFilterCoefficientsTable32000[CalcLpfFreqTableSize];
    static const BiquadFilterCoefficients LowPassFilterCoefficientsTable48000[CalcLpfFreqTableSize];

    template <typename ItemType, typename CountType = uint32_t>
    struct Table {
        CountType count;
        ItemType item[1];
    };

    struct Reference {
        static const int InvalidOffset = -1;

        bool IsValidTypeId(uint16_t validId) const { return typeId == validId; }
        bool IsValidOffset() const { return offset != InvalidOffset; }

        // Id from ElementType enum
        uint16_t typeId;
        uint8_t padding[2];
        int32_t offset;
    };
    static_assert(sizeof(Reference) == 0x8);

    struct ReferenceWithSize : public Reference {
        uint32_t size;
    };
    static_assert(sizeof(ReferenceWithSize) == 0xc);

    struct ReferenceTable : Table<Reference> {
        const void* GetReferedItem(uint32_t index) const {
            if (count <= index)
                return nullptr;

            return util::ConstBytePtr(this, item[index].offset).Get();
        }

        const void* GetReferedItem(uint32_t index, uint16_t typeId) const {
            if (count <= index || item[index].typeId != typeId)
                return nullptr;

            return util::ConstBytePtr(this, item[index].offset).Get();
        }

        const void* FindReferedItemBy(uint16_t typeId) const;
    };

    struct ReferenceWithSizeTable : Table<ReferenceWithSize> {
        const void* GetReferedItem(uint32_t index) const {
            if (count <= index)
                return nullptr;

            return util::ConstBytePtr(this, item[index].offset).Get();
        }

        const void* GetReferedItemBy(uint16_t typeId) const;
        uint32_t GetReferedItemSize(uint32_t index) const;
    };

    struct BlockReferenceTable {
        ReferenceWithSize item[1];

        const void* GetReferedItemByIndex(const void* origin, int index, uint16_t count) const;

        const ReferenceWithSize* GetReference(uint16_t typeId, uint16_t count) const {
            for (int i{0}; i < count; ++i) {
                if (item[i].IsValidTypeId(typeId))
                    return &item[i];
            }

            return nullptr;
        }

        const void* GetReferedItem(const void* origin, uint16_t typeId, uint16_t count) const {
            auto* ref{GetReference(typeId, count)};
            if (ref != nullptr && ref->offset != 0)
                return util::ConstBytePtr(origin, ref->offset).Get();

            return nullptr;
        }

        uint32_t GetReferedItemSize(uint16_t typeId, uint16_t count) const {
            auto* ref{GetReference(typeId, count)};
            if (ref != nullptr)
                return ref->size;

            return 0;
        }

        uint32_t GetReferedItemOffset(uint16_t typeId, uint16_t count) const {
            auto* ref{GetReference(typeId, count)};
            if (ref != nullptr)
                return ref->offset;

            return 0;
        }
    };

    struct SoundFileHeader {
        BinaryFileHeader header;
        BlockReferenceTable blockReferenceTable;

        int GetBlockCount() const { return header.dataBlocks; }

    protected:
        const void* GetBlock(uint16_t typeId) const {
            return blockReferenceTable.GetReferedItem(this, typeId, header.dataBlocks);
        }

        uint32_t GetBlockSize(uint16_t typeId) const {
            return blockReferenceTable.GetReferedItemSize(typeId, header.dataBlocks);
        }

        uint32_t GetBlockOffset(uint16_t typeId) const {
            return blockReferenceTable.GetReferedItemOffset(typeId, header.dataBlocks);
        }
    };

    struct BitFlag {
        uint32_t bitFlag;

        bool GetValue(uint32_t* value, uint32_t bitNumber) const {
            uint32_t count{GetTrueCount(bitNumber)};
            if (count == 0)
                return false;

            *value = (&bitFlag)[count];
            return true;
        }

        bool GetValuefloat(float* value, uint32_t bitNumber) const {
            uint32_t count{GetTrueCount(bitNumber)};
            if (count == 0)
                return false;

            *value = reinterpret_cast<const float*>(&bitFlag)[count];
            return true;
        }

    private:
        static const int BitNumberMax = 31;
        uint32_t GetTrueCount(uint32_t bitNumber) const {
            bool ret{false};
            int count{0};

            for (uint32_t i{0}; i <= bitNumber; ++i) {
                if ((bitFlag & (1 << i)) != 0) {
                    ++count;
                    if (i == bitNumber)
                        ret = true;
                }
            }

            if (ret)
                return count;

            return 0;
        }
    };
    static_assert(sizeof(BitFlag) == 0x4);

    static uint8_t DivideBy8bit(uint32_t value, int index) {
        return static_cast<uint8_t>(value >> (8 * index));
    }

    static uint16_t DivideBy16bit(uint32_t value, int index) {
        return static_cast<uint16_t>(value >> (16 * index));
    }

    static const void* GetWaveFile(uint32_t waveArchiveId, uint32_t waveIndex,
                                   const SoundArchive& arc, const SoundArchivePlayer& player);
    static const void* GetWaveFile(uint32_t waveArchiveId, uint32_t waveIndex,
                                   const SoundArchive& arc, const PlayerHeapDataManager* mgr);

    enum WaveArchiveLoadStatus {
        WaveArchiveLoadStatus_Error = -2,
        WaveArchiveLoadStatus_NotYet,
        WaveArchiveLoadStatus_Ok,
        WaveArchiveLoadStatus_Noneed,
        WaveArchiveLoadStatus_Partly
    };

    static WaveArchiveLoadStatus GetWaveArchiveOfBank(LoadItemInfo& warcLoadInfo,
                                                      bool& isLoadIndividual, const void* bankFile,
                                                      const SoundArchive& arc,
                                                      const SoundArchiveLoader& mgr);

    static const void* GetWaveFileOfWaveSound(const void* wsdFile, uint32_t index,
                                              const SoundArchive& arc,
                                              const SoundArchiveLoader& mgr);

    static ItemType GetItemType(uint32_t id) { return static_cast<ItemType>(id >> 24); }

    static uint32_t GetItemIndex(uint32_t id) { return id & 0x00FFFFFF; }

    static uint32_t GetMaskedItemId(uint32_t id, ItemType type) { return id | (type << 24); }

    struct WaveId {
        uint32_t waveArchiveId;
        uint32_t waveIndex;
    };
    static_assert(sizeof(WaveId) == 0x8);

    struct WaveIdTable {
        Table<WaveId> table;

        const WaveId* GetWaveId(uint32_t index) const {
            if (index >= table.count)
                return nullptr;

            return &table.item[index];
        }

        uint32_t GetCount() const { return table.count; }
    };

    template <typename CHILD>
    class Singleton {
        Singleton() = default;

    public:
        __attribute__((noinline)) static CHILD& GetInstance() {
            static CHILD instance;
            return instance;
        }

        friend CHILD;
    };

    static int GetSubMixBusFromMainBus();
    static int GetSubMixBus(AuxBus bus);
    static int GetSubMixBus(int bus);
    static int GetOutputReceiverMixBufferIndex(const OutputReceiver* pOutputReceiver, int bus,
                                               int channel);
    static int GetAdditionalSendIndex(int bus);

    class WarningLogger : public Singleton<WarningLogger> {
    public:
        WarningLogger() = default;

        void Log(int logId, int arg0, int arg1);
        void Print();
        void SwapBuffer();

        enum LogId {
            LogId_ChannelAllocationFailed,
            LogId_SoundthreadFailedWakeup,
            LogId_LogbufferFull,
            LogId_Max
        };

    private:
        struct LogBuffer {
            static const int LogCount{64};

            struct Element {
                int logId;
                int arg0;
                int arg1;

                void Print();
            };
            static_assert(sizeof(Element) == 0xc);

            Element element[LogCount];
            int counter{0};

            LogBuffer() = default;

            void Log(int logId, int arg0, int arg1);
            void Print();

            void Reset() { counter = 0; }
        };
        static_assert(sizeof(LogBuffer) == 0x304);

        LogBuffer m_Buffer0;
        LogBuffer m_Buffer1;
        LogBuffer* m_pCurrentBuffer{&m_Buffer0};
    };
    static_assert(sizeof(WarningLogger) == 0x610);
};

}  // namespace detail

}  // namespace nn::atk
