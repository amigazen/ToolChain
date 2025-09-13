
/*
 * amiga2sun.c - Converts Metacompco format assembly to Sun format
 */

#include "/usr/include/stdio.h"

#define isspace(x)  ((x) == '\n' || (x) == ' ' || (x) == '\t' || (x) == '\f')

char            argbuf[1024];
char            linebuf[1024];
char           *label = NULL, *opcode = NULL, *arg1 = NULL, *arg2 = NULL;
int             nextlabel = 0;

struct trans {
    char           *input;
    char           *output;
};

struct trans    table[] = {
    {"DCw", ".word"},
    {"DCl", ".long"},
    {"DCb", ".byte"},
    {"DSb", ".=.+"},
    {"CODE", ".text"},
    {"CNOP", ".even"},
    {"EQU", "="},
    {"XDEF", ".globl"},
    {"STABN", ".stabn"},
    {"STABS", ".stabs"},
    {"END", (char *) NULL},
    {"bra", "jra"},
    {"move", "movl"},
    {"movel", "movl"},
    {"movew", "movw"},
    {"moveb", "movb"},
    {"add", "addl"},
    {"sub", "subl"},
    {"neg", "negl"},
    {"asl", "asll"},
    {"and", "andl"},
    {(char *) NULL, (char *) NULL}
};

char           *
getline()
{
    char           *ptr;

    ptr = fgets(linebuf, 1024, stdin);
    return (ptr);
}

void
changeopcode()
{
    struct trans   *start = table;
    int             len;

    if (opcode != NULL) {
        len = strlen(opcode) - 1;
        if (len > 1 && opcode[len - 1] == '.') {
            opcode[len - 1] = opcode[len];
            opcode[len] = '\0';
        }

        while (start->input != NULL) {
            if (strcmp(opcode, start->input) == 0) {
                opcode = start->output;
                return;
            }
            start++;
        }
    }
}

void
changearg(arg)
    char          **arg;
{
    char           *ptr = *arg;
    int             offset, reg, reg2, num;
    char            ch;

    if (ptr != NULL) {
        switch (*ptr) {
        case '#':
            break;
        case '_':
            if (ptr[2] == '%')
                ptr[2] = '$';
            if (strcmp(ptr, "_main") == 0) {
                *arg = "_PDC_main";
                return;
            }
            break;
        case '(':
            num = sscanf(ptr, "(A%d)%c", &reg, &ch);
            if (num == 1) {
                sprintf(argbuf, "a%d@", reg);
                *arg = argbuf;
                return;
            }
            else if (num == 2 && ch == '+') {
                sprintf(argbuf, "a%d@+", reg);
                *arg = argbuf;
                return;
            }
            break;
        case 'A':
            *ptr++ = 'a';
            if (*ptr == '7')
                *arg = "sp";
            return;
            break;
        case 'D':
            *ptr++ = 'd';
            return;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            num = sscanf(ptr, "-(A%d)", &reg);
            if (num == 1) {
                sprintf(argbuf, "a%d@-", reg);
                *arg = argbuf;
                return;
            }
            num = sscanf(ptr, "%d(A%d,D%d.l)", &offset, &reg, &reg2);
            if (num == 3) {
                sprintf(argbuf, "a%d@(%d,d%d:l)", reg, offset, reg2);
                *arg = argbuf;
                return;
            }
            num = sscanf(ptr, "%d(A%d,A%d.l)", &offset, &reg, &reg2);
            if (num == 3) {
                sprintf(argbuf, "a%d@(%d,a%d:l)", reg, offset, reg2);
                *arg = argbuf;
                return;
            }
            num = sscanf(ptr, "%d(A%d)", &offset, &reg);
            if (num == 2) {
                sprintf(argbuf, "a%d@(%d)", reg, offset);
                *arg = argbuf;
                return;
            }
            if (strcmp("0x24,0,0,_main", ptr) == 0)
                *arg = "0x24,0,0,_PDC_main";
            break;
        default:
            if (strcmp("\"main:F(0,1)\"", ptr) == 0)
                *arg = "\"PDC_main:F(0,1)\"";
            break;
        }
    }
}

void
lex()
{
    char           *ptr = linebuf;
    int             paren = 0;

    label = opcode = arg1 = arg2 = NULL;

    if (*ptr == ';' || *ptr == '*')
        return;

    if (*ptr && !isspace(*ptr)) {
        label = ptr;
        while (*ptr && !isspace(*ptr))
            ptr++;
        *ptr++ = '\0';
    }

    while (*ptr && isspace(*ptr))
        ptr++;

    if (*ptr && !isspace(*ptr)) {
        opcode = ptr;
        while (*ptr && !isspace(*ptr))
            ptr++;
        *ptr++ = '\0';
    }

    while (*ptr && isspace(*ptr))
        ptr++;

    if (*ptr && !isspace(*ptr)) {
        arg1 = ptr;
        paren = 0;
        while (*ptr && !isspace(*ptr) && *ptr != '\n') {
            if (*ptr == '(')
                paren++;
            if (*ptr == ')')
                paren--;
            if (*ptr == ',' && paren == 0)
                break;
            ptr++;
        }
        if (*ptr && *ptr == ',') {
            *ptr++ = '\0';
            arg2 = ptr;
            while (*ptr && !isspace(*ptr) && *ptr != '\n')
                ptr++;
        }
        *ptr = '\0';
    }
}

