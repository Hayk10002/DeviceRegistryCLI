#pragma once

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Priority
{
public:
    Priority() : mPriority(1) {}
    Priority(int priority) : mPriority(std::clamp(priority, 1, 10)) {}
    operator int() const { return mPriority; }

private:
    int mPriority;
};

using DeviceId   = std::string;
using DeviceName = std::string;
using DeviceType = std::string;
using Tag        = std::string;

struct Device
{
    DeviceId id;
    DeviceName name;
    DeviceType type;
    std::set<Tag> tags;
    Priority priority;
    bool turnedOn = false;

    std::string AsString() const;
    std::string AsHumanReadableString() const;

    static Device FromString(std::string str);

    auto operator<=>(const Device & other) const = default;
};

class DeviceRegistry
{
public:
    DeviceRegistry() : mPriorityQueue(DeviceIdLessByDevicePriority(*this)) {}
    bool AddDevice(const Device & device);
    bool RemoveDevice(const DeviceId & id);
    Device * GetDevice(const DeviceId & id);
    const Device * GetDevice(const DeviceId & id) const;
    std::vector<Device> GetAllDevices() const;
    std::vector<Device> GetAllDevicesWithTag(const Tag & tag) const;
    void Alert();
    std::vector<DeviceId> GetPriorityQueue() const;

    struct DeviceStats
    {
        size_t totalDeviceCount;
        size_t runningDeviceCount;
        std::set<DeviceType> uniqueDeviceTypes;
        double averagePriority;
    };
    DeviceStats GetDeviceStats() const;

    enum class SetState
    {
        On,
        Off,
        Toggle
    };
    bool SetTurnedOn(const DeviceId & id, SetState state);
    bool AddToPriorityQueue(const DeviceId & id);

private:
    void UpdateUniqueDeviceTypes();

    struct DeviceIdLessByDevicePriority
    {
        DeviceRegistry & mRegistry;
        bool operator()(const DeviceId & a, const DeviceId & b)
        {
            Priority aPriority, bPriority;
            if (auto device = mRegistry.GetDevice(a); device)
            {
                aPriority = device->priority;
            }

            if (auto device = mRegistry.GetDevice(b); device)
            {
                bPriority = device->priority;
            }

            return aPriority < bPriority;
        }
    };

    std::map<DeviceId, Device> mDevices;
    std::unordered_map<Tag, std::set<DeviceId>> mTagToDevicesMap;
    std::priority_queue<DeviceId, std::vector<DeviceId>, DeviceIdLessByDevicePriority> mPriorityQueue;
    std::set<DeviceType> mUniqueDeviceTypes;
};
