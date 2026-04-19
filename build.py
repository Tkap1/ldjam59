
import time
import sys
import os
import subprocess
import shutil

g_time = []

e_debug_mode_debug = 0
e_debug_mode_optimized_debug = 1
e_debug_mode_optimized = 2

def main():
	start_time()

	open("compiler_output.txt", "w").close()

	if len(sys.argv) != len(set(sys.argv)):
		error("Duplicate arguments")

	os.makedirs("build", exist_ok=True)
	run_no_wait("generate_ctags.bat", output=False)
	run_and_wait("build/metaprogram.exe -Dm_debug", False)

	is_game_running = is_running("main.exe")

	debug_mode = e_debug_mode_debug
	compile_full = debug_mode == e_debug_mode_optimized
	compile_game_dll = not compile_full
	compile_platform = not is_game_running and not compile_full

	compiler_args = get_common_compiler_args(debug_mode)
	linker_args = get_common_linker_args(debug_mode)
	if compile_game_dll:
		start_time("Compiling game dll...")
		success = compile_dll(["../src/game.cpp"], "main", compiler_args, linker_args)
		if success:
			shutil.copyfile("build/main.dll", "main.dll")
			play_success_sound()
		else:
			play_fail_sound()
			compile_platform = False
		end_time("Compile game dll:")

	if compile_platform:
		start_time("Compiling platform...")
		success = compile_exe(["../src/platform_win32.cpp"], "main", compiler_args, linker_args)
		if success:
			shutil.copyfile("build/main.exe", "main.exe")
		else:
			play_fail_sound()
		end_time("Compile platform:")

	if compile_full:
		start_time("Compiling platform + game...")
		success = compile_exe(["../src/platform_win32.cpp", "../src/game.cpp"], "main", compiler_args, linker_args)
		if success:
			shutil.copyfile("build/main.exe", "main.exe")
		else:
			play_fail_sound()
		end_time("Full build:")

	end_time("Total time:")
#------------------------------------------------------------------------------------------------------------------------------------

def is_running(name):
	for line in subprocess.check_output(["tasklist"]).decode().splitlines():
		if name.lower() in line.lower():
			return True
	return False
#------------------------------------------------------------------------------------------------------------------------------------

def compile(src_files, output_name, pdb_name, compiler_args, linker_args):
	cmd = "pushd build && "
	cmd += "cl.exe "
	for f in src_files:
		cmd += f"{f} "
	for arg in compiler_args:
		cmd += f"{arg} "
	cmd += "-link "
	for arg in linker_args:
		cmd += f"{arg} "
	cmd += f"-PDB:{pdb_name}.pdb "
	cmd += "> temp_compiler_output.txt && popd"
	result = os.system(cmd)
	os.system("type build\\temp_compiler_output.txt")
	# print(cmd)
	success = True if result == 0 else False
	shutil.copyfile("build/temp_compiler_output.txt", "compiler_output.txt")
	return success
#------------------------------------------------------------------------------------------------------------------------------------

def compile_exe(src_files, output_name, in_compiler_args, linker_args):
	compiler_args = in_compiler_args.copy()
	compiler_args.append(f"-Fe{output_name}.exe")
	result = compile(src_files, output_name, f"{output_name}_exe", compiler_args, linker_args)
	return result
#------------------------------------------------------------------------------------------------------------------------------------

def compile_dll(src_files, output_name, in_compiler_args, linker_args):
	compiler_args = in_compiler_args.copy()
	compiler_args.append("-LD")
	compiler_args.append(f"-Fe{output_name}.dll")
	result = compile(src_files, output_name, f"{output_name}_dll", compiler_args, linker_args)
	return result
#------------------------------------------------------------------------------------------------------------------------------------

