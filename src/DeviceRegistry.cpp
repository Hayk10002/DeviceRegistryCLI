#include "DeviceRegistry.hpp"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

std::string Device::AsString() const
{
    return std::format("{},{},{},{},{},{}", id, name, type, static_cast<int>(priority), turnedOn,
                       tags | std::views::join_with(':') | std::ranges::to<std::string>());
}

std::string Device::AsHumanReadableString() const
{
    return std::format("Device {}:\n"
                       "  Id: {}\n"
                       "  Type: {}\n"
                       "  Priority: {}\n"
                       "  Tags: {}\n"
                       "  CurrentlyRunning: {}\n",
                       name, id, type, static_cast<int>(priority),
                       tags | std::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>(), turnedOn);
};

Device Device::FromString(std::string str)
{
    std::istringstream ss(str);
    Device device;
    std::string priorityStr, tagsStr, turnedOnStr;

    std::getline(ss, device.id, ',');
    std::getline(ss, device.name, ',');
    std::getline(ss, device.type, ',');
    std::getline(ss, priorityStr, ',');
    std::getline(ss, turnedOnStr, ',');
    std::getline(ss, tagsStr, ',');

    device.priority = std::stoi(priorityStr);
    device.turnedOn = (turnedOnStr == "true");
    device.tags     = tagsStr | std::views::split(':') | std::ranges::to<std::set<Tag>>();

    return device;
}

bool DeviceRegistry::AddDevice(const Device & device)
{
    if (mDevices.find(device.id) != mDevices.end())
    {
        return false;
    }

    mDevices[device.id] = device;
    std::ranges::for_each(device.tags, [&](const Tag & tag) { mTagToDevicesMap[tag].insert(device.id); });
    UpdateUniqueDeviceTypes();
    return true;
}

bool DeviceRegistry::RemoveDevice(const DeviceId & id)
{
    auto it = mDevices.find(id);
    if (it == mDevices.end())
        return false;

    auto & device = it->second;
    std::ranges::for_each(device.tags, [&](const Tag & tag) {
        mTagToDevicesMap[tag].erase(id);
        if (mTagToDevicesMap[tag].empty())
        {
            mTagToDevicesMap.erase(tag);
        }
    });
    mDevices.erase(id);
    UpdateUniqueDeviceTypes();
    return true;
}

Device * DeviceRegistry::GetDevice(const DeviceId & id)
{
    auto it = mDevices.find(id);
    if (it == mDevices.end())
        return nullptr;

    return &it->second;
}

const Device * DeviceRegistry::GetDevice(const DeviceId & id) const
{
    auto it = mDevices.find(id);
    if (it == mDevices.end())
        return nullptr;

    return &it->second;
}

std::vector<Device> DeviceRegistry::GetAllDevices() const
{
    return mDevices | std::views::values | std::ranges::to<std::vector>();
}

std::vector<Device> DeviceRegistry::GetAllDevicesWithTag(const Tag & tag) const
{
    if (!mTagToDevicesMap.contains(tag))
        return std::vector<Device>{};

    return mTagToDevicesMap.at(tag) | std::views::transform([&](const DeviceId & id) { return mDevices.at(id); }) |
        std::ranges::to<std::vector>();
}

void DeviceRegistry::Alert()
{
    for (int i = 0; i < 3 && !mPriorityQueue.empty(); i++)
    {
        const auto & device = GetDevice(mPriorityQueue.top());
        if (device)
        {
            std::cout << device->AsHumanReadableString();
        }
        mPriorityQueue.pop();
    }
}

DeviceRegistry::DeviceStats DeviceRegistry::GetDeviceStats() const
{
    DeviceStats stats;
    stats.totalDeviceCount = mDevices.size();
    stats.runningDeviceCount =
        std::ranges::count_if(mDevices | std::views::values, [](const Device & dev) { return dev.turnedOn; });
    stats.uniqueDeviceTypes = mUniqueDeviceTypes;

    stats.averagePriority = mDevices.empty()
        ? 0.0
        : std::ranges::fold_left(mDevices | std::views::values | std::views::transform([](const Device & device) {
                                     return static_cast<double>(device.priority);
                                 }),
                                 0.0, std::plus<double>{}) /
            mDevices.size();

    return stats;
}

bool DeviceRegistry::SetTurnedOn(const DeviceId & id, SetState state)
{
    if (auto device = GetDevice(id); device)
    {
        using enum SetState;
        switch (state)
        {
        case On:
            device->turnedOn = true;
            break;
        case Off:
            device->turnedOn = false;
            break;
        case Toggle:
            device->turnedOn = !device->turnedOn;
            break;
        }
        return true;
    }
    return false;
}

bool DeviceRegistry::AddToPriorityQueue(const DeviceId & id)
{
    if (GetDevice(id))
    {
        mPriorityQueue.push(id);
        return true;
    }
    return false;
}

void DeviceRegistry::UpdateUniqueDeviceTypes()
{
    mUniqueDeviceTypes.clear();
    std::ranges::for_each(mDevices | std::views::values, [&](const Device & device) { mUniqueDeviceTypes.insert(device.type); });
}

std::vector<DeviceId> DeviceRegistry::GetPriorityQueue() const
{
    auto copy = mPriorityQueue;

    std::vector<DeviceId> queue;
    queue.reserve(mPriorityQueue.size());
    while (!copy.empty())
    {
        auto id = copy.top();
        copy.pop();
        queue.push_back(id);
    }

    return queue;
}
