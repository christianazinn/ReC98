--[[








⚠️ CAUTION! ⚠️

You are on the `master` branch, which builds PC-98 DOS binaries that are
identical to ZUN's originally released binaries. If you want to port the game
to other architectures or develop a mod that doesn't need to be byte-for-byte
comparable to the original binary, start from the cleaned-up `debloated` branch
instead. That branch is easier to read and modify, and builds smaller and
slightly faster PC-98 binaries while leaving all bugs and quirks from ZUN's
original code in place.
Seriously, you'd just be torturing yourself if you do anything nontrivial based
on this branch.

⚠️ CAUTION! ⚠️







--]]

---@alias ReC98Input string | { [1]: string, extra_inputs: string | string[], o?: string}

tup.include("Pipeline/rules.lua")
tup.import("T1REPLAY_PROFILE=")
tup.import("T2REPLAY_PROFILE=")

---@class (exact) ConfigShape
---@field obj_root? string Root directory for all intermediate files
---@field bin_root? string Root directory for shipped binaries
---@field aflags? string
---@field cflags? string
---@field lflags? string

---@class Config
local Config = {
	obj_root = "obj/",
	bin_root = "bin/",
	aflags = "/m /mx /kh32768 /t",
	cflags = "-I. -O -b- -3 -Z -d",
	lflags = "-s",
}
Config.__index = Config

local Rules = NewRules(Config.obj_root)

MODEL_ASM = { lflags = "-t -3" }
MODEL_TINY = { cflags = "-mt", lflags = "-t" }
MODEL_SMALL = { cflags = "-ms" }

-- Currently, it only makes sense to enable extended dictionary processing for
-- binaries that don't link the original binary release of MASTERS.LIB, which
-- doesn't come with them.
MODEL_LARGE = { cflags = "-ml", lflags = "-E" }

---Generates a ConfigShape for an output subdirectory.
---@param dir string
---@return ConfigShape
function Subdir(dir)
	return { obj_root = dir, bin_root = dir }
end

---Maps each source filename to its compiled object file. Allows the same
---source file to be listed in multiple binaries – later occurrences get
---replaced with the previous build output.
---@type { [string]: string }
local PreviousOutputForSource = {}

---@param ... ConfigShape
function Config:branch(...)
	local arg = { ... }

	---@class Config
	local ret = setmetatable({}, self)
	ret.__index = self

	for field, value in pairs(self) do
		ret[field] = value
		for _, other in pairs(arg) do
			if other[field] then
				-- By only space-separating flags, we allow custom directories.
				if (field:sub(-5) == "flags") then
					ret[field] = (ret[field] .. " " .. other[field])
				else
					ret[field] = (ret[field] .. other[field])
				end
			end
		end
	end
	return ret
end

---@param input ReC98Input
function Config:build_uncached(input)
	local fn = ((type(input) == "string" and input) or input[1])
	local out_basename = (tup.base(fn):gsub("th0%d_", "") .. ".obj")
	if input.o then
		out_basename = input.o
		input.o = nil
	end
	local out_file = (self.obj_root .. out_basename)

	local ext = tup.ext(fn)
	if (ext == "asm") then
		-- We can't use %f since TASM32 wants backslashes and Tup would
		-- automatically rewrite them to slashes.
		local args = string.format("%s %s %%o", self.aflags, fn:gsub("/", "\\"))
		return Rules:add_32(input, "tasm32", args, out_file:gsub("/", "\\"))[1]
	elseif ((ext == "c") or (ext == "cpp")) then
		local args = string.format("-c %s -n%s %%f", self.cflags, self.obj_root)
		return Rules:add_16(input, "tcc", args, out_file)[1]
	elseif ((ext == "obj") or (ext == "lib")) then
		return fn
	end
	error(string.format("Unknown file extension, can't build: `%s`", fn))
end

---@param inputs ReC98Input[]
---@return string[]
function Config:build(inputs)
	local ret = {}
	for _, input in pairs(inputs) do
		local fn = ((type(input) == "string" and input) or input[1])

		local output = PreviousOutputForSource[fn]
		if (output == nil) then
			output = self:build_uncached(input)
			PreviousOutputForSource[fn] = output
		end
		ret += { output }
	end
	return ret
end

---@param exe_stem string
---@param inputs ReC98Input[]
function Config:link(exe_stem, inputs)
	local model_c = self.cflags:match("-m([tsmclh])")
	local objs = self:build(inputs)

	local obj_root = self.obj_root:gsub("/", "\\")
	local bin_root = self.bin_root:gsub("/", "\\")

	local ext = (self.lflags:find("-t") and ".com" or ".exe")
	local rsp_fn = string.format("%s%s.@l", obj_root, exe_stem)
	local map_fn = string.format("%s%s.map", obj_root, exe_stem)
	local bin_fn = string.format("%s%s%s", bin_root, exe_stem, ext)

	-- When TCC spawns TLINK, it writes the entire link command line into a
	-- file with the fixed name `turboc.$ln`. This can't ever work with
	-- parallel builds, so we have to replicate TCC's response file generation
	-- ourselves here.
	-- This not only saves one emulated process spawn call, but also gives us
	-- full control over the linker command line, allowing us to place the map
	-- file under `obj/` rather than next to the binary in `bin/`.
	-- Thankfully, we can even omit the directory for Borland's C standard
	-- library and rely on `TLINK.CFG` to supply it. Deriving it from the
	-- `PATH` would be quite annoying on Windows 9x…
	local lflags = string.format("-c %s ", self.lflags)
	if model_c then
		lflags = string.format("%sc0%s.obj", lflags, model_c)
	end

	-- Can't use %f because we need backslashes…
	local libs = ""
	for _, obj in pairs(objs) do
		if (obj:sub(-4) == ".lib") then
			libs = (libs .. obj:gsub("/", "\\") .. " ")
		else
			lflags = (lflags .. " " .. obj:gsub("/", "\\"))
		end
	end
	lflags = string.format("%s, %s, %s, %s", lflags, bin_fn, map_fn, libs)
	if model_c then
		local model_math = (model_c == "t" and "s" or model_c)
		lflags = string.format(
			"%semu.lib math%s.lib c%s.lib", lflags, model_math, model_c
		)
	end
	objs += Rules:add_32({}, "echo", (lflags .. ">%o"), rsp_fn)

	local outputs = { bin_fn, map_fn }
	return Rules:add_16(objs, "tlink", ("@" .. rsp_fn), outputs)[1]
end

-- Additional generally good compilation flags. Should be used for all code
-- that does not need to match the original binaries.
local optimized_cfg = Config:branch({ cflags = "-G -k- -p -x-" })

-- Pipeline
-- --------

local pipeline_cfg = optimized_cfg:branch(Subdir("Pipeline/"), {
	cflags = "-IPipeline/",
})

local pipeline_tool_cfg = pipeline_cfg:branch(MODEL_TINY)
local pipeline_stub_cfg = pipeline_cfg:branch(MODEL_ASM)

pipeline_tool_cfg:link("enedat", { "Pipeline/enedat.cpp" })

pipeline_tool_cfg:link("grzview", {
	"Pipeline/grzview.cpp",
	"th01/formats/grz.cpp",
	"platform/x86real/pc98/palette.cpp",
	"bin/masters.lib",
})

local bmp2arr = pipeline_tool_cfg:link("bmp2arr", {
	"Pipeline/bmp2arrl.c",
	"Pipeline/bmp2arr.c",
})

---@class BMPShape
---@field [1] string Input .BMP file
---@field [2] "asm" | "c" | "cpp" Output format
---@field [3] string Symbol
---@field [4] integer Width
---@field [5] integer Height
---@field [6] string? Additional arguments

---@param bmp BMPShape
function BMP(bmp)
	local out_fn = bmp[1]:gsub("%..+$", (bmp[2] == "asm" and ".asp" or ".csp"))
	local inputs = { bmp[1], extra_inputs = bmp2arr }
	local sym = ((bmp[2] == "asm" and "_" or "") .. bmp[3])
	local additional = ((bmp[6] and (" " .. bmp[6])) or "")
	local args = string.format(
		"-q -i %%f -o %%o -sym %s -of %s -sw %d -sh %d%s",
		sym, bmp[2], bmp[4], bmp[5], additional
	)
	return Rules:add_16(inputs, bmp2arr, args, out_fn)[1]
end

---@param bmps BMPShape[]
---@return table<string, string> sprites Sprite basename → source file
function Sprites(bmps)
	local ret = {}
	for _, bmp in pairs(bmps) do
		ret[tup.base(bmp[1])] = BMP(bmp)
	end
	return ret
end

---@param input string
function Config:stub(input)
	local obj_fn = self:build({ input })[1]
	local bin_fn = obj_fn:gsub("%..+$", ".bin")
	local args = string.format("-x -t %s, %s", obj_fn, bin_fn)
	return Rules:add_16(obj_fn, "tlink", args:gsub("/", "\\"), bin_fn)[1]
end

