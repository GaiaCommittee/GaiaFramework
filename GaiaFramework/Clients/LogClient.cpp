#include "LogClient.hpp"

#include <iostream>
#include <utility>

#ifndef LOG_SERVICE_CHANNEL
#define LOG_SERVICE_CHANNEL "logs/record"
#endif

namespace Gaia::Framework::Clients
{
    /// Try to connect to the Redis server, and it will use a local file instead if failed.
    LogClient::LogClient(std::string author, unsigned int port, const std::string &ip) :
        Author(std::move(author))
    {
        try
        {
            Connection = std::make_unique<sw::redis::Redis>("tcp://" + ip + ":" + std::to_string(port));
            if (Connection->publish(LOG_SERVICE_CHANNEL,
                                    LogRecorder::GenerateLogText("Log service client connected.",
                                                                 LogRecorder::Severity::Message, Author)) <1)
            {
                throw std::runtime_error("No log server detected on " + ip + ":" + std::to_string(port));
            }
        }catch (std::exception& error)
        {
            Connection.reset();
            SwitchToOfflineMode("No log service detected.");
        }
    }

    /// Reuse the connection to a Redis server.
    /// Reuse the connection to a Redis server.
    LogClient::LogClient(std::string author, std::shared_ptr<sw::redis::Redis> connection) :
        Author(std::move(author)), Connection(std::move(connection))
    {
        if (!Connection)
        {
            Logger = std::make_unique<LogRecorder>(Author);
        }

        try
        {
            if (Connection->publish(LOG_SERVICE_CHANNEL,
                                    LogRecorder::GenerateLogText("Log service client connected.",
                                                                 LogRecorder::Severity::Message, Author)) <1)
            {
                throw std::runtime_error("No log server detected.");
            }
        }catch (std::exception& error)
        {
            Connection.reset();
            SwitchToOfflineMode("No log service detected.");
        }
    }

    /// Record a raw text into the log.
    void LogClient::RecordRawText(const std::string& text)
    {
        if (Connection)
        {
            Connection->publish(LOG_SERVICE_CHANNEL, text);
        }
        else if (Logger)
        {
            Logger->RecordRawText(text);
        }

        if (PrintToConsole)
        {
            std::cout << text << std::endl;
        }
    }

    /// Record a message log.
    void LogClient::RecordMessage(const std::string& text)
    {
        RecordRawText(LogRecorder::GenerateLogText(text, LogRecorder::Severity::Message, Author));
    }

    /// Record a milestone log.
    void LogClient::RecordMilestone(const std::string& text)
    {
        RecordRawText(LogRecorder::GenerateLogText(text, LogRecorder::Severity::Milestone, Author));
    }

    /// Record a warning log.
    void LogClient::RecordWarning(const std::string& text)
    {
        RecordRawText(LogRecorder::GenerateLogText(text, LogRecorder::Severity::Warning, Author));
    }

    /// Record an error log.
    void LogClient::RecordError(const std::string& text)
    {
        RecordRawText(LogRecorder::GenerateLogText(text, LogRecorder::Severity::Error, Author));
    }

    /// Switch to offline mode.
    void LogClient::SwitchToOfflineMode(const std::string& reason)
    {
        if (Connection) Connection.reset();
        if (!Logger)
        {
            Logger = std::make_unique<LogRecorder>(Author);
            Logger->PrintToConsole = PrintToConsole;
            if (reason.empty())
            {
                RecordMilestone("Switch to offline log mode.");
            }
            else
            {
                RecordMilestone("Switch to offline log mode, reason: " + reason);
            }
        }
    }

    /// Set whether auto print logs to console or not.
    void LogClient::SetPrintToConsole(bool enable)
    {
        PrintToConsole = enable;
        if (Logger)
        {
            Logger->PrintToConsole = enable;
        }
    }
}