
project_name := "libnv" 
project_short_desc := "nvllmnd's plain C23 std lib"
project_full_name := "{{project_name}} :: {{project_short_desc}}"
build_root := "./bin"

bin_executable_name := "libnv"

# Build directories
debug_dir := build_root + "/debug"
release_dir := build_root + "/release" 

# Compiler selection
compiler := "clang"


# Build type specific configurations
debug_flags := "--buildtype=debug"

release_flags := "--buildtype='release' -Db_lto=true -Db_lto_threads=4 -Db_ndebug=true" 

version_major := "1"
version_minor := "10"
version_patch := `echo "$(git rev-list --count HEAD)"`
version_full :=  version_major + "." + version_minor + "." + version_patch 


# Lists Available Commands
@default: 
	echo "{{project_name}}. Version: {{version_full}}" 
	just --list


# Force a sync of this projects version number.
@sync-version:
	rm .version; \
	echo {{version_full}} >> .version; \
	echo "{{project_name}} version synced to v{{version_full}}!";

# Creates a symbolic link to debug build output compile_commands.json for IDE intellisense
@compile-commands-debug:
	ln -sf "{{debug_dir}}/compile_commands.json" .


# Creates a symbolic link to debug build output compile_commands.json for IDE intellisense
@compile-commands-release:
	ln -sf "{{release_dir}}/compile_commands.json" .


# Prints project project version
@version:
	echo {{version_full}}


# Ensures our constant keyword hash table is generated
# @gen-keyword-hashes:
# 	bash "./build/gperf_run"


# Download meson wrap subproject dependencies so that they
# can be compiled statically into resulting binaries, instead of dynamically linking
# against shared system libraries.
@download-deps:
	if [ -d "subprojects" ]; then \
		echo "subprojects directory found in root, checking for packagecache..."; \
		if [ ! -d "subprojects/packagecache" ]; then \
			echo "Downloading {{project_name}} depenedencies..."; \
			meson subprojects download; \
		else \
			echo "{{project_name}} dependencies already downloaded!"; \
		fi \
	else \
		echo "{{project_name}} has no meson wrap dependencies to download"; \
	fi

# Create debug build dir + configure"
@setup-debug: sync-version 
	if [ ! -d {{debug_dir}} ]; then \
		echo "Setting up DEBUG build..."; \
		mkdir -p {{debug_dir}}; \
		CC={{compiler}} meson setup {{debug_dir}} {{debug_flags}};  \
	else \
		echo "Debug Build directory already exists!"; \
		exit 0; \
	fi


@setup-release: sync-version 
	if [ ! -d {{release_dir}} ]; then \
		echo "Setting up RELEASE build..."; \
		mkdir -p {{release_dir}}; \
		CC={{compiler}} meson setup {{release_dir}} {{release_flags}};  \
	else \
		echo "Release Build directory already exists!"; \
		exit 0; \
	fi
	

# #Create release build dir + configure"
# @setup-release: sync-version gen-keyword-hashes
# 	if [ ! -d "{{release_dir}}" ]; then \
# 		echo "Setting up RELASE build..."; \
# 		mkdir -p "{{release_dir}}"; \
# 		CC="{{compiler}}" meson setup "{{release_dir}}" "{{release_flags}}"; \ 
# 	else \
# 		echo "Release Build directory already exists!"; \
# 		exit 0; \
# 	fi

# Reconfigure existing builds
@reconfig-debug:
	test -d {{debug_dir}} && meson configure {{debug_dir}} {{debug_flags}} || just setup-debug

# Reconfigure existing builds
@reconfig-release:
	test -d {{release_dir}} && meson configure {{release_dir}} {{release_flags}} || just setup-release

#Compile debug build"
@build-debug: setup-debug
	meson compile -C {{debug_dir}}; \
	just compile-commands-debug

#Compile optimized release build
@build-release: setup-release
	meson compile -C {{release_dir}}; \
	just compile-commands-release

#Run project (debug ) executable"
@run-debug: build-debug
	"{{debug_dir}}/{{bin_executable_name}}"

#Run optimized (build-release) project executable"
@run-release: build-release
	"{{release_dir}}/{{bin_executable_name}}"


# Run meson tests in debug mode
# You can pass 'v', 'verbose' or 'interactive' as an argument to this rule for
# meson to run tests with the '--interactive' flag
@test-debug arg='none': build-debug
	if [ {{arg}} = "verbose" ] || [ {{arg}} = "v" ] || [ {{arg}} = "interactive" ]; then \
		meson test -C {{debug_dir}} --interactive --print-errorlogs; \
	else \
		meson test -C {{debug_dir}}; \
	fi


# run meson tests in release mode
@test-release arg='none': build-release
	if [ {{arg}} = "verbose" ] || [ {{arg}} = "v" ] || [ {{arg}} = "interactive" ]; then \
		meson test -C {{release_dir}} --interactive --print-errorlogs; \
	else \
		meson test -C {{release_dir}}; \
	fi


# Clean both debug and release directories 
@clean:
	rm -rf {{build_root}}

# Re-build Project in debug. Runs clean then build
@rebuild-debug: clean build-debug

# Re-build Project in release. Runs clean then build
@rebuild: clean build-release


# Install release build to your systems standard directory
@install: build-release
	meson install -C {{release_dir}}


# Shortcuts / Aliases
alias br := build-release
alias bd := build-debug
alias sd := setup-debug
alias sr := setup-release
alias cfgd := reconfig-debug
alias cfg := reconfig-release

alias rd := rebuild-debug
alias r := rebuild

alias build := build-debug
alias run := run-debug 

alias test := test-debug
alias td := test-debug
alias tr := test-release


# Prints/Shows all current aliases defined for this justfile
@alias:
	echo "########################"
	echo "#   Justfile Aliases   #"
	echo "########################"
	echo "rd   := rebuild-debug"; \
	echo "r    := rebuild"; \
	echo "run  := run-debug"; \
	echo "br   := build-release"; \
	echo "bd   := build-release"; \
	echo "sd   := setup-debug"; \
	echo "sr   := setup-release"; \
	echo "cfgd := reconfig-debug"; \
	echo "cfg  := reconfig-release"; 
	echo "build := build-debug"; \
	echo "test  := test-debug"; \
	echo "td    := test-debug";\
	echo "tr    := test-release";
