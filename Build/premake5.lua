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
   staticruntime "on"
   location "../PhoenixSimDriver"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../PhoenixSimDriver/**.h",
      "../PhoenixSimDriver/**.inl",
      "../PhoenixSimDriver/**.cpp"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixCore/Public/**",
      "../PhoenixSim/Public/",
      "../PhoenixSim/Public/**",
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

project "CDT"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "on"
   location "../Tests/CDT"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../CDT/**.h",
      "../CDT/**.inl",
      "../CDT/**.cpp"
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