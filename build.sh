#!/bin/bash
# Kernel build script with Clang for arm64

set -e

# Environment Setup
export KERNEL_DIR=$(pwd)
export ARCH=arm64
export DEFCONFIG=vendor/realme-trinket_defconfig
export CROSS_COMPILE="aarch64-linux-gnu-" # Used for binutils and some scripts

# KBUILD_COMPILER_STRING is usually set by the kernel build system when LLVM=1
# For a pre-check, we can define it here.
export KBUILD_COMPILER_STRING=$(clang --version | head -n 1)

# Output directory for the build, made architecture specific
export KBUILD_OUTPUT=${KERNEL_DIR}/out/${ARCH}

# Toolchain Check (simplified)
echo "Performing toolchain checks..."
# On Fedora, ensure Clang, LLD, and AArch64 GNU toolchain components are installed:
# sudo dnf install clang lld binutils-aarch64-linux-gnu gcc-aarch64-linux-gnu
#
if ! command -v clang &> /dev/null; then
    echo "Clang could not be found. Please ensure Clang is installed and in your PATH."
    exit 1
fi
if ! command -v ld.lld &> /dev/null; then
    echo "ld.lld could not be found. Please ensure LLD is installed (usually part of LLVM) and in your PATH."
    exit 1
fi
# Check for the cross-compiler GCC, as it's still needed for some host tools or scripts
if ! command -v ${CROSS_COMPILE}gcc &> /dev/null; then
    echo "${CROSS_COMPILE}gcc (GCC for ${ARCH}) could not be found. Please ensure binutils and GCC for ${ARCH} are installed."
    exit 1
fi
echo "Using Clang: ${KBUILD_COMPILER_STRING}"

# Dependency Check
echo "Checking for general build dependencies..."
# On Fedora, you might need packages like:
# sudo dnf install make gcc elfutils-libelf-devel bison flex openssl-devel ncurses-devel rsync bc perl dkms
MISSING_DEPS=()
# DEPS lists executables that should be in PATH. Devel packages provide headers and are mentioned in the install hint.
DEPS=("make" "gcc" "bison" "flex" "openssl" "rsync" "bc" "perl") # "dkms" is more of an optional runtime thing, but can be a build dep for some external modules

for dep in "${DEPS[@]}"; do
    if ! command -v "$dep" &> /dev/null; then
        MISSING_DEPS+=("$dep")
    fi
done

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "Error: The following general executable dependencies are missing:"
    for dep in "${MISSING_DEPS[@]}"; do
        echo "  - $dep"
    done
    echo "Please install them. Also ensure corresponding -devel packages for libraries are installed (e.g., elfutils-libelf-devel, openssl-devel, ncurses-devel)."
    echo "Example for Fedora: sudo dnf install make gcc elfutils-libelf-devel bison flex openssl-devel ncurses-devel rsync bc perl"
    exit 1
else
    echo "All essential executable dependencies found. Ensure -devel packages for libraries are also installed."
fi

# Argument Parsing
CLEAN_BUILD=0
NUM_JOBS=$(nproc) # Default to all available processors

while getopts "cj:" opt; do
  case $opt in
    c)
      CLEAN_BUILD=1
      ;;
    j)
      # Validate that OPTARG is a number
      if ! [[ "$OPTARG" =~ ^[0-9]+$ ]]; then
        echo "Error: Number of jobs (-j) must be an integer." >&2
        exit 1
      fi
      NUM_JOBS=$OPTARG
      ;;
    \?)
      echo "Usage: $0 [-c] [-j NUM_JOBS]" >&2
      echo "  -c: Perform a clean build (make mrproper)" >&2
      echo "  -j NUM_JOBS: Number of parallel jobs for make (default: $(nproc))" >&2
      exit 1
      ;;
  esac
done

echo "Number of jobs for make: $NUM_JOBS"
if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Clean build will be performed."
fi

# Ensure output directory exists
mkdir -p "${KBUILD_OUTPUT}"

# Build Steps
cd "$KERNEL_DIR"

# Clean the kernel tree if requested
if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Cleaning kernel tree (make mrproper)..."
  make O="${KBUILD_OUTPUT}" LLVM=1 ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" mrproper
fi

# Check if DEFCONFIG file exists
# Note: KERNEL_DIR is the root of the kernel source tree.
DEFCONFIG_PATH="${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG}"
if [ ! -f "${DEFCONFIG_PATH}" ]; then
    echo "Error: Defconfig file '${DEFCONFIG}' not found at '${DEFCONFIG_PATH}'"
    echo "Please ensure the KERNEL_DIR is correct and the defconfig exists."
    echo "Available configs in ${KERNEL_DIR}/arch/${ARCH}/configs/:"
    ls "${KERNEL_DIR}/arch/${ARCH}/configs/" || echo "Could not list configs in ${KERNEL_DIR}/arch/${ARCH}/configs/."
    exit 1
fi

echo "Setting defconfig to $DEFCONFIG..."
make O="${KBUILD_OUTPUT}" LLVM=1 ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" "${DEFCONFIG}"

# Compile the kernel
echo "Compiling kernel (make)..."
# Using LLVM=1 tells Kbuild to use LLVM tools (clang, ld.lld, llvm-ar, etc.)
# ARCH and CROSS_COMPILE are standard. O specifies the output directory.
make O="${KBUILD_OUTPUT}" LLVM=1 ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" -j"${NUM_JOBS}"

echo "Kernel compilation finished successfully."
echo "Kernel image, .config, and System.map can be found in ${KBUILD_OUTPUT}/arch/${ARCH}/boot/"
echo "Built modules (if any) can be found under ${KBUILD_OUTPUT}"
echo ""
echo "To install modules (example):"
echo "  sudo make O=\"${KBUILD_OUTPUT}\" LLVM=1 ARCH=\"${ARCH}\" CROSS_COMPILE=\"${CROSS_COMPILE}\" modules_install"
echo "To install kernel image (example, depends on bootloader/setup):"
echo "  sudo cp \"${KBUILD_OUTPUT}/arch/${ARCH}/boot/Image.gz\" /boot/vmlinuz-$(make O=\"${KBUILD_OUTPUT}\" LLVM=1 kernelrelease)" # Or Image, depending on compression

exit 0
