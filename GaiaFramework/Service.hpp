#pragma once

#include "Clients/LogClient.hpp"
#include "Clients/ConfigurationClient.hpp"
#include "Clients/NameClient.hpp"
#include <sw/redis++/redis++.h>
#include <string>
#include <chrono>
#include <functional>
#include <optional>
#include <list>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <GaiaBackground/GaiaBackground.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

namespace Gaia::Framework
{
    class Service
    {
         template <typename, typename... ArgumentTypes>
         friend void Launch(int, char**, ArgumentTypes... constructor_arguments);

    public:
        using MessageHandler = std::function<void(const std::string&)>;

    private:
        /// Mutex for commands map.
        std::shared_mutex CommandHandlersMutex;
        /// Command handlers map.
        std::unordered_map<std::string, MessageHandler> CommandHandlers;
        /// Mutex for messages map.
        std::shared_mutex MessageHandlersMutex;
        /// Message handlers map.
        std::unordered_multimap<std::string, MessageHandler> MessageHandlers;
        /// Handle a command.
        void HandleCommand(const std::string& name, const std::string& content);
        /// Handle a message.
        void HandleMessage(const std::string& channel, const std::string& content);

        /// Time point of last name service update.
        std::chrono::system_clock::time_point LastHeartBeatTime;

    private:
        /// Connection to the Redis server.
        std::shared_ptr<sw::redis::Redis> Connection;
        /// Communicator for Redis message service.
        std::shared_ptr<sw::redis::Subscriber> Subscriber;
        /// Log service client.
        std::unique_ptr<Clients::LogClient> Logger {nullptr};
        /// Configuration service client.
        std::unique_ptr<Clients::ConfigurationClient> Configurator {nullptr};
        /// Name service client.
        std::unique_ptr<Clients::NameClient> NameResolver {nullptr};

    protected:
        /**
         * @brief Send a command to a service and receive the result.
         * @param service_name Name of the target service.
         * @param command_name Name of the service command.
         * @param content Content for the command request.
         */
        void SendServiceCommand(const std::string& service_name, const std::string& command_name,
                                const std::string& content = "");

        /**
         * @brief Set the value of the given value.
         * @tparam ValueType Type of the value to set.
         * @param name Name of the value.
         * @param value Value to set.
         */
        template <typename ValueType>
        void SetRemoteValue(const std::string& name, ValueType value)
        {
            if (Connection)
            {
                Connection->set(name, std::to_string(value));
            }
        }

        /**
         * @brief Set the value of the given remote value.
         * @tparam ValueType Type of the value to set.
         * @param name Name of the value to set.
         * @param value Value to set.
         * @param lasting_seconds After this amount of seconds, this value will expire.
         */
        template <typename ValueType>
        void SetRemoteValue(const std::string& name, ValueType value, std::chrono::seconds lasting_seconds)
        {
            if (Connection)
            {
                Connection->set(name, std::to_string(value), lasting_seconds);
            }
        }

        /**
         * @brief Check whether the remove value exists or not.
         * @param name Name of the value to check.
         * @retval true Remote value with the given name exists.
         * @retval false Remote value with the given name does not exist.
         */
        bool HasRemoteValue(const std::string& name)
        {
            if (Connection) return Connection->exists(name);
            return false;
        }

        /**
         * @brief Get the value of a remote value.
         * @tparam ValueType Type of the value.
         * @param name Name of the value.
         * @return std::nullopt, if the desired value does not exist or fail to be converted into the given type;
         *         otherwise std::optional that contains the value in the desired type.
         */
        template <typename ValueType>
        std::optional<ValueType> GetRemoteValue(const std::string& name)
        {
            if (Connection)
            {
                auto optional_text = Connection->get(name);
                if (optional_text.has_value())
                    try{
                        return {boost::lexical_cast<ValueType>(optional_text.value())};
                    }catch (boost::bad_lexical_cast& error){}
            }
            return std::nullopt;
        }

        /**
         * @brief Add a command handler to support a command.
         * @param name Name of the command.
         * @param handler Handler functor.
         */
        void AddCommand(const std::string& name, MessageHandler handler);

        /// Remove the handler of the given command.
        void RemoveCommand(const std::string& name);

        /**
         * @brief Add a subscription to a specific channel.
         * @param channel_name Name of the channel to subscribe.
         * @param handler Handler for messages from the channel.
         */
        void AddSubscription(const std::string& channel_name, const MessageHandler& handler);
        /**
         * @brief Remove the subscriptions to the given channel.
         * @param channel_name Name of the channel.
         * @attention All subscriptions to the given channel will be removed.
         */
        void RemoveSubscription(const std::string& channel_name);

        /// Get connection of this service.
        [[nodiscard]] inline const std::shared_ptr<sw::redis::Redis>& GetConnection() const noexcept
        {
            return Connection;
        }
        /// Get communicator of this service.
        [[nodiscard]] inline sw::redis::Subscriber* GetCommunicator() const noexcept
        {
            return Subscriber.get();
        }
        /// Get log client of this service.
        [[nodiscard]] inline Clients::LogClient* GetLogger() const noexcept
        {
            return Logger.get();
        }
        /// Get configuration client of this service.
        [[nodiscard]] inline Clients::ConfigurationClient* GetConfigurator() const noexcept
        {
            return Configurator.get();
        }
        /// Get name client of this service.
        [[nodiscard]] inline Clients::NameClient* GetNameResolver() const noexcept
        {
            return NameResolver.get();
        }

        /**
         * @brief Description for all program options supported by this service.
         * @details
         *  Values passed to this program can be got from OptionVariables in
         *  OnInstall() stage.
         */
        boost::program_options::options_description OptionDescription;
        /// Map that stores values parsed from the program command line.
        boost::program_options::variables_map OptionVariables;

        /// Invoked when this service is connected to a Redis server.
        virtual void OnConnect() {};

        /// Invoked when install this service.
        virtual void OnInstall() {};
        /// Invoked when uninstall this service.
        virtual void OnUninstall() {};
        /**
         * @brief Invoked in every frame.
         * @retval true Launcher should keep program alive.
         * @retval false Launcher should stop the program.
         */
        virtual void OnUpdate() {};

        /// If false, service program will stop in next frame.
        std::atomic_bool LifeFlag {true};

        /// Constructor that will bind the service name.
        explicit Service(std::string name);

    public:
        /// Name of this service.
        const std::string Name;
        /**
         * @brief Enable of this service.
         * @details
         *  If false, this service will not be updated frequently,
         *  and message will not be consumed.
         */
        std::atomic_bool Enable {true};

        /**
         * @brief Establish a connection to the Redis server.
         * @param port Port of the Redis server.
         * @param ip IP of the Redis server.
         */
        void Connect(unsigned int port = 6379, const std::string& ip = "127.0.0.1");
        /**
         * @brief Install this service.
         */
        void Install();
        /**
         * @brief Update this service and consume Redis messages in parallel.
         * @retval true Launcher should continue main loop.
         * @retval false Launcher should stop the program.
         */
        bool Update();
        /**
         * @brief Uninstall this service.
         */
        void Uninstall();
    };
}

#ifndef CONSTRUCTOR
#define CONSTRUCTOR(Name, Arguments...) public: Name(##Arguments) : Gaia::Framework::Service(#Name)
#endif