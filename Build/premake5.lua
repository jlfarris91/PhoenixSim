-- premake5.lua

workspace "Phoenix"
   architecture "x64"
   configurations { "Debug", "Release" }
   startproject "TestApp"
   location "../"

   group "Core"
      project "PhoenixCore"
      project "PhoenixSim"

   group "Tests"
      project "TestApp"

project "PhoenixCore"
   kind "SharedLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../PhoenixCore"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../PhoenixCore/Public/**.h",
      "../PhoenixCore/Public/**.inl",
      "../PhoenixCore/Private/**.h",
      "../PhoenixCore/Private/**.inl",
      "../PhoenixCore/Private/**.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/"
   }

   filter "system:windows"
      systemversion "latest"
      defines { "PHOENIXCORE_DLL_EXPORTS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"

project "PhoenixSim"
   kind "SharedLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../PhoenixSim"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   defines { "PHX_PROFILE_ENABLE" }

   files {
      "../PhoenixSim/Public/**.h",
      "../PhoenixSim/Public/**.inl",
      "../PhoenixSim/Private/**.h",
      "../PhoenixSim/Private/**.inl",
      "../PhoenixSim/Private/**.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**",
      "../PhoenixSim/Public/",
      "../PhoenixSim/Public/**"
   }

   links {
      "PhoenixCore"
   }

   filter "system:windows"
      systemversion "latest"
      defines { "PHOENIXSIM_DLL_EXPORTS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"

project "TestApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../Tests/TestApp"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   defines { "TRACY_ENABLE", "PHX_PROFILE_ENABLE" }

   files {
      "../Tests/TestApp/**.h",
      "../Tests/TestApp/**.inl",
      "../Tests/TestApp/**.cpp",

      "../External/nlohmann/*",

      "../External/imgui/*",
      "../External/imgui/backends/imgui_impl_sdl3.h",
      "../External/imgui/backends/imgui_impl_sdl3.cpp",
      "../External/imgui/backends/imgui_impl_sdlrenderer3.h",
      "../External/imgui/backends/imgui_impl_sdlrenderer3.cpp",
      
      "../External/tracy/TracyClient.cpp",
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**",
      "../PhoenixSim/Public/",
      "../PhoenixSim/Public/**",
      "../External/imgui/**",
      "../External/tracy/**"
   }

   externalincludedirs {
      "../External/",
      "../External/imgui/",
      "../External/imgui/**",
      "../External/lua/lua-5.4.8/src/"
   }

   libdirs {
      "../External/SDL3/x64/Debug"
   }

   links {
      "PhoenixCore",
      "PhoenixSim",
      "SDL3"
   }

   filter "system:windows"
      systemversion "latest"

   postbuildcommands {
      "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\%{cfg.buildcfg}\\*.*\" \"$(TargetDir)\"",
      "xcopy /s /y \"$(SolutionDir)\\External\\sol\\lib\\x64\\%{cfg.buildcfg}\\*.*\" \"$(TargetDir)\""
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"