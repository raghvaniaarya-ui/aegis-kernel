#include <stdint.h>
#include <stddef.h>
#include "ipc.h"
#include "console.h"
#include "string.h"
#include "types.h"

#define SHELL_MAX_CMD_LEN 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "aegis> "

static char cmd_buffer[SHELL_MAX_CMD_LEN];
static char *argv[SHELL_MAX_ARGS];

static int shell_parse_cmd(char *cmd, char **argv, int max_args) {
    int argc = 0;
    char *p = cmd;
    
    while (*p && argc < max_args - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }
    argv[argc] = NULL;
    return argc;
}

static void shell_exec_builtin(int argc, char **argv) {
    if (argc == 0) return;
    
    if (strcmp(argv[0], "help") == 0) {
        console_write("Available commands:\n");
        console_write("  help       - Show this help\n");
        console_write("  echo       - Print arguments\n");
        console_write("  clear      - Clear screen (not implemented)\n");
        console_write("  reboot     - Reboot system\n");
        return;
    }
    
    if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) {
            console_write(argv[i]);
            if (i < argc - 1) console_write(" ");
        }
        console_write("\n");
        return;
    }
    
    if (strcmp(argv[0], "clear") == 0) {
        console_write("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        return;
    }
    
    if (strcmp(argv[0], "reboot") == 0) {
        console_write("Rebooting...\n");
        __asm__ volatile("outw %w0, %w1" : : "a"(0x8900), "Nd"(0x604));
        return;
    }
    
    console_write("Unknown command: ");
    console_write(argv[0]);
    console_write("\n");
}

void shell_main(void) {
    endpoint_id_t ep = 0;
    int result = ipc_endpoint_create(&ep);
    if (result != 0) {
        console_write("[shell] Failed to create endpoint\n");
        return;
    }
    
    console_write("[shell] Shell server started on endpoint ");
    console_write_dec(ep);
    console_write("\n");
    
    console_write(SHELL_PROMPT);
    
    aegis_msg_t msg;
    while (1) {
        result = ipc_recv(ep, &msg);
        
        if (result == 0 && msg.type == 4) {
            char c = (char)msg.arg0;
            
            if (c == '\n' || c == '\r') {
                console_write("\n");
                cmd_buffer[strlen(cmd_buffer)] = '\0';
                
                if (strlen(cmd_buffer) > 0) {
                    int argc = shell_parse_cmd(cmd_buffer, argv, SHELL_MAX_ARGS);
                    shell_exec_builtin(argc, argv);
                }
                
                memset(cmd_buffer, 0, SHELL_MAX_CMD_LEN);
                console_write(SHELL_PROMPT);
            } else if (c == '\b' || c == 127) {
                int len = strlen(cmd_buffer);
                if (len > 0) {
                    cmd_buffer[len - 1] = '\0';
                    console_write("\b \b");
                }
            } else if (c >= 32 && c < 127) {
                int len = strlen(cmd_buffer);
                if (len < SHELL_MAX_CMD_LEN - 1) {
                    cmd_buffer[len] = c;
                    cmd_buffer[len + 1] = '\0';
                    console_write((char[]){c, '\0'});
                }
            }
        }
    }
}