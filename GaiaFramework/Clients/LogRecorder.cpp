#include "LogRecorder.hpp"

#include <ctime>
#include <chrono>
#include <sstream>

#include <iostream>

namespace Gaia::Framework::Clients
{
    /// Construct and create the log file.
    LogRecorder::LogRecorder(const std::string& unit_name) noexcept
    {
        try
        {
            auto current_time_point = std::chrono::system_clock::now();
            LastAutoSaveTime = current_time_point;
            auto global_time = std::chrono::system_clock::to_time_t(current_time_point);
            auto local_time = std::localtime(&global_time);

            std::stringstream file_name_builder;
            if (!unit_name.empty()) file_name_builder << unit_name << " ";
            file_name_builder << local_time->tm_mon + 1 << "-" << local_time->tm_mday << " ";
            file_name_builder << local_time->tm_hour << ":" << local_time->tm_min << ":" << local_time->tm_sec;
            file_name_builder << ".log";

            LogFilePath = file_name_builder.str();
        }
        catch (std::exception& error)
        {
            std::cout << "Failed to initialize log recorder: " << error.what() << std::endl;
        }
    }

    /// Save the log and destruct.
    LogRecorder::~LogRecorder()
    {
        if (LogFile.is_open())
        {
            LogFile.close();
        }
    }

    /// Redirect to the non-author Record function with a connected text of author and log content.
    void LogRecorder::Record(const std::string &text, LogRecorder::Severity level, const std::string& author)
    {
        RecordRawText(GenerateLogText(text, level, author));
    }

    /// Save the current recorded log into the log file.
    void LogRecorder::Flush()
    {
        if (LogFile.is_open())
        {
            LogFile.flush();
        }
    }

    /// Generate a log text in the log format.
    std::string LogRecorder::GenerateLogText(const std::string& text, LogRecorder::Severity severity,
                                             const std::string& author)
    {
        auto current_time_point = std::chrono::system_clock::now();
        auto global_time = std::chrono::system_clock::to_time_t(current_time_point);
        auto local_time = std::localtime(&global_time);

        std::stringstream builder;
        builder << local_time->tm_hour << ":" << local_time->tm_min << ":" << local_time->tm_sec;
        builder << "|";
        switch (severity)
        {
            case Severity::Message:
                builder << "Message";
                break;
            case Severity::Milestone:
                builder << "Milestone";
                break;
            case Severity::Warning:
                builder << "Warning";
                break;
            case Severity::Error:
                builder << "Error";
                break;
            default:
                break;
        }
        builder << "|" << author << "|" << text;

        return builder.str();
    }

    /// Record a raw text into the log.
    void LogRecorder::RecordRawText(const std::string& text)
    {
        if (!LogFile.is_open() && !LogFilePath.empty())
        {
            LogFile.open(LogFilePath, std::ios::out);
        }

        std::unique_lock operation_lock(OperationMutex);

        if (LogFile.is_open())
        {
            LogFile << text << std::endl;
            auto current_time_point = std::chrono::system_clock::now();
            if (current_time_point - LastAutoSaveTime > AutoSaveDuration)
            {
                LogFile.flush();
                LastAutoSaveTime = current_time_point;
            }
        }

        if (PrintToConsole)
        {
            std::cout << text << std::endl;
        }

        operation_lock.unlock();
    }
}
