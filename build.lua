require("src/shared/shared")
SRC = {"cobz.cpp", "colors.cpp", "img.cpp", "tables.cpp", "shared/utils.cpp",
  "shared/list.cpp", "cjson.cpp", "obz2cobz.cpp", "parser.cpp", "stbi.cpp",
  "stbiw.cpp", "zip.cpp"
}
-- -lplutosvg -lplutovg -lcurl -lz -lzstd -lssl -lpsl -lnghttp2 -lnghttp3 -lngtcp2 -lngtcp2_crypto_quictls -lssh2 -lbrotlidec -lbrotlicommon
LD_LIBRARY_PATH = ""

FINAL = false
for _, a in pairs(arg) do
  if a == "final" then
    FINAL = true
  end
end
load_os()
parse_args()
ensure_folder("temp")
ensure_folder("temp/shared")
CFLAGS = "-I include -I src/shared"
if TARGET == "LINUX" then
  LFLAGS = "-L lib -lplutosvg -lplutovg -lcurl"
  CFLAGS = CFLAGS .. " -Wl,-rpath," .. LD_LIBRARY_PATH
elseif TARGET == "WIN" then
  LFLAGS = "-L lib '-Wl,-Bstatic' -lplutosvg -lplutovg '-Wl,-Bdynamic' -L bin -lcurl-x64"
  if not exists("include/curl") then
    wget("https://curl.se/windows/latest.cgi?p=win64-mingw.zip", "curl.zip")
    extract("curl.zip")
    mv("curl-*-win64-mingw/lib/*", "lib")
    mv("curl-*-win64-mingw/include/*", "include")
    mv("curl-*-win64-mingw/COPYING.txt", "licenses/curl.txt")
    mv("curl-*-win64-mingw/bin/libcurl-x64.dll", "bin")
    rm("curl.zip")
    rm("curl-*-win64-mingw")
  end
else
  todo()
end
if FINAL then
  CFLAGS = CFLAGS .. " -O3"
end
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

