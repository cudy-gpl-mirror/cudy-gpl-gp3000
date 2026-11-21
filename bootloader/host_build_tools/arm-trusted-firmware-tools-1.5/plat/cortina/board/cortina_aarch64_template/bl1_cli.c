/*
 * Copyright (c) 2015-2016, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <debug.h>
#include <io_driver.h>
#include <io_storage.h>
#include <string.h>
#include <mmio.h>
#include <console.h>

#define _GETS_OK        1
#define _GETS_TIMEOUT   2
#define _GETS_CTRLC     3
#define MAX_ARGV        8

#define CTRL(c) ((c)&0x1F)

#define CMD_CONTINUE		90

typedef struct {
    char *cmd;
    char *help_msg;
    int (*func_exe) (int argc, char **argv);
} cmd_t;

static cmd_t command_list[8];
int command_count;

inline int isupper(int c)
{
    return (('A' <= c) && (c <= 'Z'));
}                               /* isupper() */


inline int islower(int c)
{
    return (('a' <= c) && (c <= 'z'));
}                               /* islower() */


inline int isdigit(int c)
{
    return (('0' <= c) && (c <= '9'));
}                               /* isdigit() */

inline int isspace(int c)
{
    return ((c == ' ') || (c == '\f') || (c == '\n') || (c == '\r') ||
            (c == '\t') || (c == '\v'));
}                               /* isspace() */

inline int isalpha(int c)
{
    return (islower(c) || isupper(c));
}                               /* isalpha() */

inline int tolower(int c)
{
    return isupper(c) ? c - 'A' + 'a' : c;
}                               /* tolower() */

unsigned int strtol(const char *nptr)
{
    const char *s = nptr;
    unsigned int acc;
    int c;
    int base = 16;

    //
    // Skip white space and pick up leading +/- sign if any.
    // If base is 0, allow 0x for hex and 0 for octal, else
    // assume decimal; if base is already 16, allow 0x.
    //

    do {
        c = *s++;
    }
    while (isspace(c));

    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    // no overflow check

    for (acc = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        else {
            acc *= base;
            acc += c;
        }
    }
    return acc;
}

#if 0
char cortina_getc(int portnum)
{

    char c = 0x55;
#if 0
#ifdef CONFIG_DESIGNWARE_UART
    unsigned int status;

    /* Wait for Rx FIFO available */
    do {
        status = IO_READ(port[portnum] + LSR);
    }
    while (!(status & 0x00000001));

    /* Receive the character */
    c = (char) IO_READ(port[portnum] + PBR);
#elif CONFIG_CORTINA_UART
    /* Wait until there is data in the FIFO */
    while (IO_READ(port[portnum] + UINFO) & UINFO_RX_FIFO_EMPTY);

    c = IO_READ(port[portnum] + URX_DATA) & 0xFF;
#endif
#endif
    return c;

}
#endif

int cli_gets(char *buf, int buflen, int timeout)
{
    char *ip;
    char *eol;
    char c;
    static char last_ch = '\0';

    *buf = '\0';                // Empty buffer
    ip = buf;                   // Insertion point
    eol = buf;                  // End of line

    ip = eol;

    while (1) {
        c = console_getc();

        *eol = '\0';
        switch (c) {
            case CTRL('C'):    // ^C
                // Abort current input
                printf("^C\n");
                *buf = '\0';    // Nothing useful in buffer
                return _GETS_CTRLC;
            case '\n':
            case '\r':
                // If previous character was the "other" end-of-line, ignore this one
                if (((c == '\n') && (last_ch == '\r')) ||
                    ((c == '\r') && (last_ch == '\n'))) {
                    c = '\0';
                    break;
                }

                printf("\n");
                last_ch = c;
                return _GETS_OK;
            case '\b':
            case 0x7F:         // DEL
                if (ip != buf) {
                    printf("\b");
                    printf(" ");
                    printf("\b");
                    ip--;
                    eol--;

                }
                break;
            default:
                printf("%c", c);
                if (ip == eol) {
                    // Advance both pointers
                    *ip++ = c;
                    eol = ip;
                }
        }

        last_ch = c;
        if (ip == buf + buflen - 1) {
            // Buffer full
            *ip = '\0';
            return buflen;
        }
    }
}

/*
 * Scan through an input line and break it into "arguments".  These
 * are space delimited strings.  Return a structure which points to
 * the strings, similar to a Unix program. Multiple commands in the line
 * are separated by ; similar to sh. If we find a semi we stop processing the
 * line, terminate the current command with a null and return the start
 * of the next command in *line. parse() can then be called again to
 * process the next command on the line.
 * Note: original input is destroyed by replacing the delimiters with
 * null ('\0') characters for ease of use.
*/

