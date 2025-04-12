require("src/shared/shared")
SRC = {"cobz.cpp", "colors.cpp", "img.cpp", "tables.cpp", "shared/utils.cpp",
  "shared/list.cpp"
}
CFLAGS = "-I include -I src/shared"
LFLAGS = ""


load_os()
parse_args()
ensure_folder("temp")
local objs = ""
local clangd_shit = "["
for i,p in pairs(SRC) do
  local obj = "temp/"..filename(p)..".o "
  local cmd = "g++ -c src/"..p.." -o "..obj..CFLAGS
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
shell("g++ -o bin/obz2cobz "..objs..CFLAGS.." "..LFLAGS)

