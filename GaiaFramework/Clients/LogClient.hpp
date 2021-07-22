#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <sw/redis++/redis++.h>

#include "LogRecorder.hpp"

namespace Gaia::Framework::Clients
{
    /**
     * @brief Client for the log service, provides log recording functions.
     * @details
     *  If it failed to connect to the given Redis server,
     *  it will create a local log file and use it instead.
     */
    class LogClient
    {
    private:
        /// Local file log recorder.
        std::unique_ptr<LogRecorder> Logger;
        /// Remote log service connection.
        std::shared_ptr<sw::redis::Redis> Connection;

        /// Record a raw text into the log.
        void RecordRawText(const std::string& text);

        /// The author of the logs.
        std::string Author {"Anonymous"};

        /// Whether logs will be printed to the console or not.
        bool PrintToConsole {false};

    public:
        /**
         * @brief Set whether print the log to the console or not.
         * @param enable
         *  true: Log text will be printed to the console.
         *  false: Log text will not be printed to the console.
         */
        void SetPrintToConsole(bool enable);

        /**
         * @brief Try to connect to the Redis server, and it will use a local file instead if failed.
         * @param port Port of the Redis server.
         * @param ip IP address of the Redis server.
         */
        explicit LogClient(std::string author, unsigned int port = 6379, const std::string& ip = "127.0.0.1");
        /**
         * @brief Reuse the connection to a Redis server.
         * @param connection Connection to the Redis server.
         * @details If connection is null, then this client will be initialized in offline mode.
         */
        explicit LogClient(std::string author, std::shared_ptr<sw::redis::Redis> connection);

        /// Switch to the offline mode, will use a local log file.
        void SwitchToOfflineMode(const std::string& reason = "");

        /**
         * @brief Record a message log.
         * @param text Text of the log.
         * @details Message log represents simple output of a program.
         */
        void RecordMessage(const std::string& text);
        /**
         * @brief Record a milestone log.
         * @param text Text of the log.
         * @details Milestone log represents important time points of a program.
         */
        void RecordMilestone(const std::string& text);
        /**
         * @brief Record a warning log.
         * @param text Text of the log.
         * @details Warning log represents important messages that should pay attention to.
         */
        void RecordWarning(const std::string& text);
        /**
         * @brief Record an error log.
         * @param text Text of the log.
         * @details Error log represents the abnormal situation of a program.
         */
        void RecordError(const std::string& text);
    };
}