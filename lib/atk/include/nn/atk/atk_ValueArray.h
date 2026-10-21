#pragma once

#include <algorithm>
#include <cstddef>

namespace nn::atk::detail {

template <typename T>
class ValueArray {
public:
    static size_t GetRequiredMemSize(int count) {
        return count * sizeof(T);
    }

    // UNCHECKED
    ValueArray() = default;

    bool Initialize(void* buffer, [[maybe_unused]] size_t bufferSize, int count) {
        m_pValue = static_cast<T*>(buffer);
        m_Count = count;
        Reset();
        return true;
    }

    // UNCHECKED
    bool IsInitialized() const { return m_pValue != nullptr && m_Count != 0; }

    // UNCHECKED
    void* GetBufferAddr() { return m_pValue; }

    // UNCHECKED
    void Finalize() {
        m_pValue = nullptr;
        m_Count = 0;
    }

    void Reset() {
        for (int i{0}; i < m_Count; ++i)
            m_pValue[i] = 0;
    }

    // UNCHECKED
    int GetCount() const { return m_Count; }

    // UNCHECKED
    void SetValue(int index, T value) { m_pValue[index] = value; }

    // UNCHECKED
    T GetValue(int index) const { return m_pValue[index]; }

    // UNCHECKED
    bool TrySetValue(int index, T value) {
        if (index >= m_Count)
            return false;

        m_pValue[index] = value;
        return true;
    }

    // UNCHECKED
    T TryGetValue(int index) const {
        if (index >= m_Count)
            return 0;

        return m_pValue[index];
    }

    // UNCHECKED
    ValueArray& operator=(const ValueArray& rhs) {
        const int CopyCount{std::min(m_Count, rhs.m_Count)};

        for (int i{0}; i < CopyCount; ++i)
            this->SetValue(i, rhs.GetValue(i));

        for (int i{rhs.m_Count}; i > 0; --i)
            rhs.SetValue(i, 0);

        return *this;
    }

private:
    T* m_pValue{};
    int m_Count{0};
};

}  // namespace nn::atk::detail