def get_common_compiler_args(debug_mode):
	result = [
		"-nologo",
		"-std:c++20",
		"-Zc:strictStrings-",
		"-Wall",
		"-FC",
		"-Gm-",
		"-GR-",
		"-EHa-",
		"-ID:/Dev/C/sdl",
		"-ID:/Dev/C/sdl/SDL2",
		"-I../../my_libs2",
		"-I../src/my_libs2",

		"-D_CRT_SECURE_NO_WARNINGS",
		"-wd4201", # nonstandard extension used: nameless struct/union
		"-wd4820", # '3' bytes padding added after data member 's_str_builder<256>::str'
		"-wd5219", # implicit conversion from 'int' to 'float', possible loss of data
		"-wd4365", # conversion from 'int' to 'size_t', signed/unsigned mismatch
		"-wd5045", # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
		"-wd4061", # enumerator 'e_place_result_occupied' in switch of enum 'e_place_result' is not explicitly handled by a case label
		"-wd4505", # unreferenced function with internal linkage has been removed
		"-wd5246", # 'anonymous struct or union': the initialization of a subobject should be wrapped in braces
		"-wd4711", # function 'double __cdecl at_most<double>(double,double)' selected for automatic inline expansion
		"-wd4710", # warning C4710: 'printf': function not inlined
		"-wd4191", # warning C4191: 'type cast': unsafe conversion from 'FARPROC' to 't_game_func'
		# "-wd4623",
		"-wd4062", # enumerator 'e_entity_player' in switch of enum 'e_entity' is not handled

		# "-fsanitize=address",
	]
	if debug_mode == e_debug_mode_debug:
		result.append("-Zi")
		result.append("-MTd")
		result.append("-Od")
		result.append("-Dm_debug")
	elif debug_mode == e_debug_mode_optimized_debug:
		result.append("-Zi")
		result.append("-MTd")
		result.append("-O2")
		result.append("-Dm_debug")
	elif debug_mode == e_debug_mode_optimized:
		result.append("-MT")
		result.append("-O2")
	return result
#------------------------------------------------------------------------------------------------------------------------------------

def get_common_linker_args(debug_mode):
	result = [
		"-INCREMENTAL:NO",
		"../SDL2.lib",
		"Winmm.lib",
		"User32.lib",
		"Gdi32.lib",
		"Shell32.lib",
		"Setupapi.lib",
		"Version.lib",
		"Ole32.lib",
		"Imm32.lib",
		"Advapi32.lib",
		"OleAut32.lib",
		"-DYNAMICBASE:NO",
		# "-IGNORE:4099",
		# "../SDL2_mixer.lib",
	]
	if debug_mode == e_debug_mode_debug:
		result.append("-DEBUG:FULL");
	elif debug_mode == e_debug_mode_optimized_debug:
		result.append("-DEBUG:FULL");
	elif debug_mode == e_debug_mode_optimized:
		result.append("-DEBUG:NONE");
	return result
#------------------------------------------------------------------------------------------------------------------------------------

def play_fail_sound():
	run_no_wait("cmdmp3 build_fail.mp3", output=False);
#------------------------------------------------------------------------------------------------------------------------------------

def play_success_sound():
	run_no_wait("cmdmp3 build_success.mp3", output=False);
#------------------------------------------------------------------------------------------------------------------------------------

def start_time(text = None):
	if text:
		print(text)
	g_time.append(time.time())
#------------------------------------------------------------------------------------------------------------------------------------

def end_time(text):
	before = g_time.pop()
	now = time.time()
	passed = (now - before)
	print(f"{text} {passed} seconds")
#------------------------------------------------------------------------------------------------------------------------------------

def run_and_wait(cmd, print_output):
	if print_output:
		result = subprocess.check_output(cmd, text=True)
		print(result)
	else:
		subprocess.check_output(cmd)
#------------------------------------------------------------------------------------------------------------------------------------

def run_no_wait(cmd, output=True):
	if output:
		subprocess.Popen(cmd)
	else:
		subprocess.Popen(cmd, stdout=subprocess.DEVNULL)
#------------------------------------------------------------------------------------------------------------------------------------

if __name__ == "__main__":
	main()
#------------------------------------------------------------------------------------------------------------------------------------