void cli_parse_prep(char **line, int *argc, char **argv)
{
    char *cp = *line;
    char *pp;
    int indx = 0;
    int semi = 0;

    while (*cp) {
        /* Skip leading spaces */
        while (*cp && *cp == ' ') {
            cp++;
        }

        if (!*cp) {
            break;              // Line ended with a string of spaces
        }
        if (*cp == ';') {
            *cp = '\0';
            semi = 1;
            break;
        }

        if (indx < MAX_ARGV)
            argv[indx++] = cp;
        else
            printf("Err!\n");

        while (*cp) {
            if (*cp == ' ') {
                *cp++ = '\0';
                break;
            } else if (*cp == ';') {
                break;
            } else if (*cp == '"') {
                // Swallow quote, scan till following one
                if (argv[indx - 1] == cp) {
                    argv[indx - 1] = ++cp;
                }
                pp = cp;
                while (*cp && *cp != '"') {
                    if (*cp == '\\') {
                        // Skip over escape - allows for escaped '"'
                        cp++;
                    }
                    // Move string to swallow escapes
                    *pp++ = *cp++;
                }
                if (!*cp) {
                    printf("Err!\n");
                } else {
                    if (pp != cp)
                        *pp = '\0';
                    *cp++ = '\0';
                    break;
                }
            } else {
                cp++;
            }
        }
    }
    if (semi)
        *line = cp + 1;
    else
        *line = cp;

    *argc = indx;
}

int cli_cmd_exe(int argc, char **argv)
{
    int i,ret;

    for (i = 0; i < command_count; i++) {
        if (strcmp(argv[0], command_list[i].cmd) == 0) {
            ret = command_list[i].func_exe(argc, argv);
			if((ret == CMD_CONTINUE) || (ret == 0))
				return ret;

            if (ret != 0 ) {
                printf("Syntax :\n");
                printf(command_list[i].help_msg);
            }
            return 0;
        }
    }

    printf("Illegal Command!\n");

    return 1;
}

static int do_help(int argc, char **argv)
{
    int i;

    printf("   cmds:\n");

    for (i = 0; i < command_count; i++) {
        printf("%s\n", command_list[i].help_msg);
    }

    return 0;
}

static int do_set_memory(int argc, char **argv)
{
    uintptr_t addr = strtol(argv[1]);
    uint32_t val = strtol(argv[2]);

    *(volatile uint32_t *) addr = val;

    return 0;
}

static int do_dump_memory(int argc, char **argv)
{
    uint32_t val;

    if (argc < 2)
        return 1;

    uintptr_t addr = strtol(argv[1]);
    val = mmio_read_32(addr);
    printf("R 0x%x: 0x%x\n", (unsigned int) addr, val);

    return 0;
}

int do_jump(int argc, char **argv)
{

    uintptr_t addr = strtol(argv[1]);
    printf("Jump to 0x%x\n", (unsigned int) addr);

    ((void (*)(void)) addr) ();

    return 0;
}

int do_loop(int argc, char **argv)
{
	/* Loop here */
    while (1);

    return 0;
}

int do_isb(int argc, char **argv)
{
    __asm__ volatile ("isb":::"memory");
    return 0;
}

int do_dsb(int argc, char **argv)
{
    __asm__ volatile ("dsb sy":::"memory");
    return 0;
}

int do_return(int argc, char **argv)
{
    return CMD_CONTINUE;
}

int init_cmd_list()
{
    int i = 0;

    command_list[i].cmd = "h";
    command_list[i].func_exe = &do_help;
    command_list[i++].help_msg = "\th\t\t- Show help message";

    command_list[i].cmd = "w";
    command_list[i].func_exe = &do_set_memory;
    command_list[i++].help_msg = "\tw addr value\t- write memory";

    command_list[i].cmd = "r";
    command_list[i].func_exe = &do_dump_memory;
    command_list[i++].help_msg = "\tr addr\t- read memory";

    command_list[i].cmd = "j";
    command_list[i].func_exe = &do_jump;
    command_list[i++].help_msg = "\tj addr\t\t- jump to addr";

    command_list[i].cmd = "l";
    command_list[i].func_exe = &do_loop;
    command_list[i++].help_msg = "\tl\t\t- loop";

    command_list[i].cmd = "i";
    command_list[i].func_exe = &do_isb;
    command_list[i++].help_msg = "\ti\t\t- ISB";

    command_list[i].cmd = "d";
    command_list[i].func_exe = &do_dsb;
    command_list[i++].help_msg = "\td\t\t- DSB";

    command_list[i].cmd = "c";
    command_list[i].func_exe = &do_return;
    command_list[i++].help_msg = "\tc\t\t- Continue run";

    printf("Total %d command provided\n", i);

    return i;
}

void bl_cli(void)
{
    char *command;
    char line[64];
    int cli_argc, ret;
    char *cli_argv[MAX_ARGV];

    command_count = init_cmd_list();

    while (1) {
        printf(">");

        memset(line, 0, sizeof(line));
        memset(cli_argv, 0, sizeof(char *) * MAX_ARGV);

        if (_GETS_TIMEOUT != cli_gets(line, sizeof(line), 1000000)) {
            command = (char *) &line;

            while (strlen(command) > 0) {

                printf("do command: %s \n", line);

                cli_parse_prep(&command, &cli_argc, &cli_argv[0]);
                ret = cli_cmd_exe(cli_argc, cli_argv);
				if(ret == CMD_CONTINUE)
					return ;
            }
        }
    }

    return;
}