int
genmask(ptr, dir)
    char           *ptr;
    int             dir;
{
    int             reg, mask;
    int             start, stop;

    mask = 0;
    start = stop = -1;

    while (*ptr) {
        switch (*ptr) {
        case 'D':
            ptr++;
            stop = *ptr - '0';
            if (start == -1)
                start = stop;
            else {
                for (reg = start; reg <= stop; reg++)
                    if (dir)
                        mask |= 1 << (15 - reg);
                    else
                        mask |= 1 << reg;
                start = stop = -1;
            }
            break;
        case 'A':
            ptr++;
            stop = *ptr - '0' + 8;
            if (start == -1)
                start = stop;
            else {
                for (reg = start; reg <= stop; reg++)
                    if (dir)
                        mask |= 1 << (15 - reg);
                    else
                        mask |= 1 << reg;
                start = stop = -1;
            }
            break;
        case '-':
            break;
        case '/':
            if (start != -1) {
                for (reg = start; reg <= stop; reg++)
                    if (dir)
                        mask |= 1 << (15 - reg);
                    else
                        mask |= 1 << reg;
                start = stop = -1;
            }
            break;
        }
        ptr++;
    }
    if (start != -1) {
        for (reg = start; reg <= stop; reg++)
            if (dir)
                mask |= 1 << (15 - reg);
            else
                mask |= 1 << reg;
    }
    return (mask);
}

void
domask()
{
    int             mask;

    fputs(opcode, stdout);
    fputs("\t", stdout);
    if (*arg2 == '-') { /* Push onto Stack */
        mask = genmask(arg1, 1);
        sprintf(argbuf, "#%d", mask);
        fputs(argbuf, stdout);
        fputs(",", stdout);
        changearg(&arg2);
        fputs(arg2, stdout);
    }
    else {          /* Pull from stack */
        changearg(&arg1);
        fputs(arg1, stdout);
        fputs(",", stdout);
        mask = genmask(arg2, 0);
        sprintf(argbuf, "#%d", mask);
        fputs(argbuf, stdout);
    }
}

void
putlabel(s)
    char           *s;
{
    int             len;

    if (s != NULL) {
        if (strcmp(s, "_main:") == 0)
            s = "_PDC_main:";
        len = strlen(s) - 1;
        if (len > 2 && s[2] == '%')
            s[2] = '$';
        fputs(s, stdout);
        if (len > 0) {
            if (s[len] != ':' && (opcode == NULL || *opcode != '='))
                fputs(":", stdout);
        }
    }
}

void
translate()
{
    while (getline() != NULL) {
        lex();
        if (opcode && strcmp(opcode, "XREF") == 0)
            continue;
        changeopcode();
        if (opcode && strcmp(opcode, "SECTION") == 0) {
            if (strcmp(arg2, "CODE") == 0)
                opcode = ".text";
            if (strcmp(arg2, "DATA") == 0)
                opcode = ".data";
            if (strcmp(arg2, "BSS") == 0)
                opcode = ".bss";
        }
        if (opcode && *opcode == '=')
            fputs("\t", stdout);
        if (label != NULL) {
            putlabel(label);
        }
        fputs("\t", stdout);
        if (opcode != NULL) {
            if (strcmp(opcode, "moveml") == 0)
                domask();
            else {
                fputs(opcode, stdout);
                if (strcmp(opcode, ".data") == 0)
                    arg1 = arg2 = NULL;
                if (strcmp(opcode, ".text") == 0)
                    arg1 = arg2 = NULL;
                if (strcmp(opcode, ".bss") == 0)
                    arg1 = arg2 = NULL;
                if (strcmp(opcode, ".even") == 0)
                    arg1 = arg2 = NULL;
                if (arg1 != NULL) {
                    changearg(&arg1);
                    fputs("\t", stdout);
                    fputs(arg1, stdout);
                    if (arg2 != NULL) {
                        changearg(&arg2);
                        fputs(",", stdout);
                        fputs(arg2, stdout);
                    }
                }
            }
        }
        fputs("\n", stdout);

    }
}

void
main(argc, argv)
    int             argc;
    char           *argv[];

{
    translate();
}
