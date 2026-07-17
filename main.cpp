#include <iostream>

#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <iostream>
#include <assert.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <processthreadsapi.h>

struct cfg
{
    std::string vendor;
    std::vector<std::string> patterns;
    std::string option;

    void read(nlohmann::json& js)
    {
        vendor = js["vendor"];
        patterns = js["patterns"];
        option = js["option"];
    }
};

std::string read_file(const std::string& name)
{
    std::ifstream ifs(name);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

int main()
{
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Create gpu device
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

    assert(device);

    SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);

    const char* val = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "");

    assert(val);

    std::string sval = val;

    std::cout << sval << std::endl;

    for(auto& i : sval)
        i = std::tolower(i);

    //--rendering-driver vulkan
    std::string full_arg = "";

    std::string vendor = "amd";

    if(sval.contains("nvidia"))
        vendor = "nvidia";

    if(sval.contains("intel"))
        vendor = "intel";

    auto launch_with = [](std::string params)
    {
        params = "--disable-crash-handler " + params;

        std::cout << "Launching " << params << std::endl;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        assert(CreateProcessA("./LettersOfMarque.exe", params.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi));
    };

    auto contains_pattern = [](const std::string& who, const std::string& pattern)
    {
        if(pattern.size() > who.size())
            return false;

        int fin = (int)who.size() - (int)pattern.size();

        for(int i=0; i < fin; i++)
        {
            bool all_match = true;

            for(int j=0; j < (int)pattern.size(); j++)
            {
                int cidx = i + j;

                if(pattern[j] == '?')
                {
                    if(!std::isdigit(who[cidx]))
                    {
                        all_match = false;
                        break;
                    }

                    continue;
                }

                if(who[cidx] != pattern[j])
                {
                    all_match = false;
                    break;
                }
            }

            if(all_match)
            {
                if(i == fin - 1)
                    return true;
                else
                    return !std::isdigit(who[i + pattern.size()]);
            }
        }

        return false;
    };

    auto any_pattern = [&](const std::string& who, const std::vector<std::string>& patterns)
    {
        for(auto& i : patterns)
        {
            if(contains_pattern(who, i))
                return true;
        }

        return false;
    };

    {
        std::string test = "AMD Radeon RX 6700 XT";
        std::string p1 = "6??";
        std::string p2 = "6???";
        std::string p3 = "5??";
        std::string p4 = "5???";

        assert(!contains_pattern(test, p1));
        assert(contains_pattern(test, p2));
        assert(!contains_pattern(test, p3));
        assert(!contains_pattern(test, p4));
    }

    std::vector<nlohmann::json> data = nlohmann::json::parse(read_file("./launch_cfg.json"));

    std::vector<cfg> cfgs;

    for(auto& i : data)
    {
        cfg& c = cfgs.emplace_back();
        c.read(i);
    }

    for(const cfg& c : cfgs)
    {
        if(vendor == c.vendor)
        {
            if(c.patterns.size() == 0)
            {
                launch_with(c.option);
                return 0;
            }
            else if(any_pattern(sval, c.patterns))
            {
                launch_with(c.option);
                return 0;
            }
        }
    }

    launch_with("");

    return 0;
}
