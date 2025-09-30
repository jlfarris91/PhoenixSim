-- premake5.lua

workspace "Phoenix"
   architecture "x64"
   configurations { "Debug", "Release" }
   startproject "PhoenixSimDriver"
   location "../"

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
      "../PhoenixCore/Public"
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
      "../PhoenixSim/Public"
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

   removefiles {
      "../PhoenixSimDriver/External/**"
   }

   includedirs {
      "../PhoenixCore/Public/",
      "../PhoenixSim/Public/",
   }

   externalincludedirs {
      "../PhoenixSimDriver/External/"
   }

   libdirs {
      "../PhoenixSimDriver/External/SDL3/x64/Debug"
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
        "xcopy /s /y \"$(ProjectDir)\\External\\SDL3\\x64\\Debug\\*.*\" \"$(TargetDir)\""
      }

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      
      postbuildcommands {
        "xcopy /s /y \"$(ProjectDir)\\External\\SDL3\\x64\\Release\\*.*\" \"$(TargetDir)\""
      }