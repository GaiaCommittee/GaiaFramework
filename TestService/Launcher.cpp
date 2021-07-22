#include <GaiaFramework/GaiaFramework.hpp>
#include <iostream>

using namespace Gaia::Framework;

class TestService : public Gaia::Framework::Service
{
public:
    CONSTRUCTOR(TestService){}

protected:
    void OnInstall() override
    {
        AddCommand("hello", [](const std::string &content) {
            std::cout << "World!" << std::endl;
        });
        AddCommand("greet", [](const std::string &content) {
            std::cout << "Greet " << content << "!" << std::endl;
        });

        AddSubscription("sample_channel", [](const std::string& content){
           std::cout << "Message: " << content << std::endl;
        });

        GetLogger()->RecordMessage("Installed");
    }

    void OnUninstall() override
    {
        GetLogger()->RecordMessage("Uninstalled");
    }

    void OnUpdate() override
    {

    }
};

int main(int argc, char **argv)
{
    Launch<TestService>(argc, argv);
}