-- Small list of dependencies by name:
-- TARGET={LINUX,WIN,ANDROID}
--  zip by kuba-- on github under MIT license
--  cJSON by DaveGamble on github under MIT license
--  plutosvg by sammycage under MIT license
--  stb_image by nothings under public domain
require("src/shared/shared")
load_os()
CJSON_VERSION = "1.7.18"
PLUTOSVG_VERSION = "0.0.6"
ZIP_VERSION = "0.3.3"
TEMP_FOLDERS = { "plutosvg" }
parse_args()

function inst_zip()
  print("Download, compile and move zip...")
  if TARGET == "LINUX" then
    shell("wget https://github.com/kuba--/zip/archive/refs/tags/v"..ZIP_VERSION..".tar.gz")
    shell("tar -xzvf v"..ZIP_VERSION..".tar.gz")
    shell("mkdir -p zip-"..ZIP_VERSION.."/build")
    shell("cd zip-"..ZIP_VERSION.."/build && cmake -DBUILD_SHARED_LIBS=true -DBUILD_STATIC_PIC=OFF .. && cmake --build .")
    mv("zip-"..ZIP_VERSION.."/src/zip.h", "include/")
    mv("zip-"..ZIP_VERSION.."/build/*.so", "lib/")
    mv("zip-"..ZIP_VERSION.."/LICENSE.txt", "licenses/zip.txt")
    rm("v"..ZIP_VERSION..".tar.gz*")
    rm("zip-"..ZIP_VERSION)
  else
    todo()
  end
end
function inst_cjson()
  print("Download and move cJSON...")
  if TARGET == "LINUX" then
    shell("wget https://github.com/DaveGamble/cJSON/archive/refs/tags/v"..CJSON_VERSION..".tar.gz")
    shell("tar -xzvf v"..CJSON_VERSION..".tar.gz")
    mv("cJSON-"..CJSON_VERSION.."/cJSON.*", "include/")
    mv("cJSON-"..CJSON_VERSION.."/LICENSE", "licenses/cJSON.txt")
    rm("v"..CJSON_VERSION..".tar.gz*")
    rm("cJSON-"..CJSON_VERSION)
  else
    todo()
  end
end
function inst_plutosvg()
  print("Download, compile and move plutosvg...")
  if TARGET == "LINUX" then
    if exists("plutosvg") then
      rm("plutosvg")
    end
    shell("git clone --recursive https://github.com/sammycage/plutosvg.git")
    shell("cd plutosvg && cmake -B build . && cmake --build build")
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
  if TARGET == "LINUX" then
    shell("wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h")
    mv("stb_image.h", "include/stb_image.h")
  else
    todo()
  end
end
function inst_stbiw()
  print("Download and move stb_image_write...")
  if TARGET == "LINUX" then
    shell("wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h")
    mv("stb_image_write.h", "include/stb_image_write.h")
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
