#pragma once

#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include "Service.hpp"

namespace Gaia::Framework
{
    /**
     * @brief Launch the service server with the given type of service.
     * @tparam ServiceClass Type of service.
     * @tparam ArgumentTypes Type of service constructor arguments.
     * @param constructor_arguments Arguments to pass to camera driver constructor.
     * @details
     *  This function will block until the server receive a shutdown command and exit normally.
     */
    template <typename ServiceClass, typename... ArgumentTypes>
    void Launch(int command_line_counts, char** command_line, ArgumentTypes... constructor_arguments)
    {
        using namespace boost::program_options;

        bool crashed;
        do
        {
            try
            {
                crashed = false;

                ServiceClass service_object;
                auto* service = dynamic_cast<Service*>(&service_object);

                std::string option_host = "127.0.0.1";
                unsigned int option_port = 6379;

                if (command_line_counts > 0 && command_line && *command_line)
                {
                    store(parse_command_line(command_line_counts, command_line,
                                             service->OptionDescription),
                          service->OptionVariables);
                    notify(service->OptionVariables);

                    if (service->OptionVariables.count("help"))
                    {
                        std::cout << service->OptionDescription << std::endl;
                        return;
                    }

                    option_host = service->OptionVariables["host"].as<std::string>();
                    option_port = service->OptionVariables["port"].as<unsigned int>();
                }

                std::cout << "Service " << service->Name << " starting..." << std::endl;
                service->Connect(option_port, option_host);
                std::cout << "Service " << service->Name << " connected to data center at "
                    << option_host << ":" << option_port << std::endl;
                service->Install();
                std::cout << "Service " << service->Name << " initialized." << std::endl;
                while (service->Update());
                service->Uninstall();
                std::cout << "Service " << service->Name << " stopped." << std::endl;
            }catch (std::exception& error)
            {
                crashed = true;
                std::cout << "Service crashed, exception:" << std::endl;
                std::cout << error.what() << std::endl;
                std::cout << "Service will restart in 1 second." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (crashed);
    }
}