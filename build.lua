require("src/shared/shared")
SRC = {"cobz.cpp", "colors.cpp", "img.cpp", "tables.cpp", "shared/utils.cpp",
  "shared/list.cpp", "cjson.cpp", "obz2cobz.cpp", "parser.cpp", "stbi.cpp",
  "stbiw.cpp"
}
CFLAGS = "-fsanitize=address -I include -I src/shared"
LFLAGS = "-lpthread -L lib -lzip -lplutosvg -lplutovg -lcurl"
LD_LIBRARY_PATH = ""

load_os()
parse_args()
ensure_folder("temp")
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

