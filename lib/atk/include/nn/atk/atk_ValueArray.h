#pragma once

#include <algorithm>
#include <cstddef>

namespace nn::atk::detail {

template <typename T>
class ValueArray {
public:
    static size_t GetRequiredMemSize(int count) { return count * sizeof(T); }

    ValueArray() = default;

    bool Initialize(void* buffer, [[maybe_unused]] size_t bufferSize, int count) {
        m_pValue = static_cast<T*>(buffer);
        m_Count = count;
        Reset();
        return true;
    }

    // NOT_CHECKED
    bool IsInitialized() const { return m_pValue != nullptr && m_Count != 0; }

    // NOT_CHECKED
    void* GetBufferAddr() { return m_pValue; }

    void Finalize() {
        m_pValue = nullptr;
        m_Count = 0;
    }

    void Reset() {
        for (int i{0}; i < m_Count; ++i)
            m_pValue[i] = 0;
    }

    // NOT_CHECKED
    int GetCount() const { return m_Count; }

    void SetValue(int index, T value) { m_pValue[index] = value; }

    T GetValue(int index) const { return m_pValue[index]; }

    // NOT_CHECKED
    bool TrySetValue(int index, T value) {
        if (index >= m_Count)
            return false;

        m_pValue[index] = value;
        return true;
    }

    T TryGetValue(int index) const {
        if (index >= m_Count)
            return 0;

        return m_pValue[index];
    }

    ValueArray& operator=(const ValueArray& rhs) {
        const int CopyCount{std::min(m_Count, rhs.m_Count)};

        for (int i{0}; i < CopyCount; ++i)
            SetValue(i, rhs.GetValue(i));

        for (int i{CopyCount}; i < m_Count; ++i)
            SetValue(i, 0);

        return *this;
    }

private:
    T* m_pValue{};
    int m_Count{0};
};

}  // namespace nn::atk::detail
