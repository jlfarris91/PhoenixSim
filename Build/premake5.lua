-- premake5.lua

workspace "Phoenix"
   architecture "x64"
   configurations { "Debug", "Release" }
   startproject "PhoenixSimDriver"
   location "../"

project "PhoenixFixedPoint"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   location "../PhoenixFixedPoint"
   
   targetdir ("./Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("./Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

   files {
      "../PhoenixFixedPoint/Public/**.h",
      "../PhoenixFixedPoint/Private/**.h",
      "../PhoenixFixedPoint/Private/**.cpp"
   }

   includedirs {
      "../PhoenixFixedPoint/Public"
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
      "../PhoenixSim/Private/**.h",
      "../PhoenixSim/Private/**.cpp"
   }

   includedirs {
      "../PhoenixSim/Public"
   }

   externalincludedirs {
      "../PhoenixFixedPoint/Public/"
   }

   links {
      "PhoenixFixedPoint"
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
      "../PhoenixSimDriver/**.cpp"
   }

   removefiles {
      "../PhoenixSimDriver/External/**"
   }

   includedirs {
      "../PhoenixSimDriver"
   }

   externalincludedirs {
      "../PhoenixFixedPoint/Public/",
      "../PhoenixSim/Public/",
      "../PhoenixSimDriver/External/"
   }

   libdirs {
      "../PhoenixSimDriver/External/SDL3/x64/Debug"
   }

   links {
      "PhoenixFixedPoint",
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