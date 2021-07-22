#include "ConfigurationClient.hpp"

#include <sstream>
#include <exception>

namespace Gaia::Framework::Clients
{
    /// Combine the unit name and item name to get the key name of this configuration item.
    std::string ConfigurationClient::GenerateKeyName(const std::string &unit_name, const std::string &item_name)
    {
        std::stringstream key_builder;
        key_builder << "configurations/" << unit_name << "/" << item_name;
        return key_builder.str();
    }

    /// Establish a connection to the Redis server on the given port and ip address.
    ConfigurationClient::ConfigurationClient(const std::string& unit_name, unsigned int port, const std::string &ip) :
        ConfigurationClient(unit_name,
                            std::make_shared<sw::redis::Redis>(
                                    "tcp://" + ip + ":" + std::to_string(port)))
    {}


    /// Reuse the connection to a Redis server.
    ConfigurationClient::ConfigurationClient(std::string unit_name,
                                             std::shared_ptr<sw::redis::Redis> connection) :
        Connection(std::move(connection)), UnitName(std::move(unit_name))
    {}

    /// Get the string value of the given configuration item.
    std::optional<std::string> ConfigurationClient::Get(const std::string& name)
    {
        return Connection->get(GenerateKeyName(UnitName, name));
    }

    /// Update or add the value of the given configuration item.
    void ConfigurationClient::Set(const std::string &name, const std::string &value)
    {
        Connection->set(GenerateKeyName(UnitName, name), value);
    }

    /// Reload the configuration from the JSON file into the Redis server.
    void ConfigurationClient::Reload()
    {
        Connection->publish("configurations/load", UnitName);
    }

    /// Apply the configuration in the Redis server to a JSON file.
    void ConfigurationClient::Apply()
    {
        Connection->publish("configurations/save", UnitName);
    }
}