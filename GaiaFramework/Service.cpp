#include "Service.hpp"
#include <thread>
#include <future>
#include <iostream>
#include <utility>
#include <tbb/tbb.h>

namespace Gaia::Framework
{
    /// Reuse a connection to the Redis server.
    Service::Service(std::string name) : Name(std::move(name)),
    MessageUpdater([this](const std::atomic_bool& life_flag){
        while (life_flag.load())
        {
            try
            {
                this->Subscriber->consume();
                auto current_time = std::chrono::system_clock::now();
                if (current_time- this->LastHeartBeatTime > std::chrono::seconds(1))
                {
                    this->NameResolver->Update();
                }
            }
            catch (sw::redis::Error& error){}
        }
    })
    {
        OptionDescription.add_options()
                ("help,?", "show help message.")
                ("host,h", boost::program_options::value<std::string>()->default_value("127.0.0.1"),
                 "ip address of the Redis server.")
                ("port,p", boost::program_options::value<unsigned int>()->default_value(6379),
                 "port of the Redis server.");
    }

    /// Update this service.
    bool Service::Update()
    {
        if (Enable)
        {
            OnUpdate();
        }
        return LifeFlag.load();
    }

    /// Install this service.
    void Service::Install()
    {
        Enable = true;

        std::unique_lock lock(CommandHandlersMutex);
        CommandHandlers.clear();
        lock.unlock();

        AddCommand("pause", [this](const std::string &content) {
            this->Enable = false;
            this->Logger->RecordMilestone("Service paused by command. " + content);
        });
        AddCommand("resume", [this](const std::string &content) {
            this->Enable = true;
            this->Logger->RecordMilestone("Service resumed by command. " + content);
        });
        AddCommand("shutdown", [this](const std::string &content) {
            this->LifeFlag = false;
            this->Logger->RecordMilestone("Service shutdown by command. " + content);
        });

        LastHeartBeatTime = std::chrono::system_clock::now();

        OnInstall();

        MessageUpdater.Start();
    }

    /// Uninstall this service.
    void Service::Uninstall()
    {
        Enable = false;
        MessageUpdater.Stop();

        OnUninstall();
    }

    /// Handle a command.
    void Service::HandleCommand(const std::string& name, const std::string &content)
    {
        std::shared_lock lock(CommandHandlersMutex);
        auto finder = CommandHandlers.find(name);
        lock.unlock();
        if (finder == CommandHandlers.end())
        {
            Logger->RecordError("Unknown command received: " + name);
            return;
        }
        if (!finder->second)
        {
            Logger->RecordError("Invalid command handler: " + name);
            return;
        }
        finder->second(content);
    }

    /// Handle a message.
    void Service::HandleMessage(const std::string &channel, const std::string &content)
    {
        std::shared_lock lock(MessageHandlersMutex);
        const auto& [begin_iterator, end_iterator] = MessageHandlers.equal_range(channel);
        lock.unlock();
        if (begin_iterator == end_iterator)
        {
            Logger->RecordError("Unknown message received: " + channel);
            return;
        }
        tbb::parallel_for_each(begin_iterator, end_iterator,
                          [&content](const std::pair<std::string, MessageHandler>& pair){
            if (pair.second) (pair.second)(content);
        });
    }

    void Service::Connect(unsigned int port, const std::string &ip)
    {
        Connection = std::make_shared<sw::redis::Redis>("tcp://" + ip + ":" + std::to_string(port));
        sw::redis::ConnectionOptions options;
        options.host = ip;
        options.port = static_cast<int>(port);
        options.socket_timeout = std::chrono::milliseconds(1000);
        RealtimeConnection = std::make_shared<sw::redis::Redis>(options);
        Subscriber = std::make_shared<sw::redis::Subscriber>(RealtimeConnection->subscriber());
        Subscriber->psubscribe(Name + "/command*");
        Subscriber->on_pmessage([this](
                const std::string& pattern, const std::string& channel, const std::string& message){
            if (channel.size() < 4) return;
            auto command_slash_index = channel.find_last_of('/');
            if (command_slash_index == std::string::npos)
            {
                this->Logger->RecordError("Error format command " + channel);
                return;
            }
            auto command_name = channel.substr(command_slash_index + 1);
            if (command_name == "command")
            {
                this->HandleCommand(message, std::string());
            }
            else
            {
                this->HandleCommand(command_name, message);
            }
        });
        Subscriber->on_message([this](const std::string& channel, const std::string& message){
            this->HandleMessage(channel, message);
        });
        Logger = std::make_unique<Clients::LogClient>(Name, Connection);
        Configurator = std::make_unique<Clients::ConfigurationClient>(Name, Connection);
        NameResolver = std::make_unique<Clients::NameClient>(Connection);
        NameResolver->RegisterName(Name);
        OnConnect();
    }

    void Service::SendServiceCommand(const std::string &service_name,
                                     const std::string &command_name, const std::string &content)
    {
        Connection->publish(service_name + "/command/" + command_name, content);
    }

    /// Add a command handler.
    void Service::AddCommand(const std::string& name, Service::MessageHandler handler)
    {
        std::unique_lock lock(CommandHandlersMutex);
        CommandHandlers.emplace(name, handler);
    }

    /// Remove a command handler.
    void Service::RemoveCommand(const std::string &name)
    {
        std::unique_lock lock(CommandHandlersMutex);
        CommandHandlers.erase(name);
    }

    /// Add a subscription to the given channel.
    void Service::AddSubscription(const std::string &channel_name, const Service::MessageHandler& handler)
    {
        std::unique_lock lock(MessageHandlersMutex);
        MessageHandlers.emplace(std::make_pair(channel_name, handler));
        Subscriber->subscribe(channel_name);
    }

    /// Remove all subscriptions to the given channel.
    void Service::RemoveSubscription(const std::string &channel_name)
    {
        Subscriber->unsubscribe(channel_name);
        std::unique_lock lock(MessageHandlersMutex);
        MessageHandlers.erase(channel_name);
    }

    /// Pause this service.
    void Service::Pause()
    {
        Enable = false;
        OnPause();
    }

    /// Resume this service.
    void Service::Resume()
    {
        Enable = true;
        OnResume();
    }
}