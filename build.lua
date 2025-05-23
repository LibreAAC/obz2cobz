require("src/shared/shared")
SRC = {"cobz.cpp", "colors.cpp", "img.cpp", "tables.cpp", "shared/utils.cpp",
  "shared/list.cpp", "cjson.cpp", "obz2cobz.cpp", "parser.cpp", "stbi.cpp",
  "stbiw.cpp", "zip.cpp"
}
LFLAGS = "-lpthread -L lib -lplutosvg -lplutovg -lcurl"
LD_LIBRARY_PATH = ""

load_os()
parse_args()
ensure_folder("temp")
CFLAGS = "-I include -I src/shared"
if TARGET == "LINUX" then
  CFLAGS = CFLAGS .. " `pkg-config --cflags libcurl`"
elseif TARGET == "WIN" then
  if not exists("include/curl.h") then
    wget("https://curl.se/windows/latest.cgi?p=win64-mingw.zip", "curl.zip")
    extract("curl.zip")
    mv("curl-*-win64-mingw/lib/*", "lib")
    mv("curl-*-win64-mingw/include/*", "include")
    mv("curl-*-win64-mingw/COPYING.txt", "licenses/curl.txt")
    rm("curl.zip")
    rm("curl-*-win64-mingw")
  end
else
  todo()
end
CFLAGS = CFLAGS .. " -Wl,-rpath," .. LD_LIBRARY_PATH
local objs = ""
local clangd_shit = "["
for i,p in pairs(SRC) do
  local obj = "temp/"..filename(p)..".o "
  local cmd = "g++ -g -c src/"..p.." -o "..obj..CFLAGS
  local shlexed = cmd:split(" ")
  local jsonified = ('", "'):join(shlexed)
  shell(cmd)
  objs = objs .. obj
  clangd_shit = clangd_shit ..
    '{"directory":"'..CWD..'","arguments":["'..jsonified..'"],"file":"src/'..p..'"},'
end
clangd_shit = clangd_shit:sub(1, clangd_shit:len()-1) .. ']'
local ccmds = io.open("compile_commands.json","w")
ccmds:write(clangd_shit)
ccmds:close()
shell("g++ -g -o bin/obz2cobz "..objs..CFLAGS.." "..LFLAGS)

