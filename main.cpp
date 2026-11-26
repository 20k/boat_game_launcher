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
#include <networking/networking_compat.hpp>
#include <stdlib.h>
#include <stdio.h>

std::string read(const std::string& file)
{
    FILE* f = fopen(file.c_str(), "r");

    if(f == nullptr)
        return "";

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(fsize == -1L)
    {
        fclose(f);
        return "";
    }

    std::string buffer;
    buffer.resize(fsize);
    size_t real = fread(buffer.data(), 1, fsize, f);
    buffer.resize(real);

    fclose(f);

    return buffer;
}


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

void task_launch()
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

        for(int i=0; i < (int)who.size(); i++)
        {
            bool all_match = true;

            for(int j=0; j < (int)pattern.size(); j++)
            {
                int cidx = i + j;

                if(cidx >= (int)who.size())
                {
                    all_match = false;
                    break;
                }

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
                if(i + (int)pattern.size() >= (int)who.size())
                    return true;
                else
                    return !std::isdigit(who.at(i + pattern.size()));
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
        std::string test = "1650";
        std::string test2 = "NVIDIA GeForce GTX 1650";

        assert(contains_pattern(test, "16??"));
        assert(contains_pattern(test2, "16??"));
        assert(!contains_pattern(test, "16?"));
    }

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
                return;
            }
            else if(any_pattern(sval, c.patterns))
            {
                launch_with(c.option);
                return;
            }
        }
    }

    launch_with("");
}

void task_crashdump(bool local)
{
    network_client* conn = start_network_client();

    if(!local)
        conn->connect(85568392937018816ULL, 1284, "lettersofmarque.xyz", false);
    else
        conn->connect(0, 1285, "127.0.0.1", true);

    while(conn->connection_in_progress()){}

    if(!conn->client_connected_to_server())
        return;

    nlohmann::json js;
    js["command"] = "crash_dump";
    js["data_crash"] = read("crash.txt");
    js["data_crash_exception"] = read("crash_exception.txt");

    connection_send_data snd;

    write_data dat;
    dat.id = -1;
    dat.data = js.dump();

    snd.write_to_websocket(std::move(dat));
    conn->send_bulk(std::move(snd));

    conn->terminate();
}

int main(int argc, char* argv[])
{
    std::ofstream* crash = new std::ofstream("launcher_log.txt");
    std::cout.rdbuf(crash->rdbuf());

    bool crashdump = false;
    bool local = false;

    for(int i=0; i < argc; i++)
    {
        char* str = argv[i];

        if(strcmp(str, "-crashdump") == 0)
        {
            crashdump = true;
        }

        if(strcmp(str, "-local") == 0)
        {
            local = true;
        }
    }

    #ifdef LOCAL
    //#define CRASHDUMP
    local = true;
    #endif
    #ifdef CRASHDUMP
    crashdump = true;
    #endif // CRASHDUMP

    if(!crashdump)
        task_launch();
    else
        task_crashdump(local);

    return 0;
}
