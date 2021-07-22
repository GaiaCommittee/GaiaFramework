#pragma once

#include <string>
#include <unordered_set>
#include <memory>
#include <future>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include <sw/redis++/redis++.h>

namespace Gaia::Framework::Clients
{
    /**
     * @brief NameClient for name resolving service.
     * @details
     *  This client instance represents a connection to a Redis server.-
     */
    class NameClient
    {
    protected:
        /// Connection to Redis server, default address is '127.0.0.1:6379'
        std::shared_ptr<sw::redis::Redis> Connection;

    private:
        /// Update the timestamp of a name to keep it valid.
        void UpdateName(const std::string& name, const std::string& address);

        /// Mutex for names list.
        std::shared_mutex NamesMutex;
        /// Names to update.
        std::unordered_map<std::string, std::string> Names;

    public:
        /**
         * @brief Construct and try to connect to the Redis server on the given address.
         * @param port The port of the redis server.
         * @param ip The ip of the redis server.
         */
        explicit NameClient(unsigned int port = 6379, const std::string& ip = "127.0.0.1");
        /// Reuse the connection to a Redis server.
        explicit NameClient(std::shared_ptr<sw::redis::Redis> connection);

        /**
         * @brief Query all registered names.
         * @return Set of valid names which have not been expired yet.
         * @attention This is a time consuming function.
         */
        std::unordered_set<std::string> GetNames();

        /**
         * @brief Query whether a name is valid or not.
         * @retval true The given name is online.
         * @retval false The given name does not exist.
         * @attention This is a time consuming function.
         */
        bool IsNameValid(const std::string& name);

        /**
         * @brief Register a name and add it to the update list.
         * @param name Name to register.
         * @param address Address corresponding to the name.
         */
        void RegisterName(const std::string& name, const std::string& address = "");

        /**
         * @brief Unregister a name and remove it from the update list.
         * @param name Name to unregister.
         */
        void UnregisterName(const std::string& name);

        /// Update the expiration time of names in the update list.
        void Update();

        /**
         * @brief Query the address text of the given name.
         * @param name The name to query.
         * @return The address text of given name, maybe empty.
         */
        std::string QueryAddress(const std::string& name);
    };
}
