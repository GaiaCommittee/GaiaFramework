#pragma once

#include <string>
#include <optional>
#include <memory>
#include <sw/redis++/redis++.h>
#include <boost/lexical_cast.hpp>

namespace Gaia::Framework::Clients
{
    /**
     * @brief Client for accessing, updating and removing configuration items.
     */
    class ConfigurationClient
    {
    protected:
        /**
         * @brief Combine the unit name and item name to get the key name of this configuration item.
         * @param unit_name Name of this configuration unit.
         * @param item_name Name of the configuration item.
         * @return The key name of the desired configuration item.
         */
        static std::string GenerateKeyName(const std::string& unit_name, const std::string& item_name);

        /// Name of the bound configuration unit.
        const std::string UnitName;

        /// Connection to the Redis server.
        std::shared_ptr<sw::redis::Redis> Connection;

    public:
        /**
         * @brief Connect to the given Redis server and bind the given configuration unit.
         * @param unit_name Name of the configuration unit to bind.
         * @param port Port of the Redis server.
         * @param ip IP address of the Redis server.
         */
        explicit ConfigurationClient(const std::string& unit_name,
                                     unsigned int port = 6379, const std::string& ip = "127.0.0.1");

        /**
         * @brief Reuse the connection to a Redis server.
         * @param unit_name Name of the configuration unit to bind.
         * @param connection The connection to a Redis server.
         */
        explicit ConfigurationClient(std::string unit_name,
                                     std::shared_ptr<sw::redis::Redis> connection);

        /**
         * @brief Get the string value of the given configuration item.
         * @param name The name of the configuration item to get.
         * @return The string value of the given configuration item,
         *         or std::nullopt if the corresponding value does not exist.
         */
        std::optional<std::string> Get(const std::string& name);

        /**
         * @brief Get the value of the given configuration item and cast into desired type.
         * @tparam ValueType The type of the value to cast into.
         * @param name The name of the configuration item to get.
         * @return The casted value of the given configuration item,
         *         or std::nullopt if the corresponding value does not exist.
         */
        template <typename ValueType>
        std::optional<ValueType> Get(const std::string& name)
        {
            auto result = Get(name);
            if (!result.has_value()) return std::nullopt;
            return boost::lexical_cast<ValueType>(*result);
        }

        /**
         * @brief Update or add the value of the given configuration item.
         * @param name The name of the configuration item.
         * @param value The value to add or update.
         */
        void Set(const std::string& name, const std::string& value);

        /**
         * @brief Update or add the value of the given configuration item.
         * @tparam ValueType The type of the value.
         * @param name The name of the configuration item.
         * @param value The value to add or update.
         */
        template <typename ValueType>
        void Set(const std::string& name, ValueType value)
        {
            Set(name, std::to_string(value));
        }

        /// Reload the configuration from the JSON file into the Redis server.
        void Reload();
        /// Apply the configuration in the Redis server to a JSON file.
        void Apply();
    };
}