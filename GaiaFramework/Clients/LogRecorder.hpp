#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <atomic>

namespace Gaia::Framework::Clients
{
    /**
     * @brief A log recorder will record logs into a text file.
     * @details
     *  Its Record(...) function is multi-threads safe to use.
     */
    class LogRecorder
    {
    public:
        /// Severity to mark how important a piece of log is.
        enum class Severity
        {
            Message = 0,    ///< Message stands for basic output information.
            Milestone = 1,  ///< Milestone stands for the important time point of the application life circle.
            Warning = 2,    ///< Warning stands for abnormal situations which are not deadly.
            Error = 3       ///< Error stands for critical deadly abnormal situations.
        };

    protected:
        /// Stream for log file.
        std::fstream LogFile;
        /// Path for log file.
        std::string LogFilePath;

        /// The time point of last auto-save.
        std::chrono::system_clock::time_point LastAutoSaveTime;
        /// The interval time of auto-save.
        std::chrono::system_clock::duration AutoSaveDuration {std::chrono::seconds(1)};

        /// Mutex for log file and timestamp of auto-save.
        std::mutex OperationMutex;

    public:
        /// Generate a log text in the log format.
        static std::string GenerateLogText(const std::string& text, Severity severity, const std::string& author = "Anonymous");

    public:
        /**
         * @brief Constructor
         */
        explicit LogRecorder(const std::string& unit_name = "") noexcept;

        /// Destructor which will automatically save the logs.
        ~LogRecorder();

        /// Whether the log will be printed to the console or not.
        bool PrintToConsole {false};

        /// Record a raw text into the log.
        void RecordRawText(const std::string& text);

        /**
         * @brief Record a log.
         * @param author The name of the object which produce this log.
         * @param text The log text.
         * @param level The level of the log.
         */
        void Record(const std::string& text, Severity level = Severity::Message, const std::string& author = "Anonymous");

        /// Save the current recorded log into the log file.
        void Flush();

        /// Record a message. Equals Record(..., Severity::Message).
        template<typename AuthorType, typename TextType>
        inline void RecordMessage(TextType&& text, AuthorType&& author)
        {
            Record(std::forward<TextType>(text), Severity::Message, std::forward<AuthorType>(author));
        }
        /// Record a message. Equals Record(..., Severity::Message).
        template<typename TextType>
        inline void RecordMessage(TextType&& text)
        {
            Record(std::forward<TextType>(text), Severity::Message);
        }

        /// Record a milestone. Equals Record(..., Severity::Milestone).
        template<typename AuthorType, typename TextType>
        inline void RecordMilestone(TextType&& text, AuthorType&& author)
        {
            Record(std::forward<TextType>(text), Severity::Milestone, std::forward<AuthorType>(author));
        }
        /// Record a milestone. Equals Record(..., Severity::Milestone).
        template<typename TextType>
        inline void RecordMilestone(TextType&& text)
        {
            Record(std::forward<TextType>(text), Severity::Milestone);
        }

        /// Record a warning. Equals Record(..., Severity::Warning).
        template<typename AuthorType, typename TextType>
        inline void RecordWarning(TextType&& text, AuthorType&& author)
        {
            Record(std::forward<TextType>(text), Severity::Warning, std::forward<AuthorType>(author));
        }
        /// Record a warning. Equals Record(..., Severity::Warning).
        template<typename TextType>
        inline void RecordWarning(TextType&& text)
        {
            Record(std::forward<TextType>(text), Severity::Warning);
        }

        /// Record an error. Equals Record(..., Severity::Error).
        template<typename AuthorType, typename TextType>
        inline void RecordError(TextType&& text, AuthorType&& author)
        {
            Record(std::forward<TextType>(text), Severity::Error, std::forward<AuthorType>(author));
        }
        /// Record an error. Equals Record(..., Severity::Error).
        template<typename TextType>
        inline void RecordError(TextType&& text)
        {
            Record(std::forward<TextType>(text), Severity::Error);
        }
    };
}