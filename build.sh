
#!/bin/bash

# Exit on error
set -e

# Configuration
BUILD_TYPE=${1:-Debug}
BUILD_DIR="build"
INSTALL_DIR="install"
NUM_CORES=$(nproc)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print with color
print_color() {
    color=$1
    message=$2
    echo -e "${color}${message}${NC}"
}

# Print section header
print_section() {
    echo ""
    print_color $YELLOW "=== $1 ==="
    echo ""
}

# Check for required tools
check_requirements() {
    print_section "Checking requirements"
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_color $RED "CMake is not installed. Please install CMake."
        exit 1
    fi
    
    # Check for C++ compiler
    if ! command -v g++ &> /dev/null; then
        print_color $RED "G++ is not installed. Please install G++."
        exit 1
    fi
    
    print_color $GREEN "All requirements satisfied"
}

# Clean build directory
clean_build() {
    print_section "Cleaning build directory"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_color $GREEN "Build directory cleaned"
    fi
    
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        print_color $GREEN "Install directory cleaned"
    fi
}

# Configure project with CMake
configure_project() {
    print_section "Configuring project"
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR" \
        -DBUILD_TESTS=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cd ..
    
    print_color $GREEN "Project configured successfully"
}

# Build project
build_project() {
    print_section "Building project"
    
    cd "$BUILD_DIR"
    cmake --build . --config $BUILD_TYPE -j$NUM_CORES
    cd ..
    
    print_color $GREEN "Project built successfully"
}

# Run tests
run_tests() {
    print_section "Running tests"
    
    cd "$BUILD_DIR"
    ctest --output-on-failure
    cd ..
    
    print_color $GREEN "All tests completed successfully"
}

# Install project
install_project() {
    print_section "Installing project"
    
    cd "$BUILD_DIR"
    cmake --install .
    cd ..
    
    print_color $GREEN "Project installed successfully"
}

# Main build process
main() {
    print_section "Starting build process"
    print_color $YELLOW "Build type: $BUILD_TYPE"
    
    check_requirements
    clean_build
    configure_project
    build_project
    run_tests
    install_project
    
    print_section "Build process completed"
    print_color $GREEN "Project built and tested successfully"
}

# Execute main function
main
