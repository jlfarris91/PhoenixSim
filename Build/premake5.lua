-- premake5.lua

workspace "Phoenix"
   architecture "x64"
   configurations { "Debug", "Release" }
   startproject "PhoenixSimDriver"
   location "../"

   group "Core"
      project "PhoenixCore"
      project "PhoenixSim"

   group "Tests"
      project "PhoenixSimDriver"
      project "CDT"

project "PhoenixCore"
   kind "StaticLib"
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
      defines { "DLL_EXPORTS" }

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
      defines { "DLL_EXPORTS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"

project "PhoenixSimDriver"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../Tests/PhoenixSimDriver"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../Tests/PhoenixSimDriver/**.h",
      "../Tests/PhoenixSimDriver/**.inl",
      "../Tests/PhoenixSimDriver/**.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**",
      "../PhoenixSim/Public/",
      "../PhoenixSim/Public/**",
   }

   externalincludedirs {
      "../External/",
      "../External/imgui/",
      "../External/imgui/**"
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

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Debug\\*.*\" \"$(TargetDir)\""
      }

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      
      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Release\\*.*\" \"$(TargetDir)\""
      }

project "CDT"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../Tests/CDT"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../Tests/CDT/**.h",
      "../Tests/CDT/**.inl",
      "../Tests/CDT/**.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**"
   }

   externalincludedirs {
      "../External/"
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

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Debug\\*.*\" \"$(TargetDir)\""
      }

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      
      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Release\\*.*\" \"$(TargetDir)\""
      }

project "TestApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../Tests/TestApp"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../Tests/TestApp/**.h",
      "../Tests/TestApp/**.inl",
      "../Tests/TestApp/**.cpp",

      "../External/imgui/*",
      "../External/imgui/backends/imgui_impl_sdl3.h",
      "../External/imgui/backends/imgui_impl_sdl3.cpp",
      "../External/imgui/backends/imgui_impl_sdlrenderer3.h",
      "../External/imgui/backends/imgui_impl_sdlrenderer3.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**",
      "../PhoenixSim/Public/",
      "../PhoenixSim/Public/**",
      "../External/imgui/**"
   }

   externalincludedirs {
      "../External/",
      "../External/imgui/",
      "../External/imgui/**"
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

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "On"

      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Debug\\*.*\" \"$(TargetDir)\""
      }

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      
      postbuildcommands {
        "xcopy /s /y \"$(SolutionDir)\\External\\SDL3\\x64\\Release\\*.*\" \"$(TargetDir)\""
      }