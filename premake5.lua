-- premake5.lua

local projects = "" .. _MAIN_SCRIPT_DIR .. "/.build/" .. _ACTION
local ext = _MAIN_SCRIPT_DIR .. "/ext"

workspace "Phoenix"
    platforms { "x64" }
    configurations { "Debug", "Release", "ReleaseWithSymbols" }
    startproject "TestApp"
    warnings "default"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    debugdir (_MAIN_SCRIPT_DIR)
    location (_MAIN_SCRIPT_DIR)

    group "External"
        project "lua"
        project "sol2"

    group "Phoenix"
        project "PhoenixCore"
        project "PhoenixSim"
        project "PhoenixLDS"
        project "PhoenixBlackboard"
        project "PhoenixECS"
        project "PhoenixPhysics"
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

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "sol2"
    kind "StaticLib"
    location (projects)

    dependson ( "lua" )

    files { 
        ext .. "/sol/**"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixCore"
    kind "StaticLib"
    location (projects)

    files { "src/PhoenixCore/**", }
    includedirs { "src/PhoenixCore/Public/" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIXCORE_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixSim"
    kind "StaticLib"
    location (projects)

    dependson { "PhoenixCore" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIXSIM_DLL_EXPORTS" }
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

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "Off"
        optimize "speed"

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixLDS"
    kind "StaticLib"
    location (projects)

    dependson { "PhoenixCore", "PhoenixSim" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIX_LDS_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixLDS/**"
    }

    includedirs {
        "src/PhoenixLDS/Public",
        "src/PhoenixLDS/Private",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
    }

    externalincludedirs {
        ext,
        ext .. "/nlohmann/*",
    }

    links {
        "PhoenixCore",
        "PhoenixSim"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixBlackboard"
    kind "StaticLib"
    location (projects)

    dependson { "PhoenixCore", "PhoenixSim" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIX_BLACKBOARD_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixBlackboard/**"
    }

    includedirs {
        "src/PhoenixBlackboard/Public",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
    }

    links {
        "PhoenixCore",
        "PhoenixSim"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixECS"
    kind "StaticLib"
    location (projects)

    dependson { "PhoenixCore", "PhoenixSim", "PhoenixBlackboard" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIXECS_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixECS/**"
    }

    includedirs {
        "src/PhoenixECS/Public",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
        "src/PhoenixBlackboard/Public"
    }

    links {
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixBlackboard"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixPhysics"
    kind "StaticLib"
    location (projects)

    dependson { "PhoenixCore", "PhoenixSim", "PhoenixECS" }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIX_PHYSICS_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixPhysics/**"
    }

    includedirs {
        "src/PhoenixPhysics/Public",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
        "src/PhoenixBlackboard/Public",
        "src/PhoenixECS/Public"
    }

    links {
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixECS"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "PhoenixLua"
    kind "StaticLib"
    location (projects)

    dependson {
        "lua",
        "sol2",
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixBlackboard",
        "PhoenixECS",
        "PhoenixPhysics"
    }

    -- defines { "PHOENIX_DLL" }
    -- defines { "PHOENIXSIM_DLL_EXPORTS" }
    defines { "PHX_PROFILE_ENABLE" }

    files { 
        "src/PhoenixLua/**"
    }

    includedirs {
        "src/PhoenixLua/Public",
        "src/PhoenixLua/Private",
        "src/PhoenixCore/Public",
        "src/PhoenixSim/Public",
        "src/PhoenixBlackboard/Public",
        "src/PhoenixECS/Public",
        "src/PhoenixPhysics/Public",
    }

    externalincludedirs {
        ext,
        ext .. "/lua/lua-5.4.8/src/",
        ext .. "/sol/"
    }

    links {
        "lua",
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixBlackboard",
        "PhoenixECS",
        "PhoenixPhysics",
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }

project "TestApp"
    kind "ConsoleApp"
    location (projects)

    dependson {
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixLDS",
        "PhoenixECS",
        "PhoenixBlackboard",
        "PhoenixPhysics",
        "PhoenixLua"
    }

    -- defines { "PHOENIX_DLL" }
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
        "src/PhoenixLDS/Public",
        "src/PhoenixBlackboard/Public",
        "src/PhoenixECS/Public",
        "src/PhoenixPhysics/Public",
        "src/PhoenixLua/Public"
    }

    externalincludedirs {
        ext,
        ext .. "/imgui/",
        ext .. "/imgui/**",
        ext .. "/nlohmann/*",
        ext .. "/lua/lua-5.4.8/src/",
        ext .. "/tracy/"
    }

    libdirs {
        ext .. "/SDL3/x64/Debug"
    }

    links {
        "lua",
        "PhoenixCore",
        "PhoenixSim",
        "PhoenixLDS",
        "PhoenixBlackboard",
        "PhoenixECS",
        "PhoenixPhysics",
        "PhoenixLua",
        "SDL3"
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

    filter "configurations:ReleaseWithSymbols"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "speed"
        
    filter {}

    -- TODO (jfarris): fix
    disablewarnings {
        "4251", "4275"
    }
    
    filter "not configurations:ReleaseWithSymbols"
        postbuildcommands {
            "xcopy /s /y \"" .. ext .. "\\SDL3\\x64\\%{cfg.buildcfg}\\*.*\" \"$(TargetDir)\""
        }

    filter "configurations:ReleaseWithSymbols"
        postbuildcommands {
            "xcopy /s /y \"" .. ext .. "\\SDL3\\x64\\Release\\*.*\" \"$(TargetDir)\""
        }

    filter {}