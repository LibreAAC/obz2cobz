-- Small list of dependencies by name:
-- TARGET={LINUX,WIN,ANDROID}
--  zip by kuba-- on github under MIT license
--  cJSON by DaveGamble on github under MIT license
--  plutosvg by sammycage under MIT license
--  stb_image by nothings under public domain
--  stb_image_write by nothings under public domain
require("src/shared/shared")
load_os()
CJSON_VERSION = "1.7.18"
PLUTOSVG_VERSION = "0.0.6"
ZIP_VERSION = "0.3.3"
TEMP_FOLDERS = { "plutosvg" }
parse_args()

CMAKE_FLAGS = ""
if TARGET == "WIN" then
  CMAKE_FLAGS = ("-G 'MinGW Makefiles' -DCMAKE_C_COMPILER='"..
                popen("where gcc")..
                "' -DCMAKE_CXX_COMPILER='"..
                popen("where g++")..
                "' -DCMAKE_MAKE_PROGRAM='"..
                popen("where make")..
                "'"
                )
end

function inst_zip()
  print("Download, compile and move zip...")
  print("\n"..TARGET.."\n\n")
  if TARGET == "LINUX" then
    wget("https://github.com/kuba--/zip/archive/refs/tags/v"..ZIP_VERSION..".tar.gz",
        "v"..ZIP_VERSION..".tar.gz")
    extract("v"..ZIP_VERSION..".tar.gz")
    shell("mkdir -p zip-"..ZIP_VERSION.."/build")
    mv("zip-"..ZIP_VERSION.."/src/*", "include/")
    mv("zip-"..ZIP_VERSION.."/LICENSE.txt", "licenses/zip.txt")
    rm("v"..ZIP_VERSION..".tar.gz*")
    rm("zip-"..ZIP_VERSION)
  elseif TARGET == "WIN" then
    wget("https://github.com/kuba--/zip/archive/refs/tags/v"..ZIP_VERSION..".zip",
        "v"..ZIP_VERSION..".zip")
    extract("v"..ZIP_VERSION..".zip")
    shell("mkdir -p zip-"..ZIP_VERSION.."/build")
    mv("zip-"..ZIP_VERSION.."/src/*", "include/")
    mv("zip-"..ZIP_VERSION.."/LICENSE.txt", "licenses/zip.txt")
    rm("v"..ZIP_VERSION..".zip*")
    rm("zip-"..ZIP_VERSION)
  else
    todo()
  end
end
function inst_cjson()
  print("Download and move cJSON...")
  if TARGET == "LINUX" then
    wget("https://github.com/DaveGamble/cJSON/archive/refs/tags/v"..CJSON_VERSION..".tar.gz",
        "v"..CJSON_VERSION..".tar.gz")
    extract("v"..CJSON_VERSION..".tar.gz")
    mv("cJSON-"..CJSON_VERSION.."/cJSON.*", "include/")
    mv("cJSON-"..CJSON_VERSION.."/LICENSE", "licenses/cJSON.txt")
    rm("v"..CJSON_VERSION..".tar.gz*")
    rm("cJSON-"..CJSON_VERSION)
  elseif TARGET == "WIN" then
    wget("https://github.com/DaveGamble/cJSON/archive/refs/tags/v"..CJSON_VERSION..".zip",
        "v"..CJSON_VERSION..".zip")
    extract("v"..CJSON_VERSION..".zip")
    mv("cJSON-"..CJSON_VERSION.."/cJSON.*", "include/")
    mv("cJSON-"..CJSON_VERSION.."/LICENSE", "licenses/cJSON.txt")
    rm("v"..CJSON_VERSION..".zip*")
    rm("cJSON-"..CJSON_VERSION)
  else
    todo()
  end
end
function inst_plutosvg()
  print("Download, compile and move plutosvg...")
  if exists("plutosvg") then
    rm("plutosvg")
  end
  shell("git clone --recursive https://github.com/sammycage/plutosvg.git")
  if TARGET == "LINUX" then
    shell("cd plutosvg && cmake -B build . && cmake --build build")
    mv("plutosvg/source/*", "include/")
    mv("plutosvg/plutovg/include/plutovg.h", "include/")
    mv("plutosvg/build/*.a", "lib/")
    mv("plutosvg/build/plutovg/*.a", "lib/")
    mv("plutosvg/LICENSE", "licenses/plutosvg.txt")
    rm("plutosvg")
  elseif TARGET == "WIN" then
    shell("; cd plutosvg ; cmake -B build . ; cmake --build build")
    mv("plutosvg/source/*", "include/")
    mv("plutosvg/plutovg/include/plutovg.h", "include/")
    mv("plutosvg/build/*.a", "lib/")
    mv("plutosvg/build/plutovg/*.a", "lib/")
    mv("plutosvg/LICENSE", "licenses/plutosvg.txt")
    rm("plutosvg")
  elseif TARGET == "WIN" then
    if exists("plutosvg") then
      rm("plutosvg")
    end
    shell("git clone --recursive https://github.com/sammycage/plutosvg.git")
    shell("cd plutosvg && cmake -B build . "..CMAKE_FLAGS.." && cmake --build build "..CMAKE_FLAGS)
    mv("plutosvg/source/*", "include/")
    mv("plutosvg/plutovg/include/plutovg.h", "include/")
    -- mv("plutosvg/build/plutovg/*.a", "include") -- reactivate this line if plutosvg.a is not enough
    mv("plutosvg/build/*.a", "lib/")
    mv("plutosvg/build/plutovg/*.a", "lib/")
    mv("plutosvg/LICENSE", "licenses/plutosvg.txt")
    rm("plutosvg")
  else
    todo()
  end
end
function inst_stbi()
  print("Download and move stb_image...")
  if TARGET == "LINUX" or TARGET == "WIN" then
    wget("https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h", "stb_image.h")
    mv("stb_image.h", "include/stb_image.h")
  elseif TARGET == "WIN" then
    shell("iwr -OutFile include/stb_image.h -Uri https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h")
  else
    todo()
  end
end
function inst_stbiw()
  print("Download and move stb_image_write...")
  if TARGET == "LINUX" or TARGET == "WIN" then
    wget("wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h", "stb_image_write.h")
    mv("stb_image_write.h", "include/stb_image_write.h")
  elseif TARGET == "WIN" then
    shell("iwr -OutFile include/stb_image_write.h -Uri https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h")
  else
    todo()
  end
end

ensure_folder("lib")
ensure_folder("include")
ensure_folder("temp")
ensure_folder("bin")
ensure_folder("licenses")
run_install_scripts()
