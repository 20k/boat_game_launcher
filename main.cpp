#include <iostream>

#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <iostream>
#include <assert.h>

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

    auto launch_with = [](const std::string& params)
    {

    };

    auto contains_pattern = [](const std::string& who, const std::string& pattern)
    {
        if(pattern.size() > who.size())
            return false;

        int fin = (int)who.size() - (int)pattern.size();

        for(int i=0; i < fin; i++)
        {
            bool all_match = true;

            for(int j=0; j < pattern.size(); j++)
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

    if(vendor == "intel")
    {
        launch_with("--rendering-driver vulkan --myarg=nosdfgi");
    }
    else if(vendor == "nvidia")
    {
        //whole 5xxx series confirmed affected. Unsure about 4xxx, probably will affect 6xxx
        std::vector<std::string> patt = {"5???", "6???"};

        if(any_pattern(sval, patt))
            launch_with("--rendering-driver vulkan");
        else
            launch_with("");

    }
    else if(vendor == "amd")
    {
        //580 confirmed affected
        std::vector<std::string> patt = {"5??"};

         if(any_pattern(sval, patt))
            launch_with("--rendering-driver vulkan");
         else
            launch_with("");
    }

    return 0;
}
