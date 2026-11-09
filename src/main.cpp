#include "DeviceRegistry.hpp"
#include <CLI11.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>
#include <string>
#include <vector>

struct DeviceRegistryFileIOWrapper
{
    DeviceRegistryFileIOWrapper()
    {
        std::ifstream file("devices.csv");
        if (file.is_open())
        {
            std::string countStr;
            std::getline(file, countStr);
            size_t deviceCount = std::stoi(countStr);
            std::string line;
            for (size_t i = 0; i < deviceCount && std::getline(file, line); i++)
            {
                registry.AddDevice(Device::FromString(line));
            }

            std::string priorityQueueId;
            while (std::getline(file, priorityQueueId, ','))
            {
                registry.AddToPriorityQueue(priorityQueueId);
            }
        }
    }

    ~DeviceRegistryFileIOWrapper()
    {
        std::ofstream file("devices.csv");
        auto devices = registry.GetAllDevices();
        if (file.is_open())
        {
            file << devices.size() << std::endl;
            for (auto device : devices)
            {
                file << device.AsString() << std::endl;
            }

            file << (registry.GetPriorityQueue() | std::views::join_with(',') | std::ranges::to<std::string>());
        }
    }
    DeviceRegistry registry;
};

std::unique_ptr<CLI::App> createApp(DeviceRegistry & registry)
{
    static Device configDeviceData;
    static std::vector<Tag> configTagsData;
    static Tag configTagData;
    static DeviceRegistry::SetState configStateData;

    auto app = std::make_unique<CLI::App>("Device Registration App");

    {
        auto * add = app->add_subcommand("add", "Add a device");
        add->add_option("id,-i,--id", configDeviceData.id)->required();
        add->add_option("name,-n,--name", configDeviceData.name)->required();
        add->add_option("type,-t,--type", configDeviceData.type)->required();
        add->add_option("priority,-p,--priority", configDeviceData.priority);
        add->add_option("--tags", configTagsData);
        add->callback([&] mutable {
            configDeviceData.tags = configTagsData | std::ranges::to<std::set<Tag>>();
            if (registry.AddDevice(configDeviceData))
            {
                std::println("Device added successfully");
            }
            else
            {
                std::println("Device addition failed. Device with specified id already exists");
                throw CLI::RuntimeError();
            }
        });
    }

    {
        auto * remove = app->add_subcommand("remove", "Remove a device");
        remove->add_option("id,-i,--id", configDeviceData.id)->required();
        remove->callback([&] {
            if (registry.RemoveDevice(configDeviceData.id))
            {
                std::println("Device removed successfully");
            }
            else
            {
                std::println("Device removal failed. No device with specified id found");
                throw CLI::RuntimeError();
            }
        });
    }

    {
        auto * list = app->add_subcommand("list", "List all devices");
        list->callback([&] {
            auto devices = registry.GetAllDevices();
            std::ranges::sort(devices, [](const Device & left, const Device & right) { return left.name < right.name; });
            for (const auto & device : devices)
            {
                std::cout << device.AsHumanReadableString() << std::endl;
            }
        });
    }

    {
        auto * listByPriority = app->add_subcommand("list-by-priority", "List all devices sorted by priority");
        listByPriority->callback([&] {
            auto devices = registry.GetAllDevices();
            std::ranges::sort(devices, [](const Device & left, const Device & right) { return left.priority < right.priority; });
            for (const auto & device : devices)
            {
                std::cout << device.AsHumanReadableString() << std::endl;
            }
        });
    }

    {
        auto * listWithTag = app->add_subcommand("list-with-tag", "List all devices with the specified tag");
        listWithTag->add_option("tag,-t,--tag", configTagData)->required();
        listWithTag->callback([&] {
            auto devicesWithTag = registry.GetAllDevicesWithTag(configTagData);
            std::ranges::sort(devicesWithTag, [](const Device & left, const Device & right) { return left.name < right.name; });
            for (const auto & device : devicesWithTag)
            {
                std::cout << device.AsHumanReadableString() << std::endl;
            }
        });
    }

    {
        auto * listHighPriority =
            app->add_subcommand("list-high-priority", "List all devices with the specified priority or higher");
        listHighPriority->add_option("priority,-p,--priority", configDeviceData.priority);
        listHighPriority->callback([&] {
            auto devices = registry.GetAllDevices();
            std::vector<Device> matchingDevices{};
            std::ranges::copy_if(devices, std::back_inserter(matchingDevices),
                                 [&](const Device & device) { return device.priority >= configDeviceData.priority; });
            for (const auto & device : matchingDevices)
            {
                std::cout << device.AsHumanReadableString() << std::endl;
            }
        });
    }

    {
        auto * alert = app->add_subcommand(
            "alert", "Alert (print) top 3 devices with the highest priorities in the alert queue, and remove them from it");
        alert->callback([&] { registry.Alert(); });
    }

    {
        auto * stats = app->add_subcommand("stats", "Show some statistics about the registry");
        stats->callback([&] {
            auto deviceStats = registry.GetDeviceStats();

            std::print("Total devices: {}\n"
                       "Running devices: {}\n"
                       "Unique device types: {}\n"
                       "Average priority: {}\n",
                       deviceStats.totalDeviceCount, deviceStats.runningDeviceCount, deviceStats.uniqueDeviceTypes,
                       deviceStats.averagePriority);
        });
    }

    {
        auto * addToAlertQueue = app->add_subcommand("add-to-alert-queue", "Add a device to the alert queue");
        addToAlertQueue->add_option("id,-i,--id", configDeviceData.id)->required();
        addToAlertQueue->callback([&] {
            if (registry.AddToPriorityQueue(configDeviceData.id))
            {
                std::println("Device successfully added to alert queue");
            }
            else
            {
                std::println("Device addition to alert queue failed: No device with specified id found");
                throw CLI::RuntimeError();
            }
        });
    }

    {
        using enum DeviceRegistry::SetState;
        auto * setState = app->add_subcommand("set-state", "Set the state of a device");
        setState->add_option("id,-i,--id", configDeviceData.id)->required();
        setState->add_option("state,-s,--state", configStateData)
            ->transform(CLI::CheckedTransformer(
                std::map<std::string, DeviceRegistry::SetState>{ { "on", On }, { "off", Off }, { "toggle", Toggle } },
                CLI::ignore_case))
            ->required();
        setState->callback([&] {
            if (registry.SetTurnedOn(configDeviceData.id, configStateData))
            {
                std::println("Device state successfully changed");
            }
            else
            {
                std::println("Device state change failed. No device with specified id found");
                throw CLI::RuntimeError();
            }
        });
    }

    return app;
}

template <class T>
int runCommand(DeviceRegistry & registry, T arguments)
{
    auto app = createApp(registry);

    CLI11_PARSE(*app, arguments);
    return 0;
}

int main(int argc, char ** argv)
{
    DeviceRegistryFileIOWrapper reg;
    if (argc > 1)
    {
        std::vector<std::string> arguments{ argv + 1, argv + argc };
        std::ranges::reverse(arguments);
        return runCommand(reg.registry, arguments);
    }

    std::cout << "Interactive mode.\nEnter -h or --help for help.\nEnter a command or type quit." << std::endl;

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line))
    {
        if (line == "quit")
        {
            break;
        }

        runCommand(reg.registry, line);
    }
}
