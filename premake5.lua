-- premake5.lua

local projects = "" .. _MAIN_SCRIPT_DIR .. "/.build/" .. _ACTION
local ext = _MAIN_SCRIPT_DIR .. "/ext"

workspace "Phoenix"
    platforms { "x64" }
    configurations { "Debug", "Release" }
    startproject "TestApp"
    warnings "default"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    debugdir (_MAIN_SCRIPT_DIR)
    location (projects)

    group "External"
        project "lua"
        project "sol2"

    group "Phoenix"
        project "PhoenixCore"
        project "PhoenixSim"
        project "PhoenixLua"

    group "Tests"
        project "TestApp"

    group ""

project "lua"
    kind "StaticLib"
    location (projects)

    files { 
        ext .. "/lua/lua-5.4.8/src/**"
    }

project "sol2"
    kind "StaticLib"
    location (projects)

    dependson ( "lua" )

    files { 
        ext .. "/sol/**"
    }

project "PhoenixCore"
    kind "SharedLib"
    location (projects)

    files { "src/PhoenixCore/**", }
    includedirs { "src/PhoenixCore/Public/" }

    filter "system:windows"
        defines { "PHOENIXCORE_DLL_EXPORTS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"
        
    filter {}

project "PhoenixSim"
    kind "SharedLib"
    location (projects)

    dependson { "PhoenixCore" }

    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixSim/**"
    }

    includedirs {
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
    }

    links {
        "PhoenixCore"
    }

    filter "system:windows"
        defines { "PHOENIXSIM_DLL_EXPORTS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"
        
    filter {}

project "PhoenixLua"
    kind "SharedLib"
    location (projects)

    dependson { "lua", "sol2", "PhoenixCore", "PhoenixSim" }

    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixLua/**"
    }

    includedirs {
        "src/PhoenixLua/Public",
        "src/PhoenixLua/Private",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
    }

    externalincludedirs {
        ext,
        ext .. "/lua/lua-5.4.8/src/",
        ext .. "/sol/"
    }

    links {
        "lua",
        "PhoenixCore",
        "PhoenixSim"
    }

    filter "system:windows"
        defines { "PHOENIXLUA_DLL_EXPORTS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"
        
    filter {}

project "TestApp"
    kind "ConsoleApp"
    location (projects)

    dependson { "PhoenixCore", "PhoenixSim", "PhoenixLua" }

    defines { "TRACY_ENABLE", "PHX_PROFILE_ENABLE" }

    files {
        "tests/TestApp/**.h",
        "tests/TestApp/**.inl",
        "tests/TestApp/**.cpp",

        ext .. "/imgui/*",
        ext .. "/imgui/backends/imgui_impl_sdl3.h",
        ext .. "/imgui/backends/imgui_impl_sdl3.cpp",
        ext .. "/imgui/backends/imgui_impl_sdlrenderer3.h",
        ext .. "/imgui/backends/imgui_impl_sdlrenderer3.cpp",

        ext .. "/tracy/TracyClient.cpp",
    }

    includedirs {
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
        "src/PhoenixLua/Public",
        ext .. "/imgui/**",
        ext .. "/tracy/**"
    }

    externalincludedirs {
        ext,
        ext .. "/imgui/",
        ext .. "/imgui/**",
        ext .. "/nlohmann/*",
        ext .. "/lua/lua-5.4.8/src/"
    }

    libdirs {
        ext .. "/SDL3/x64/Debug"
    }

    links {
        "lua",
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixLua",
        "SDL3"
    }

    postbuildcommands {
        "xcopy /s /y \"" .. ext .. "\\SDL3\\x64\\%{cfg.buildcfg}\\*.*\" \"$(TargetDir)\""
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"
        
    filter {}