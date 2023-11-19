-- premake5.lua
workspace "SimpleRaytracer"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "SimpleRaytracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "SimpleRaytracer"