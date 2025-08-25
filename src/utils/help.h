#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>

// ANSI color codes
#define BOLD     "\033[1m"
#define RESET    "\033[0m"
#define CYAN     "\033[36m"
#define GREEN    "\033[32m"
#define YELLOW   "\033[33m"
#define BLUE     "\033[34m"
#define MAGENTA  "\033[35m"

static void print_help(void) {
    printf(BOLD CYAN "Catalyze" RESET " - A build system for C :3\n\n");
    
    printf(BOLD "USAGE:" RESET "\n");
    printf("    " BOLD "catalyze" RESET " " GREEN "[COMMAND]" RESET " " YELLOW "[ARGS]" RESET "\n\n");
    
    printf(BOLD "COMMANDS:" RESET "\n");
    
    // init command
    printf("    " BOLD GREEN "init" RESET "\n");
    printf("        Creates a new project in the current directory\n\n");
    
    // new command  
    printf("    " BOLD GREEN "new" RESET " " YELLOW "<project_name>" RESET "\n");
    printf("        Creates a new project with the specified name\n\n");
    
    // build command
    printf("    " BOLD GREEN "build" RESET " " YELLOW "[target]" RESET "\n");
    printf("        Builds the specified target\n");
    printf("        If no target is specified, builds all targets\n\n");
    
    // run command
    printf("    " BOLD GREEN "run" RESET " " YELLOW "[target]" RESET "\n");
    printf("        Runs the specified executable target\n");
    printf("        If no target is specified, runs all executable targets\n\n");
    
    // test command
    printf("    " BOLD GREEN "test" RESET " " YELLOW "[target]" RESET "\n");
    printf("        Builds and runs the specified test target\n");
    printf("        If no target is specified, runs all test targets\n\n");
    
    // debug command
    printf("    " BOLD GREEN "debug" RESET " " YELLOW "[target]" RESET "\n");
    printf("        Builds and runs the specified debug target\n");
    printf("        If no target is specified, runs all debug targets\n\n");
    
    // help command
    printf("    " BOLD GREEN "help" RESET "\n");
    printf("        Display this help message\n\n");
    
    printf(BOLD "EXAMPLES:" RESET "\n");
    printf("    " BOLD "catalyze new" RESET " myproject      " BLUE "# Create new project called 'myproject'" RESET "\n");
    printf("    " BOLD "catalyze init" RESET "               " BLUE "# Initialize project in current directory" RESET "\n");
    printf("    " BOLD "catalyze build" RESET "              " BLUE "# Build all targets" RESET "\n");
    printf("    " BOLD "catalyze build" RESET " release      " BLUE "# Build only the 'release' target" RESET "\n");
    printf("    " BOLD "catalyze run" RESET " myapp          " BLUE "# Run the 'myapp' executable" RESET "\n");
    printf("    " BOLD "catalyze test" RESET "               " BLUE "# Run all tests" RESET "\n");
    printf("    " BOLD "catalyze debug" RESET " myapp        " BLUE "# Build and run 'myapp' in debug mode" RESET "\n\n");
    
    printf("For more information, visit: " BOLD MAGENTA "url goes here silly" RESET "\n");
}

#endif // !define ARGS_H