local zungen = pipeline_tool_cfg:link("zungen", { "Pipeline/zungen.c" })
local comcstm = pipeline_tool_cfg:link("comcstm", { "Pipeline/comcstm.c" })
local zun_stub = pipeline_stub_cfg:stub("Pipeline/zun_stub.asm")
local cstmstub = pipeline_stub_cfg:stub("Pipeline/cstmstub.asm")

---@param bin_fn string
---@param procs [string, string][] [Menu name, input filename]
function Config:zungen(bin_fn, procs)
	local rsp_fn = string.format(
		"%s%s.@z", self.obj_root, tup.base(bin_fn)
	):gsub("/", "\\")
	local inputs = { extra_inputs = { zungen, zun_stub, rsp_fn }}

	local names = ""
	local fns = ""
	for _, proc in pairs(procs) do
		inputs += { proc[2] }
		names = string.format("%s %s", names, proc[1])
		fns = string.format("%s %s", fns, proc[2])
	end

	Rules:add_32(
		{}, "echo", string.format("%d%s%s>%%o", #procs, names, fns), rsp_fn
	)
	return Rules:add_16(inputs, zungen, "%2i %3i %o", bin_fn)[1]
end

---@param out_basename string
---@param prog_fn string
---@param usage_fn string
---@param timestamp number
function Config:comcstm(out_basename, usage_fn, prog_fn, timestamp)
	local inputs = { comcstm, usage_fn, prog_fn, cstmstub }
	local cmd = string.format("%%2f %%3f %%4f %d %%o", timestamp)
	return Rules:add_16(
		inputs, comcstm, cmd, (self.bin_root .. out_basename)
	)[1]
end
-- --------

-- Third-party libraries
-- ---------------------

local piload_cfg = Config:branch({ aflags = "/ml" })
local piloadc = piload_cfg:build({ "libs/piloadc/piloadc.asm" })
local piloadm = piload_cfg:build({ "libs/piloadc/piloadm.asm" })
local sprite16 = Config:branch({ aflags = "/dTHIEF" }):build({
	{ "libs/sprite16/sprite16.asm", o = "th03/zunsp.obj" }
})[1]
-- ---------------------

-- Games
-- -----

---@param game number
function GameShape(game)
	local ret = Subdir(string.format("th0%d/", game))
	ret.aflags = string.format("/dGAME=%d", game)
	ret.cflags = string.format("-DGAME=%d", game)
	return ret
end

local T1REPLAY_PROFILES = {
	["t1exact-capture"] = {
		obj_root = "x/c/",
		bin_root = "x/c/",
		cflags = "-DT1RP=1",
	},
	["t1exact-sequential"] = {
		obj_root = "x/s/",
		bin_root = "x/s/",
		cflags = "-DT1RP=2",
	},
	["t1exact-direct"] = {
		obj_root = "x/d/",
		bin_root = "x/d/",
		cflags = "-DT1RP=3",
	},
	["t1pixel-sequential"] = {
		obj_root = "x/q/",
		bin_root = "x/q/",
		cflags = "-DT1RP=4",
	},
	["t1pixel-direct"] = {
		obj_root = "x/r/",
		bin_root = "x/r/",
		cflags = "-DT1RP=5",
	},
	["t1konngara-phase1"] = {
		obj_root = "x/k/",
		bin_root = "x/k/",
		cflags = "-DT1RP=6",
	},
	["t1konngara-phase1-direct"] = {
		obj_root = "x/kd/",
		bin_root = "x/kd/",
		cflags = "-DT1RP=8",
	},
	["t1yuugenmagan-first-combat"] = {
		obj_root = "x/y/",
		bin_root = "x/y/",
		cflags = "-DT1RP=7",
	},
	["t1yuugenmagan-first-combat-direct"] = {
		obj_root = "x/yd/",
		bin_root = "x/yd/",
		cflags = "-DT1RP=9",
	},
	["t1elis-first-combat"] = {
		obj_root = "x/e/",
		bin_root = "x/e/",
		cflags = "-DT1ELXN=1",
	},
	["t1elis-first-combat-direct"] = {
		obj_root = "x/ed/",
		bin_root = "x/ed/",
		cflags = "-DT1ELXD=1",
	},
	["t1kikuri-first-combat"] = {
		obj_root = "x/kn/",
		bin_root = "x/kn/",
		cflags = "-DT1KIKN=1",
	},
	["t1kikuri-first-combat-direct"] = {
		obj_root = "x/kid/",
		bin_root = "x/kid/",
		cflags = "-DT1KIKD=1",
	},
	["t1sariel-first-combat"] = {
		obj_root = "x/sn/",
		bin_root = "x/sn/",
		cflags = "-DT1SARN=1",
	},
	["t1sariel-first-combat-direct"] = {
		obj_root = "x/sd/",
		bin_root = "x/sd/",
		cflags = "-DT1SARD=1",
	},
	["t1score-proof"] = {
		obj_root = "x/p/",
		bin_root = "x/p/",
		cflags = "-DT1REPLAY_FUUIN_SCORE_PROOF=1",
	},
	["t1savestate-acceptance-20260828"] = {
		obj_root = "x/g/",
		bin_root = "x/g/",
		cflags = "-DT1SGA=1",
	},
}
local t1replay_profile_name = tostring(T1REPLAY_PROFILE or "")
local t1replay_profile = {}
if t1replay_profile_name ~= "" then
	t1replay_profile = T1REPLAY_PROFILES[t1replay_profile_name]
end
if (t1replay_profile_name ~= "") and (t1replay_profile == nil) then
	error(string.format(
		"Unsupported T1REPLAY_PROFILE: `%s`", t1replay_profile_name
	))
end

local th01 = Config:branch(GameShape(1), t1replay_profile)
-- FUUIN score proof has no REIIDEN component. Keep its private output root,
-- but do not let the FUUIN-only define select a score-proof main source there.
local th01_reiiden = th01
if t1replay_profile_name == "t1score-proof" then
	th01_reiiden = Config:branch(GameShape(1), {
		obj_root = t1replay_profile.obj_root,
		bin_root = t1replay_profile.bin_root,
	})
end
local T2REPLAY_PROFILES = {
	["t2practice-diagnostics-rc25"] = {
		obj_root = "x/p/",
		bin_root = "x/p/",
		-- Keep the DOS compiler command tail under 127 bytes. The public names
		-- are expanded by practice_diag.hpp.
		cflags = "-DT2PD=1 -DT2PID=25",
	},
	["t2exact-direct"] = {
		obj_root = "x/d/",
		bin_root = "x/d/",
		cflags = "-DT2REPLAY_EXACT_APPLY=1 -DT2REPLAY_EXACT_TRACE=1",
	},
	["t2savestate-acceptance-20260828"] = {
		obj_root = "x/g/",
		bin_root = "x/g/",
		cflags = "-DT2SGA=1",
	},
}
local t2replay_profile_name = tostring(T2REPLAY_PROFILE or "")
local t2replay_profile = {}
if t2replay_profile_name ~= "" then
	t2replay_profile = T2REPLAY_PROFILES[t2replay_profile_name]
end
if (t2replay_profile_name ~= "") and (t2replay_profile == nil) then
	error(string.format(
		"Unsupported T2REPLAY_PROFILE: `%s`", t2replay_profile_name
	))
end

local th02 = Config:branch(GameShape(2))
local th02_replay = th02:branch(t2replay_profile)
local th02_main = th02_replay
local th03 = Config:branch(GameShape(3))
local th04 = Config:branch(GameShape(4))
local th05 = Config:branch(GameShape(5))
-- -----

-- TH01
-- ----

local th01_sprites = Sprites({
	{ "th01/sprites/leaf_s.bmp", "cpp", "sSPARK", 8, 8 },
	{ "th01/sprites/leaf_l.bmp", "cpp", "sLEAF_LEFT", 8, 8 },
	{ "th01/sprites/leaf_r.bmp", "cpp", "sLEAF_RIGHT", 8, 8 },
	{ "th01/sprites/ileave_m.bmp", "cpp", "sINTERLEAVE_MASKS", 8, 8 },

	-- ZUN bug: Supposed to be 8 preshifted sprites, with a height of 1 and a
	-- width of SHOOTOUT_LASER_MAX_W pixels each, but ZUN messed up a single
	-- pixel in the first pre-shifted sprite.
	-- So, we're forced to manually unroll them…
	{ "th01/sprites/laser_s.bmp", "cpp", "sSHOOTOUT_LASER", 16, 8 },

	{ "th01/sprites/mousecur.bmp", "cpp", "sMOUSE_CURSOR", 16, 16 },
	{ "th01/sprites/pellet.bmp", "cpp", "sPELLET", 8, 8, "-pshf inner" },
	{ "th01/sprites/pellet_c.bmp", "cpp", "sPELLET_CLOUD", 16, 16 },
	{ "th01/sprites/pillar.bmp", "cpp", "sPILLAR", 32, 8 },
	{ "th01/sprites/shape8x8.bmp", "cpp", "sSHAPE8X8", 8, 8 },
	{ "th01/sprites/bonusbox.bmp", "cpp", "sSTAGEBONUS_BOX", 8, 4 },
})

local th01_zunsoft = th01:branch(MODEL_TINY):link("zunsoft", {
	"th01/zunsoft.cpp",
	"bin/masters.lib",
})
th01:branch(MODEL_LARGE, { cflags = "-DBINARY='O'" }):link("op", {
	piloadc,
	"th01/op_01.cpp",
	"th01/frmdelay.cpp",
	{ "th01/vsync.cpp", extra_inputs = th01_sprites["mousecur"] },
	"th01/ztext.cpp",
	"th01/initexit.cpp",
	"th01/graph.cpp",
	"th01/ptn_0to1.cpp",
	"th01/vplanset.cpp",
	"th01/op_07.cpp",
	"th01/grp_text.cpp",
	"th01/ptn.cpp",
	"th01/op_09.cpp",
	"th01/f_imgd.cpp",
	"th01/grz.cpp",
	"th01_op.asm",
	"th01/resstuff.cpp",
	"th01/mdrv2.cpp",
	"th01/pf.cpp",
	-- Keep patch-owned title state after every original OP object so new BSS
	-- cannot move original resident variables.
	"th01/rpyop.cpp",
	"th01/language.cpp",
})
th01_reiiden:branch(MODEL_LARGE, { cflags = "-DBINARY='M'" }):link("reiiden", {
	piloadc,
	"th01/main_01.cpp",
	"th01/frmdelay.cpp",
	{ "th01/vsync.cpp", extra_inputs = th01_sprites["mousecur"] },
	"th01/ztext.cpp",
	"th01/initexit.cpp",
	"th01/graph.cpp",
	"th01/ptn_0to1.cpp",
	"th01/vplanset.cpp",
	"th01/main_07.cpp",
	"th01/ptn.cpp",
	"th01/main_08.cpp",
	"th01/f_imgd.cpp",
	"th01/grz.cpp",
	"th01_reiiden.asm",
	{ "th01/main_09.cpp", extra_inputs = th01_sprites["pellet_c"] },
	"th01/bullet_l.cpp",
	"th01/grpinv32.cpp",
	"th01/resstuff.cpp",
	"th01/scrollup.cpp",
	"th01/egcrows.cpp",
	{ "th01/pgtrans.cpp", extra_inputs = th01_sprites["ileave_m"] },
	"th01/2x_main.cpp",
	"th01/egcwave.cpp",
	"th01/grph1to0.cpp",
	"th01/main_14.cpp",
	{ "th01/main_15.cpp", extra_inputs = th01_sprites["laser_s"] },
	"th01/mdrv2.cpp",
	"th01/main_17.cpp",
	{ "th01/main_18.cpp", extra_inputs = th01_sprites["bonusbox"] },
	"th01/main_19.cpp",
	"th01/main_20.cpp",
	"th01/main_21.cpp",
	"th01/pf.cpp",
	{ "th01/main_23.cpp", extra_inputs = th01_sprites["shape8x8"] },
	"th01/main_24.cpp",
	"th01/main_25.cpp",
	"th01/main_26.cpp",
	"th01/main_27.cpp",
	"th01/main_28.cpp",
	{ "th01/main_29.cpp", extra_inputs = th01_sprites["pillar"] },
	"th01/main_30.cpp",
	"th01/main_31.cpp",
	"th01/main_32.cpp",
	"th01/main_33.cpp",
	"th01/main_34.cpp",
	"th01/main_35.cpp",
	{ "th01/main_36.cpp", extra_inputs = {
		th01_sprites["leaf_s"],
		th01_sprites["leaf_l"],
		th01_sprites["leaf_r"],
	} },
	"th01/main_37.cpp",
	{ "th01/main_38.cpp", extra_inputs = th01_sprites["pellet"] },
	-- Must stay last: These mod-only replay/checkpoint segments preserve every
	-- original REIIDEN data/BSS offset. Each new owner gets its own tail segment
	-- rather than growing the original gameplay contribution it reconstructs.
	"th01/replay.cpp",
	"th01/rstage.cpp",
	"th01/rboss.cpp",
	"th01/rpypause.cpp",
	"th01/language.cpp",
	"th01/rroute.cpp",
	"th01/rpresent.cpp",
	"th01/rpypixel.cpp",
	"th01/t1ymx.cpp",
	"th01/t1elx.cpp",
	"th01/t1kik.cpp",
	"th01/t1sar.cpp",
})
th01:branch(MODEL_LARGE, { cflags = "-DBINARY='E'" }):link("fuuin", {
	piloadc,
	"th01/fuuin_01.cpp",
	"th01/input_mf.cpp",
	"th01/fuuin_02.cpp",
	"th01/fuuin_03.cpp",
	"th01/fuuin_04.cpp",
	{ "th01/vsync.cpp", extra_inputs = th01_sprites["mousecur"] },
	"th01/ztext.cpp",
	"th01/initexit.cpp",
	"th01/graph.cpp",
	"th01/grp_text.cpp",
	"th01/fuuin_10.cpp",
	"th01/f_imgd_f.cpp",
	"th01/vplanset.cpp",
	"th01/fuuin_11.cpp",
	"th01/2x_fuuin.cpp",
	"th01/mdrv2.cpp",
	"th01_fuuin.asm",
	-- Keep FUUIN's patch-owned replay and presentation state after every
	-- original object so neither can move an original owner.
	"th01/rpyfuuin.cpp",
	"th01/language.cpp",
	"th01/langfuu.cpp",
})
-- ----

-- TH02
-- ----

local th02_sprites = Sprites({
	{ "th02/sprites/bombpart.bmp", "asm", "sBOMB_PARTICLES", 8, 8 },
	{ "th02/sprites/pellet.bmp", "asm", "sPELLET", 8, 8, "-pshf outer" },
	{ "th02/sprites/sparks.bmp", "asm", "sSPARKS", 8, 8, "-pshf outer" },
	{ "th02/sprites/pointnum.bmp", "asm", "sPOINTNUMS", 8, 8 },
	{ "th02/sprites/verdict.bmp", "cpp", "sVERDICT_MASKS", 16, 16 },
})

local th02_zuninit = th02:branch(MODEL_ASM):link("zuninit", {
	"th02_zuninit.asm",
})
th02:zungen("bin/th02/zun.com", {
	{ "ONGCHK", "libs/kaja/ongchk.com" },
	{ "ZUNINIT", th02_zuninit },
	{ "ZUN_RES", th02:branch(MODEL_TINY):link("zun_res", {
		"th02/zun_res1.cpp",
		"th02/zun_res2.cpp",
		"bin/masters.lib",
	}) },
	{ "ZUNSOFT", th01_zunsoft },
})
th02_replay:branch(MODEL_LARGE, { cflags = "-DBINARY='O'" }):link("op", {
	"th02/op_01.cpp",
	"th02/exit_dos.cpp",
	"th02/zunerror.cpp",
	"th02/grppsafx.cpp",
	"th02_op.asm",
	"th01/vplanset.cpp",
	"th02/pi_load.cpp",
	"th02/grp_rect.cpp",
	"th02/frmdely2.cpp",
	"th02/input_rs.cpp",
	"th02/initop.cpp",
	"th02/exit.cpp",
	"th02/snd_mmdr.c",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th02/snd_load.cpp",
	"th02/pi_put.cpp",
	"th02/snd_kaja.cpp",
	"th02/op_02_3.cpp",
	"th02/snd_se_r.cpp",
	"th02/snd_se.cpp",
	"th02/frmdely1.cpp",
	"th02/op_03.cpp",
	"th02/globals.cpp",
	"th02/op_04.cpp",
	"th02/op_05.cpp",
	"th02/op_music.cpp",
	-- Keep process-local language state after every OP owner and replay tail.
	"th02/lang_op.cpp",
	-- English v1.00's remaining OP-resident presentation strings. Keep this
	-- ungrouped CODE segment last so no stock initialized-data address moves.
	"th02/op/langstr.asm",
})
local th02_main_sources = {
	{ "th02_main.asm", extra_inputs = {
		th02_sprites["pellet"],
		th02_sprites["bombpart"],
		th02_sprites["sparks"],
		th02_sprites["pointnum"],
	} },
	"th02/main/entry.cpp",
	"th02/mpn_put.cpp",
	"th02/pf_i.asm",
	"th02/spark.cpp",
	"th02/spark_i.asm",
	"th02/tile.cpp",
	"th02/main/stage/init.cpp",
	"th02/main/hud/menu.cpp",
	"th02/main/scroll.cpp",
	"th02/main/player/shot.cpp",
	"th02/main/bgm_show.cpp",
	"th02/main/demo.cpp",
	"th02/main/stage/loop.cpp",
	"th02/main/cfg_load.cpp",
	"th02/pointnum.cpp",
	"th02/item.cpp",
	"th02/hud.cpp",
	"th02/main/player/bombload.cpp",
	"th02/player_b.cpp",
	"th02/player.cpp",
	"th02/zunerror.cpp",
	"th02/keydelay.cpp",
	"th02/main02_1.cpp",
	"th01/vplanset.cpp",
	"th02/pi_load.cpp",
	"th02/vector.cpp",
	"th02/frmdely1.cpp",
	"th02/input_rs.cpp",
	"th02/exit.cpp",
	"th02/snd_mmdr.c",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th02/snd_dlyv.c",
	"th02/snd_load.cpp",
	"th02/mpn_l_i.cpp",
	"th02/initmain.cpp",
	"th02/pi_put.cpp",
	"th02/snd_kaja.cpp",
	"th02/snd_dlym.cpp",
	"th02/snd_se_r.cpp",
	"th02/snd_se.cpp",
	"th02/main_03.cpp",
	"th02/hud_ovrl.cpp",
	"th02/explode.cpp",
	"th02/bullet.cpp",
	"th02/main/stage/stages.cpp",
	"th02/main/midboss/m3.cpp",
	"th02/main/boss/b3.cpp",
	"th02/dialog.cpp",
	"th02/main/stage/s1.cpp",
	"th02/main/midboss/m1.cpp",
	"th02/main/boss/b1.cpp",
	"th02/main/stage/s2.cpp",
	"th02/main/midboss/m2.cpp",
	"th02/main/boss/b2m.cpp",
	"th02/main/boss/b2.cpp",
	"th02/main/midboss/mx.cpp",
	"th02/main/boss/b6.cpp",
	"th02/main/enemy/update.cpp",
	"th02/boss_5.cpp",
	"th02/main/boss/b5.cpp",
	"th02/main/midboss/m4.cpp",
	"th02/main/boss/b4.cpp",
	"th02/main_04.cpp",
	"th02/main_05.cpp",
	"th02/regist_m.cpp",
	-- Must stay last: These patch-only segments and their BSS contributions
	-- preserve every original MAIN.EXE data/BSS offset. Each patch segment
	-- follows the previous seam so later parcels cannot move existing code.
	"th02/main/replay.cpp",
	"th02/main/practice.cpp",
	"th02/main/s1_actor.cpp",
	"th02/main/s2_actor.cpp",
	"th02/main/s3_actor.cpp",
	"th02/main/s4_actor.cpp",
	"th02/main/s5_actor.cpp",
	"th02/main/s5_tile.cpp",
	"th02/main/s6_actor.cpp",
	-- TCC shortens actor_core.cpp to actor_~1.obj under the DOS output root.
	{ "th02/main/actor_core.cpp", o = "actor_~1.obj" },
	-- Patch-owned Pause terminal-action tail. It must remain after every
	-- existing patch contributor so it cannot move a retained offset.
	-- TCC shortens pause_replay.cpp to pause_~1.obj under the DOS output root.
	{ "th02/main/pause_replay.cpp", o = "pause_~1.obj" },
	-- This private Stage 5 exact-codec tail follows every previous patch tail
	-- so it cannot move retained replay or Practice offsets.
	"th02/main/s5_fx.cpp",
	-- The semantic palette codec remains a separate ungrouped tail so it does
	-- not move any preceding patch code or original data/BSS offset.
	-- TCC shortens s5_palette.cpp to s5_pal~1.obj under the DOS output root.
	{ "th02/main/s5_palette.cpp", o = "s5_pal~1.obj" },
	-- Stable Stage 3 direct-Practice constructors live in their own final tail.
	"th02/main/s3_pract.cpp",
	-- The North Stone phase-4 direct target is separately appended so none of
	-- the native or earlier Stage 3 Practice contributions can move.
	"th02/main/s3_north.cpp",
	-- Final Stage 5 callback/redraw capture recipes remain pointer-free and
	-- live in a new ungrouped tail after every retained patch contribution.
	"th02/main/s5_cbred.cpp",
	-- The language substrate is process-local and must not move prior state.
	"th02/langm.cpp",
	-- SAVESTATE GUARD MOD: The TH04/TH05 source is game-generic and derives
	-- TH02's 8.3 filenames from GAME. Keep it at the final patch-owned tail.
	"th02/main/rp_guard.cpp",
	-- Public later-boss Phase 1 constructors remain in their own final tail.
	-- TCC shortens later_boss_practice.cpp to later_~1.obj under DOS.
	{ "th02/main/later_boss_practice.cpp", o = "later_~1.obj" },

}
th02_main:branch(MODEL_LARGE, { cflags = "-DBINARY='M'" }):link("main", th02_main_sources)
th02:branch(MODEL_LARGE, { cflags = "-DBINARY='E'" }):link("maine", {
	{ "th02/end.cpp", extra_inputs = th02_sprites["verdict"] },
	"th02_maine.asm",
	"th02/grppsafx.cpp",
	"th02/keydelay.cpp",
	"th01/vplanset.cpp",
	"th02/pi_load.cpp",
	"th02/frmdely1.cpp",
	"th02/maine022.cpp",
	"th02/input_rs.cpp",
	"th02/exit.cpp",
	"th02/snd_mmdr.c",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th02/snd_load.cpp",
	"th02/initmain.cpp",
	"th02/pi_put.cpp",
	"th02/snd_kaja.cpp",
	"th02/snd_dlym.cpp",
	"th02/globals.cpp",
	"th02/maine_03.cpp",
	"th02/maine_04.cpp",
	"th02/staff.cpp",
	-- Link the MAINE preference/overlay reader after every end-game owner.
	"th02/lange.cpp",
	-- MAINE-only English v1.00 resident copy. Keep it after the C++ tail so
	-- every stock segment and DGROUP contribution remains pinned.
	"th02/end/maine_langstr.asm",
})
-- ----

-- TH03
-- ----

local th03_sprites = Sprites({
	{ "th03/sprites/score.bmp", "asm", "sSCORE_FONT", 8, 8, "-u" },

	-- ZUN bloat: Investing 32 bytes just so that the individual rows can be
	-- loaded with a 16-bit `MOV`…
	{ "th03/sprites/flake.bmp", "asm", "sFLAKE", 16, 8 },

	-- Double-preshifting just to ensure word-aligned VRAM writes? Was this
	-- really worth the added 384 bytes?
	{ "th03/sprites/pellet.bmp", "asm", "sPELLET", 32, 4 },
})

th03:zungen("bin/th03/zun.com", {
	{ "-1", "libs/kaja/ongchk.com" },
	{ "-2", th02_zuninit },
	{ "-3", th01_zunsoft },
	{ "-4", th03:branch(MODEL_ASM):link("zunsp", { sprite16 }) },
	{ "-5", th03:branch(MODEL_TINY):link("res_yume", {
		"th03/res_yume.cpp",
		"bin/masters.lib",
	})}
})
th03:branch(MODEL_LARGE, { cflags = "-DBINARY='O'" }):link("op", {
	"th03/op_01.cpp",
	"th03/opsfmdat.asm",
	"th03/opbss.asm",
	"th03/optext.asm",
	"th03/op_music.cpp",
	"th03/op_main.cpp",
	"th03/op_02.cpp",
	"th03/scoredat.cpp",
	"th03/op_sel.cpp",
	"th02/exit_dos.cpp",
	"th01/vplanset.cpp",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th02/snd_load.cpp",
	"th03/exit.cpp",
	"th03/polar.cpp",
	"th03/cdg_put.asm",
	"th02/frmdely1.cpp",
	"th03/input_s.cpp",
	"th03/pi_put.cpp",
	"th03/snd_kaja.cpp",
	"th03/initop.cpp",
	"th03/cdg_load.cpp",
	"th03/grppsafx.cpp",
	"th03/pi_load.cpp",
	"th03/inp_m_w.cpp",
	"th03/cdg_p_na.asm",
	"th03/hfliplut.cpp",
	"th02/frmdely2.cpp",
})
th03:branch(MODEL_LARGE, { cflags = "-DBINARY='M'" }):link("main", {
	"th03/main/main_03_prefix.asm",
	"th03/main_03.cpp",
	"th03/main/main_03_dispatch.asm",
	"th03/main_03p.cpp",
	"th03/main/main_03_mima_update.asm",
	"th03/main_03q.cpp",
	"th03/main/main_03_yumemi_update.asm",
	"th03/main_03r.cpp",
	"th03/main/main_03_reimu_update.asm",
	"th03/main_03s.cpp",
	"th03/main/main_03_ellen_update.asm",
	"th03/main_03t.cpp",
	"th03/main/main_03_kotohime_update.asm",
	"th03/main_03u.cpp",
	"th03/main/main_03_chiyuri_update.asm",
	"th03/main_03v.cpp",
	"th03/main/main_03_kana_update.asm",
	"th03/main_03w.cpp",
	"th03/main/main_03_rikako_update.asm",
	"th03/main_03x.cpp",
	"th03/main/rand2.cpp",
	"th03/main/main_04_randring.asm",
	"th03/main/main_04_hitbox_prefix.asm",
	"th03/main/player/ch_shot.cpp",
	"th03/main/main_06_anchor.asm",
	"th03/main/player/chybomb.cpp",
	"th03/main_06.cpp",
	"th03/main_07.cpp",
	"th03/main_08.cpp",
	"th03/main_09.cpp",
	"th03/main_10.cpp",
	"th03/main_11.cpp",
	"th03/main/hitc_prf.asm",
	"th03/main/hitc_pal.cpp",
	"th03/main/randfill.cpp",
	"th03/main/hitc_mid.asm",
	"th03/main/hitc_mrs.cpp",
	"th03/main/entry.cpp",
	"th03/main/rndloop.cpp",
	"th03/main/roundcb.cpp",
	"th03/main/rstart.cpp",
	"th03/main/story_startup_data.asm",
	"th03/main/support_format_data.asm",
	"th03/main/gba_hitc_enemy_data.asm",
	"th03/main/pbpat_data.asm",
	"th03/main/warn_data.asm",
	"th03/main/cmb_data.asm",
	"th03/main/soch_data.asm",
	{ "th03/main/sfntdat.asm", extra_inputs = th03_sprites["score"] },
	"th03/main/ccorddat.asm",
	"th03/main/rtextdat.asm",
	{ "th03/main/peldat.asm", extra_inputs = th03_sprites["pellet"] },
	"th03/main/round_gate_bss.asm",
	"th03/main/support_resident_bss.asm",
	"th03/main/palette_gba_bss.asm",
	"th03/main/gba_boss_bss.asm",
	"th03/main/randring_bss.asm",
	"th03/main/chiyuri_bss.asm",
	"th03/main/enemy_formation_bss.asm",
	"th03/main/ellen_bss.asm",
	"th03/main/kana_bss.asm",
	"th03/main/hitcircle_bss.asm",
	"th03/main/shot_kotohime_bss.asm",
	"th03/main/exatt_bss.asm",
	"th03/main/chargeshot_gba_bss.asm",
	"th03/main/gba_exatt_bomb_bss.asm",
	"th03/main/hud_player_gba_bss.asm",
	"th03/main/hitbox_defeat_rikako_bss.asm",
	"th03/main/gba_combo_playerm_bss.asm",
	"th03/main/enemy_explosion_score_bss.asm",
	"th03/main/yumemi_rikako_small_bss.asm",
	"th03/main/collmap_bss.asm",
	"th03/main/bomb_player_pid_bss.asm",
	"th03/main/round_frame_bss.asm",
	"th03/main/player_shot_result_tail_bss.asm",
	"th03/maintext.asm",
	"th03/hitcb.cpp",
	"th03/main/hitc_sfx.asm",
	"th03/main_05.cpp",
	"th03/main/resscore.cpp",
	"th03/playfld.cpp",
	"th03/cfg_lres.cpp",
	"th03/hitcirc.cpp",
	"th03/hud_stat.cpp",
	"th03/main/player/bb12.cpp",
	"th03/main/player/bdc2.cpp",
	"th03/main/player/be2a.cpp",
	"th03/main/player/hudstart.cpp",
	"th03/main/player/defeat.cpp",
	"th03/main/player/c2f9.cpp",
	"th03/main/player/c433.cpp",
	"th03/main/player/c54a.cpp",
	"th03/main/player/c568.cpp",
	"th03/main/player/c7a5.cpp",
	"th03/main/player/gaugeovl.cpp",
	"th03/main/player/warning.cpp",
	"th03/main/player/warning_cacb.asm",
	"th03/main/player/d3f9.cpp",
	"th03/main/player/scoreblt.cpp",
	"th03/main/player/score_rn.asm",
	"th03/player_m.cpp",
	"th03/main/player/bomb.cpp",
	"th03/main_010.cpp",
	"th03/p_shot.cpp",
	"th01/vplanset.cpp",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th03/vector.cpp",
	"th03/exit.cpp",
	"th03/polar.cpp",
	"th02/frmdely1.cpp",
	"th03/input_s.cpp",
	"th02/snd_se_r.cpp",
	"th03/snd_se.cpp",
	"th03/snd_kaja.cpp",
	"th03/initmain.cpp",
	"th03/pi_load.cpp",
	"th03/inp_m_w.cpp",
	"th03/collmap.asm",
	"th03/main/colvline.cpp",
	"th03/collmap_slope.asm",
	"th03/mbomb.cpp",
	"th03/bullet.cpp",
	"th03/e_enemy.cpp",
	"th03/hitbox.cpp",
	"th03/p_combo.cpp",
	"th03/p_gauge.cpp",
	"th03/e_expl.cpp",
	"th03/e_fireb.cpp",
	"th03/p_exatt.cpp",
	"th03/hfliplut.cpp",
	"th03/mrs.cpp",
	"th03/sprite16.cpp",
})
th03:branch(MODEL_LARGE, { cflags = "-DBINARY='L'" }):link("mainl", {
	"th03/cfg_lres.cpp",
	"th03/mainl_sc.cpp",
	"th03/mainl/screens_data.asm",
	"th03/mainl/mlsfmdat.asm",
	"th03/mainl/ccutdat.asm",
	"th03/mainl/rsedat.asm",
	{ "th03/mainl/fldsdat.asm", extra_inputs = th03_sprites["flake"] },
	"th03/mainl/vstfdat.asm",
	"th03/mainl/mainl_03_anchor.asm",
	"th03/mainl/cdgunput.cpp",
	"th03/mainl/stf_bclr.cpp",
	"th03/mainl/cdgdislv.cpp",
	"th03/mainl/stf_cdg.cpp",
	"th03/mainl/stf_vrd.cpp",
	"th03/mainl/stf_drv.cpp",
	"th03/mainl/mlentry.cpp",
	"th03/mainl/mlsbss.asm",
	"th03/mainl/mlrhbss.asm",
	"th03/mainl/mlsvbss.asm",
	"th03/mainl/mltext.asm",
	"th03/mainl/ending.cpp",
	"th03/cutscene/continue.cpp",
	"th03/cutscene.cpp",
	"th03/scoredat.cpp",
	"th03/regist.cpp",
	"th03/staff.cpp",
	"th01/vplanset.cpp",
	"th02/snd_mode.c",
	"th02/snd_pmdr.c",
	"th02/snd_dlyv.c",
	"th02/snd_load.cpp",
	"th03/vector.cpp",
	"th03/exit.cpp",
	"th03/cdg_put.asm",
	"th02/frmdely1.cpp",
	"th03/input_s.cpp",
	"th03/pi_put.cpp",
	"th03/pi_put_i.cpp",
	"th02/snd_se_r.cpp",
	"th03/snd_se.cpp",
	"th03/snd_kaja.cpp",
	"th03/initmain.cpp",
	"th03/cdg_load.cpp",
	"th03/exitmain.cpp",
	"th03/grppsafx.cpp",
	"th03/snd_dlym.cpp",
	"th03/inp_wait.cpp",
	"th03/pi_load.cpp",
	"th03/pi_put_q.cpp",
	"th03/inp_m_w.cpp",
	"th03/cdg_p_na.asm",
	"th03/hfliplut.cpp",
})
-- ----

-- TH04
-- ----

local th04_sprites = Sprites({
	{"th04/sprites/pelletbt.bmp", "asm", "sPELLET_BOTTOM", 8, 4, "-pshf outer"},
	{"th04/sprites/pointnum.bmp", "asm", "sPOINTNUMS", 8, 8, "-pshf inner"},
})

local th04_zuncom = th04:zungen("obj/th04/zuncom.bin", {
	{ "-O", "libs/kaja/ongchk.com" },
	{ "-I", th04:branch(MODEL_ASM):link("zuninit", { "th04_zuninit.asm" }) },
	{ "-S", th04:branch(MODEL_TINY):link("res_huma", {
		"th04/res_huma.cpp",
		"bin/masters.lib",
	}) },
	-- `th04/memchka.cpp`, not `memchk.cpp`: the object basename must be unique
	-- across obj/th04/, and `th04_memchk.asm` already claims `memchk.obj`
	-- (kb/codegen/0071). It MUST stay ahead of the dump -- it carries main(),
	-- which is the first thing both the _TEXT and the _DATA contribution emit.
	{ "-M", th04:branch(MODEL_TINY):link("memchk", {
		"th04/memchka.cpp",
		"th04_memchk.asm",
	}) },
})
th04:comcstm("zun.com", "th04/zun.txt", th04_zuncom, 621381155)
th04:branch(MODEL_LARGE, { cflags = "-DBINARY='O'" }):link("op", {
	"th04/op_main.cpp",
	"th04_op.asm",
	"th01/vplanset.cpp",
	"th02/frmdely1.cpp",
	"th03/pi_put.cpp",
	"th03/pi_load.cpp",
	"th03/hfliplut.cpp",
	"th04/input_w.cpp",
	"th04/vector.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_kaja.cpp",
	"th04/cdg_p_nc.asm",
	"th04/snd_mode.cpp",
	"th04/snd_dlym.cpp",
	"th02/exit_dos.cpp",
	"th04/snd_load.cpp",
	"th04/grppsafx.asm",
	"th04_op2.asm",
	"th04/cdg_put.asm",
	"th04/exit.cpp",
	"th04/initop.cpp",
	"th04/cdg_p_na.cpp",
	"th04/input_s.asm",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th04/egcrect.cpp",
	"th04/bgimage.cpp",
	"th04/bgimager.asm",
	"th04/cdg_load.asm",
	"th02/frmdely2.cpp",
	"th04/op_setup.cpp",
	"th04/zunsoft.cpp",
	"th04/op_music.cpp",
	"th04/score_db.cpp",
	"th04/score_e.cpp",
	"th04/hi_view.cpp",
	"th04/op_title.cpp",
	"th04/m_char.cpp",
	"th04/rpyop.cpp",
})
local th04_main_inputs = {
	{ "th04_main.asm", extra_inputs = {
		th02_sprites["pellet"],
		th02_sprites["sparks"],
		th04_sprites["pelletbt"],
		th04_sprites["pointnum"],
	} },
	"th04/mpn_put.cpp",
	"th04/maintext_tail.asm",
	"th04/slowdown.cpp",
	"th04/entry.cpp",
	"th04/stg_loop.cpp",
	"th04/p_marisa.cpp",
	"th04/laser_r.cpp",
	"th04/gameover.cpp",
	"th04/execl.cpp",
	"th04/demo.cpp",
	"th04/ems.cpp",
	"th04/tile_set.cpp",
	"th04/std.cpp",
	"th04/end_ext.cpp",
	"th04/map.cpp",
	"th04/it_spl_d.cpp",
	"th04/null.cpp",
	"th04/pn_inv.cpp",
	"th04/selectr.cpp",
	"th04/circle.cpp",
	"th04/bul_ginv.cpp",
	"th04/tile.cpp",
	"th04/playfld.cpp",
	"th04/midboss4.cpp",
	"th04/midbossx.cpp",
	"th04/f_dialog.cpp",
	"th04/dialog.cpp",
	"th04/boss_exp.cpp",
	"th04/boss_5r.cpp",
	"th04/boss_bg.cpp",
	-- POSITION-CRITICAL: must stay immediately before th04/boss_fg.cpp. Both
	-- contribute to BOSS_FG_TEXT, and TLINK lays a segment's contributions out
	-- in link order; bullets_render() precedes items_render() in the original.
	"th04/bullet_r.cpp",
	"th04/boss_fg.cpp",
	"th04/mai.cpp",
	"th04/stages.cpp",
	"th04/hud_pnt.cpp",
	"th04/hud_drm.cpp",
	-- POSITION-CRITICAL: must stay immediately before th04/hud_put.cpp. Both
	-- contribute to HUD_PUT_TEXT, and TLINK lays a segment's contributions
	-- out in link order; hud_bar_put() precedes hud_put() in the original.
	"th04/hud_bar.cpp",
	"th04/hud_put.cpp",
	"th04/hud_grz.cpp",
	"th04/hud_pwr.cpp",
	"th04/player_b.cpp",
	"th04/shot_inv.cpp",
	"th04/main_.cpp",
	"th04/player_m.cpp",
	"th04/player_p.cpp",
	"th04/main_0.cpp",
	-- POSITION-CRITICAL: must stay immediately before th04/scoreupd.asm, which
	-- is main_01_TEXT's other contribution. See th04/main_01.cpp.
	"th04/main_01.cpp",
	"th04/scoreupd.asm",
	-- Append-anywhere, and parked next to main_012.cpp only because it is the
	-- other half of what used to be one segment: y6_fg.cpp is Y6_FG_TEXT's
	-- ONLY C++ contribution, so its position in this list cannot reorder
	-- anything. The segment's own place in group main_01 comes from
	-- th04_main.asm, which defines it (and main_012_TEXT behind it) and is the
	-- first object linked.
	"th04/y6_fg.cpp",
	-- shots_add(), into the MAIN_012_A_TEXT head carve immediately before the
	-- remaining main_012_TEXT root contribution. Its object needs -k-.
	"th04/shotsadd.cpp",
	"th04/main_012.cpp",
	"th04/main_033.cpp",
	-- POSITION-CRITICAL: b6_next.cpp is yuuka6_phase_next() alone and must
	-- stay immediately before main_034.cpp, whose object it would otherwise
	-- shift by an odd number of bytes, dropping the padding in front of
	-- elly_1BDB4()'s generated switch table. See th04/b6_next.cpp.
	"th04/b6_next.cpp",
	"th04/main_034.cpp",
	"th04/main_035.cpp",
	-- POSITION-CRITICAL: main_36r.cpp is Reimu's half of main_036_TEXT and
	-- must stay immediately before main_036.cpp, which is Gengetsu's. They
	-- are two objects rather than one because the padding in front of their
	-- two generated switch tables is unreachable otherwise; see
	-- th04/main_36r.cpp.
	"th04/main_36r.cpp",
	"th04/main_036.cpp",
	"th04/hud_ovrl.cpp",
	"th04/cfg_lres.cpp",
	"th04/checkerb.cpp",
	"th04/mb_inv.cpp",
	"th04/boss_bd.cpp",
	"th04/score_rm.cpp",
	"th01/vplanset.cpp",
	"th03/vector.cpp",
	"th02/frmdely1.cpp",
	"th03/hfliplut.cpp",
	"th04/mpn_free.cpp",
	"th04/input_w.cpp",
	"th04/mpn_l_i.cpp",
	"th04/vector.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_kaja.cpp",
	"th04/snd_mode.cpp",
	"th04/snd_load.cpp",
	"th04/cdg_put.asm",
	"th04/exit.cpp",
	"th04/initmain.cpp",
	"th04/cdg_p_na.cpp",
	"th04/cdg_p_pr.asm",
	"th04/input_s.asm",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th04/cdg_load.asm",
	"th04/gather.cpp",
	"th04/std_run.cpp",
	"th04/enm_btpl.cpp",
	"th04/scrolly3.cpp",
	"th04/motion_3.asm",
	"th04/midboss.cpp",
	"th04/hud_hp.cpp",
	"th04/mb_dft.cpp",
	"th04/mb_dfr.cpp",
	"th04/vector2n.asm",
	"th04/spark_a.asm",
	"th04/grcg_3.cpp",
	"th04/it_spl_u.cpp",
	-- POSITION-CRITICAL: these two are the whole of MB_UPD_TEXT, the
	-- kb/codegen/0080 head carve off ENM_POS_TEXT, whose root contribution is
	-- now empty. mb_upd1.cpp must stay immediately before mb_upd.cpp: it is
	-- 0x33F bytes of prefix, and folding it into that object instead flips the
	-- parity of midboss3_update()'s `-a2` jump-table pad. See th04/mb_upd1.cpp.
	"th04/mb_upd1.cpp",
	"th04/mb_upd.cpp",
	-- POSITION-CRITICAL: enemy_u.cpp is MUGETSU_TEXT's first C++ object and
	-- replaces the root dump's former enemies_update() contribution. The four
	-- Mugetsu boss objects follow in address order. bx1_pose.cpp must
	-- be its own object because a translation unit that reaches
	-- th04/main/bullet/bullet.hpp compiles the two pose drivers' dense
	-- `switch` heads through AX instead of BX, length-neutrally; bx1_upd.cpp
	-- must be its own because mugetsu_update()'s `-a2` table pad only appears
	-- at an even object offset, which a zero prefix gives it and the 0x3D7 of
	-- bx1_ptn.cpp ahead of it does not. See th04/bx1_gath.cpp.
	"th04/enemy_u.cpp",
	"th04/bx1_gath.cpp",
	"th04/bx1_pose.cpp",
	"th04/bx1_ptn.cpp",
	"th04/bx1_upd.cpp",
	-- POSITION-CRITICAL: must stay immediately before th04/enm_pos.cpp, which
	-- is ENM_POS_TEXT's other C++ contribution. See th04/enm_pos1.cpp.
	"th04/enm_pos1.cpp",
	"th04/enm_pos.cpp",
	-- POSITION-CRITICAL: these three are B4M_UPDATE_TEXT's C++ half in
	-- address order, and enm_scr.cpp is its head. expl_sm.cpp must stay
	-- immediately before boss_4m.cpp, whose object it would otherwise shift
	-- by an odd number of bytes, moving the padding in front of both switch
	-- tables b4m.cpp generates under `-a2`. See th04/expl_sm.cpp.
	"th04/enm_scr.cpp",
	"th04/expl_sm.cpp",
	"th04/boss_4m.cpp",
	"th04/bullet_u.cpp",
	"th04/bullet_a.cpp",
	-- POSITION-CRITICAL: these three are IT_UPDT_TEXT's whole contents in
	-- address order, and hudnum.cpp is its head. The dump contributes nothing
	-- to that segment any more, so link order alone decides where the two
	-- gaiji number renderers and the bonus multipliers land.
	"th04/hudnum.cpp",
	"th04/itminit.cpp",
	"th04/it_updt.cpp",
	"th04/boss.cpp",
	"th04/boss_4r.cpp",
	"th04/boss_x2.cpp",

	-- Production oracle facade. The full validation oracle is linked only into
	-- bin/th04/oracle/main.exe below.
	"th04/orl_rel.cpp",
	-- USER REPLAY MOD: production code in an isolated REPLAY_TEXT segment.
	-- Keep mod-only segments at the tail of the link list.
	"th04/replay.cpp",
	-- PORTABLE CHECKPOINT MOD: field codecs in an isolated tail segment.
	"th04/rp_ckpt.cpp",
	-- SAVESTATE GUARD MOD: physical FAT verification in an isolated tail.
	"th04/main/rp_guard.cpp",
}
th04:branch(MODEL_LARGE, { cflags = "-DBINARY='M'" }):link(
	"main", th04_main_inputs
)

local th04_oracle_inputs = {}
for i, input in ipairs(th04_main_inputs) do
	th04_oracle_inputs[i] = (
		(input == "th04/orl_rel.cpp") and "th04/oracle.cpp" or input
	)
end
th04:branch(MODEL_LARGE, Subdir("oracle/"), {
	cflags = "-DBINARY='M'",
}):link("main", th04_oracle_inputs)
th04:branch(MODEL_LARGE, { cflags = "-DBINARY='E'" }):link("maine", {
	"th04/maine_e.cpp",
	{ "th04_maine_master.asm", o = "mainem.obj" },
	"th04/score_d.cpp",
	"th04/score_e.cpp",
	"th04/hi_end.cpp",
	"th01/vplanset.cpp",
	"th02/frmdely1.cpp",
	"th03/pi_put.cpp",
	"th03/pi_load.cpp",
	"th03/pi_put_q.cpp",
	"th03/hfliplut.cpp",
	"th04/input_w.cpp",
	"th04/vector.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_kaja.cpp",
	"th04/snd_mode.cpp",
	"th04/snd_dlym.cpp",
	"th04/cdg_p_pl.asm",
	"th04/snd_load.cpp",
	"th04/grppsafx.asm",
	"th04_maine.asm",
	"th04/cdg_put.asm",
	"th04/exit.cpp",
	"th04/initmain.cpp",
	"th04/input_s.asm",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th04/bgimage.cpp",
	"th04/bgimager.asm",
	"th04/cdg_load.asm",
	"th04/cutscene.cpp",
	"th04/staffrol.cpp",
	"th04/staff.cpp",
	-- LANGUAGE OVERLAY MOD: optional presentation assets in a trailing segment.
	"th04/rpyend.cpp",
})
-- ----

-- TH05
-- ----

local th05_sprites = Sprites({
	{ "th05/sprites/gaiji.bmp", "asm", "sGAIJI", 16, 16 },
	{ "th05/sprites/piano_l.bmp", "asm", "sPIANO_LABEL_FONT", 8, 8 },
})

local th05_zuninit_resident = th05:branch(MODEL_TINY):build({
	"th05/zuninit/resident.cpp",
})[1]

local th05_zuncom = th05:zungen("obj/th05/zuncom.bin", {
	{ "-O", "libs/kaja/ongchk.com" },
	{ "-I", th05:branch(MODEL_ASM):link("zuninit", {
		"th05_zuninit.asm",
		th05_zuninit_resident,
	}) },
	{ "-S", th05:branch(MODEL_TINY):link("res_kso", {
		"th05/res_kso.cpp",
		"bin/masters.lib",
	}) },
	{ "-G", th05:branch(MODEL_ASM):link("gjinit", {
		{ "th05_gjinit.asm", extra_inputs = th05_sprites["gaiji"] },
	}) },
	{ "-M", th05:branch(MODEL_ASM):link("memchk", { "th05_memchk.asm" }) },
})
th05:comcstm("zun.com", "th05/zun.txt", th05_zuncom, 628731748)
th05:branch(MODEL_LARGE, { cflags = "-DBINARY='O'" }):link("op", {
	"th05/op_main.cpp",
	"th05_op.asm",
	"th03/hfliplut.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_mode.cpp",
	"th02/exit_dos.cpp",
	"th04/grppsafx.asm",
	{ "th05_op2.asm", extra_inputs = th05_sprites["piano_l"] },
	"th04/cdg_p_na.cpp",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th04/bgimage.cpp",
	"th05/cdg_put.asm",
	"th04/exit.cpp",
	"th05/vector.cpp",
	"th05/musicp_c.cpp",
	"th05/musicp_a.asm",
	"th05/bgimager.asm",
	"th05/snd_load.cpp",
	"th05/snd_kaja.cpp",
	"th05/pi_cpp_1.cpp",
	"th05/pi_asm_1.asm",
	"th05/pi_cpp_2.cpp",
	"th05/pi_asm_2.asm",
	"th05/initop.cpp",
	"th05/input_s.asm",
	"th05/inp_h_w.cpp",
	"th05/snd_dlym.cpp",
	"th05/cdg_p_nc.cpp",
	"th05/frmdelay.cpp",
	"th04/cdg_load.asm",
	"th05/egcrect.cpp",
	"th05/op_setup.cpp",
	"th05/zunsoft.cpp",
	"th05/cfg.cpp",
	"th05/op_title.cpp",
	"th05/op_music.cpp",
	"th05/score_db.cpp",
	"th05/score_e.cpp",
	"th05/hi_view.cpp",
	"th05/m_char.cpp",
	"th05/rpyop.cpp",
})
local th05_main_inputs = {
	{ "th05_main.asm", extra_inputs = {
		th02_sprites["pellet"],
		th02_sprites["sparks"],
		th04_sprites["pelletbt"],
		th04_sprites["pointnum"],
	} },
	"th05/mpn_free.cpp",
	"th05/maintext_tail.asm",
	"th05/bbcheeto.cpp",
	"th04/slowdown.cpp",
	"th05/entry.cpp",
	"th05/stg_loop.cpp",
	"th05/execl.cpp",
	"th05/demo.cpp",
	"th05/ems.cpp",
	"th05/cfg_lres.cpp",
	"th05/std.cpp",
	"th05/map.cpp",
	"th05/end_ext.cpp",
	"th04/tile.cpp",
	"th05/main010.cpp",
	"th05/main011.cpp",
	"th05/it_spl_d.cpp",
	"th05/pn_inv.cpp",
	"th05/circle.cpp",
	"th05/f_dialog.cpp",
	"th05/dialog.cpp",
	"th05/boss_exp.cpp",
	"th05/playfld.cpp",
	"th05/hud_pnt.cpp",
	"th05/hud_drm.cpp",
	"th05/hud_grz.cpp",
	"th05/hud_pwr.cpp",
	-- hud_hp_put(), into the MIDBOSSX_A_TEXT that a kb/codegen/0080 head carve
	-- split off MIDBOSSX_TEXT for it. Position-free: it is that segment's only
	-- contribution, and the segment's own place in the layout comes from
	-- th05_main.asm's `group` and `segment` directives, which the dump declares
	-- first. Listed with the other HUD objects rather than with MIDBOSSX_TEXT's
	-- four, because it shares nothing with them.
	"th05/hud_hp.cpp",
	"th05/bombchar.cpp",
	"th04/mb_inv.cpp",
	"th04/boss_bd.cpp",
	"th05/boss_bg.cpp",
	-- shots_add(), into the SCORE_A_TEXT that a kb/codegen/0080 head carve
	-- split off SCORE_TEXT for it. Ahead of score_rm.cpp because that is the
	-- order the two segments have, though neither position is load-bearing:
	-- each object is the only contribution to its own segment.
	"th05/shotsadd.cpp",
	"th05/selectr.cpp",
	"th05/score_rm.cpp",
	"th05/gameover.cpp",
	"th05/laser_rh.cpp",
	"th05/null.cpp",
	"th05/player_b.cpp",
	"th05/shot_inv.cpp",
	-- Preserve the original object boundary before p_common.cpp. Its first
	-- function has a word-aligned compiler-generated switch table.
	"th05/main/player/shot_hit.cpp",
	"th05/p_common.cpp",
	"th05/p_reimu.cpp",
	"th05/p_marisa.cpp",
	"th05/p_mima.cpp",
	"th05/p_yuuka.cpp",
	"th05/player.asm",
	"th05/hud_bar.asm",
	"th05/scoreupd.asm",
	"th05/midboss5.cpp",
	"th05/b34fg.cpp",
	"th05/b6cbull.cpp",
	"th05/stages.cpp",
	"th05/midbossx.cpp",
	"th05/hud_ovrl.cpp",
	"th04/player_p.cpp",
	"th04/vector2n.asm",
	"th05/spark_a.asm",
	"th05/bullet_p.cpp",
	"th04/grcg_3.cpp",
	"th05/player_a.cpp",
	"th05/bullet_1.asm",
	"th05/bullet_c.cpp",
	"th05/bullet.asm",
	"th05/main/bullet/add_far.cpp",
	"th05/main/bullet/tune.asm",
	"th05/bullet_t.cpp",
	"th03/vector.cpp",
	"th03/hfliplut.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_mode.cpp",
	"th04/cdg_p_na.cpp",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th05/cdg_put.asm",
	"th04/exit.cpp",
	"th05/vector.cpp",
	"th05/snd_load.cpp",
	"th05/snd_kaja.cpp",
	"th05/initmain.cpp",
	"th05/input_s.asm",
	"th05/inp_h_w.cpp",
	"th05/frmdelay.cpp",
	"th04/cdg_load.asm",
	"th04/scrolly3.cpp",
	"th04/motion_3.asm",
	"th05/main031.cpp",
	"th05/enemy_u.cpp",
	"th05/gather.cpp",
	"th05/main032.cpp",
	"th05/itmadd.cpp",
	"th05/main033.cpp",
	"th05/std_run.cpp",
	"th05/enm_btpl.cpp",
	"th05/midboss.cpp",
	"th04/hud_hp.cpp",
	"th05/mb_dft.cpp",
	"th05/mb_dfr.cpp",
	"th05/laser.cpp",
	"th05/cheeto_u.cpp",
	"th04/it_spl_u.cpp",
	"th05/bullet_u.cpp",
	"th05/midboss1.cpp",
	"th05/boss_1.cpp",
	"th05/midboss2.cpp",
	"th05/boss_4.cpp",
	-- BEFORE th05/b4mai.cpp, and that order is load-bearing: b4pair.cpp is
	-- its own object only because @mai_yuki_update$qv takes its `-a2` pad on
	-- the opposite parity from the three jump tables inside b4mai.obj, and
	-- TLINK lays a segment out in link order (kb/codegen 0112 + 0114).
	"th05/b4pair.cpp",
	"th05/b4mai.cpp",
	"th05/swords.cpp",
	"th05/main035.cpp",
	-- Append-anywhere, and parked next to main_036.cpp only because it is the
	-- head of what used to be one segment with it: exalice.cpp is BX_TEXT's
	-- ONLY C++ contribution, so its position in this list cannot reorder
	-- anything. The segment's own place in group main_03 comes from
	-- th05_main.asm, which defines it (and main_036_TEXT behind it) and is the
	-- first object linked.
	"th05/exalice.cpp",
	-- Append-anywhere: main_036_TEXT has no other C++ contribution, so
	-- TLINK puts this object at that segment's tail by construction.
	"th05/main_036.cpp",
	"th05/boss_6.cpp",
	"th05/boss_x.cpp",
	"th05/hud_num.asm",
	"th05/boss.cpp",
	"th05/main014.cpp",

	-- Production oracle facade. The full validation oracle is linked only into
	-- bin/th05/oracle/main.exe below.
	"th05/orl_rel.cpp",
	-- USER REPLAY MOD: production code in an isolated REPLAY_TEXT segment.
	-- Keep mod-only segments at the tail of the link list.
	"th05/replay.cpp",
	-- PORTABLE CHECKPOINT MOD: field codecs in an isolated tail segment.
	"th05/rp_ckpt.cpp",
	-- SAVESTATE GUARD MOD: physical FAT verification in an isolated tail.
	"th05/main/rp_guard.cpp",
}
th05:branch(MODEL_LARGE, { cflags = "-DBINARY='M'" }):link(
	"main", th05_main_inputs
)

local th05_oracle_inputs = {}
for i, input in ipairs(th05_main_inputs) do
	th05_oracle_inputs[i] = (
		(input == "th05/orl_rel.cpp") and "th05/oracle.cpp" or input
	)
end
th05:branch(MODEL_LARGE, Subdir("oracle/"), {
	cflags = "-DBINARY='M'",
}):link("main", th05_oracle_inputs)
th05:branch(MODEL_LARGE, { cflags = "-DBINARY='E'" }):link("maine", {
	"th05/maine_e.cpp",
	{ "th05_maine_master.asm", o = "mainem.obj" },
	"th05/score_d.cpp",
	"th05/score_e.cpp",
	"th05/hi_end.cpp",
	"th03/hfliplut.cpp",
	"th04/snd_pmdr.c",
	"th04/snd_mmdr.c",
	"th04/snd_mode.cpp",
	"th04/grppsafx.asm",
	"th04/cdg_p_na.cpp",
	"th02/snd_se_r.cpp",
	"th04/snd_se.cpp",
	"th04/bgimage.cpp",
	"th04/exit.cpp",
	"th05/vector.cpp",
	"th05/bgimager.asm",
	"th05/snd_load.cpp",
	"th05/snd_kaja.cpp",
	"th05/pi_cpp_1.cpp",
	"th05/pi_asm_1.asm",
	"th05/pi_cpp_2.cpp",
	"th05/pi_asm_2.asm",
	"th05/initmain.cpp",
	"th05/input_s.asm",
	"th05/inp_h_w.cpp",
	"th05/snd_dlym.cpp",
	"th05/frmdelay.cpp",
	"th04/cdg_load.asm",
	"th05/egcrect.cpp",
	"th05/cutscene.cpp",
	"th05/allcast.cpp",
	"th05/regist.cpp",
	-- POSITION-CRITICAL: th05/space.cpp, th05/verd_bmp.cpp and
	-- th05/staffrol.cpp all contribute to MAINE_01__TEXT, and TLINK
	-- concatenates a segment's contributions in link order. space.cpp holds
	-- the head of the dump's block and has to stay immediately before
	-- th05_maine.asm; verd_bmp.cpp holds what th05/end/verdict_bitmap.asm
	-- used to contribute and has to stay immediately after it; staffrol.cpp
	-- holds the tail and has to stay after that. th05_maine.asm itself now
	-- contributes ZERO bytes here and sits between them only to keep that
	-- order readable. Reordering any of the four moves every body below it.
	"th05/space.cpp",
	"th05_maine.asm",
	"th05/verd_bmp.cpp",
	"th05/staffrol.cpp",
	"th05/staff.cpp",
	-- LANGUAGE OVERLAY MOD: optional presentation assets in a trailing segment.
	"th05/rpyend.cpp",
})
-- ----

-- Research
-- --------

local research_cfg = optimized_cfg:branch(Subdir("Research/"), MODEL_SMALL)

research_cfg:link("holdkey", {
	"Research/holdkey.c",
	"bin/masters.lib",
})

research_cfg:branch({ cflags = "-DCPU=386" }):link("pi_bench", {
	"Research/pi_bench.cpp",
	research_cfg:build_uncached("platform/x86real/noexcept.cpp"),
	research_cfg:build_uncached("platform/x86real/pc98/blitter.cpp"),
	research_cfg:build_uncached("platform/x86real/pc98/grp_clip.cpp"),
	"platform/x86real/pc98/graph.cpp",
	"bin/masters.lib",
	piloadm,
})

local research_sprites = Sprites({
	{ "Research/blitperf/blitperf.bmp", "cpp", "sBLITPERF", 16, 16 },
	{ "Research/blitperf/wide.bmp", "c", "sWIDE", 640, 40 },
})

-- Must be an ordered table to retain the order for `build_dumb.bat`.
for _, t in pairs({ { 86, " -1-" }, { 286, " -2" }, { 386, "" } }) do
	local cpu_str = string.format("%03d", t[1])
	local cfg = research_cfg:branch({
		obj_root = (cpu_str .. "/"),
		cflags = string.format("-DCPU=%d%s", t[1], t[2]),
	})
	-- Bypass `PreviousOutputForSource` by explicitly building each unit.
	local obj = {
		cfg:build_uncached({ "Research/blitperf/blitperf.cpp", extra_inputs = {
			research_sprites["blitperf"],
		} }),
		cfg:build_uncached("platform/x86real/noexcept.cpp"),
		cfg:build_uncached("platform/x86real/pc98/blitter.cpp"),
		cfg:build_uncached("platform/x86real/pc98/font.cpp"),
		cfg:build_uncached("platform/x86real/pc98/graph.cpp"),
		cfg:build_uncached("platform/x86real/pc98/grcg.cpp"),
		cfg:build_uncached("platform/x86real/pc98/grp_clip.cpp"),
		cfg:build_uncached("platform/x86real/pc98/palette.cpp"),
		cfg:build_uncached("platform/x86real/pc98/vsync.cpp"),
	}

	local if_shift = {
		cfg:build_uncached({ "Research/blitperf/if_shift.cpp", extra_inputs = {
			th01_sprites["pellet"],
		} }),
	}
	cfg:link(("ifshf" .. cpu_str), tup_append_assignment(if_shift, obj))

	local wide = {
		cfg:build_uncached({ "Research/blitperf/wide.cpp", extra_inputs = {
			research_sprites["wide"],
		} }),
		cfg:build_uncached({ "Research/blitperf/wide_b.cpp" }),
	}
	cfg:link(("wide" .. cpu_str), tup_append_assignment(wide, obj))

	local masked = {
		piloadm,
		cfg:build_uncached({ "Research/blitperf/xfade.cpp"}),
		cfg:build_uncached({ "Research/blitperf/xfade_b.cpp"}),
		cfg:build_uncached({ "platform/x86real/pc98/egc.cpp"}),
		cfg:build_uncached({ "platform/x86real/pc98/grp_surf.cpp"}),
		"bin/masters.lib",
	}
	cfg:link(("xfade" .. cpu_str), tup_append_assignment(masked, obj))
end
-- --------

Rules:print()
