#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <termios.h>
#include <locale.h>
#include <wchar.h>

#define CSI "\e["
#define CUU CSI"A"
#define CUD CSI"B"
#define CUF CSI"C"
#define CUB CSI"D"
#define CUP CUB CUU

#define BASE_WIDTH  (1 << 2)


struct termios original_terminal_attributes;


void risk2style(const float r)
{
    assert((r >= 0.f) && (r <= 1.f));

    const char * const styles[] = {
        CSI"92m",
        CSI"92m",
        CSI"93m",
        CSI"91m",
        CSI"91m"
    };
    const int l = (sizeof(styles) / sizeof(*styles)) - 1;

    printf("%s", styles[(int) floorf(r * l)]);
}

void printHbar(const float usage, const int width)
{
    assert((usage >= 0.f) && (usage <= 1.f));

    const int total_subbars = width * 8;
    const int quantized_usage = (int) roundf(usage * total_subbars);
    int i = 0;

    // printf("%lc", L'▕');
    
    risk2style(0.49f);
    for (; (i*8 < total_subbars/2) && (i < quantized_usage/8); ++i)
    {
        printf("%lc", L'█');
    }
    
    risk2style(0.74f);
    for (; (i*8 < total_subbars/2 + total_subbars/4) && (i < quantized_usage/8); ++i)
    {
        printf("%lc", L'█');
    }
    
    risk2style(0.99f);
    for (; (i*8 < total_subbars) && (i < quantized_usage/8); ++i)
    {
        printf("%lc", L'█');
    }

    risk2style(usage);
    printf("%lc", L"\0▏▎▍▌▋▊▉█"[quantized_usage & 0b111]);
    i += !!(quantized_usage & 0b111);

    printf(CSI"m");
    for (; i*8 < total_subbars; ++i)
    {
        printf("%lc", L' ');
    }
    // printf(CSI"m" "%lc", L'▏');
    printf(CSI"m");
}

void printVbar(const float usage, const int height)
{
    assert(!"WIP");


    assert((usage >= 0.f) && (usage <= 1.f));

    const int total_subbars = height * 8;
    const int quantized_usage = (int) roundf(usage * total_subbars);
    int i = 0;

    for (int j = 0; j < height; ++j)
    {
        printf(CUD);
    }

    risk2style(0.49f);
    for (; (i*8 < total_subbars/2) && (i < quantized_usage/8); ++i)
    {
        printf("%lc"CUP, L'█');
    }
    
    risk2style(0.74f);
    for (; (i*8 < total_subbars/2 + total_subbars/4) && (i < quantized_usage/8); ++i)
    {
        printf("%lc"CUP, L'█');
    }
    
    risk2style(0.99f);
    for (; (i*8 < total_subbars) && (i < quantized_usage/8); ++i)
    {
        printf("%lc"CUP, L'█');
    }

    risk2style(usage);
    printf("%lc"CUP, L"\0▁▂▃▄▅▆▇▉"[quantized_usage & 0b111]);
    i += !!(quantized_usage & 0b111);

    printf(CSI"m");
    for (; i*8 < total_subbars; ++i)
    {
        printf("%lc"CUP, L' ');
        // printf(CUP);
    }
    printf(CSI"m" CUF);
}

void probeCPU(const int nproc, uint64_t clocks[][7])
{
    FILE *fp;
    if (NULL == (fp = fopen("/proc/stat", "r")))
    {
        perror("Open `/proc/stat`");
        exit(EXIT_FAILURE);
    }

    int check = 0;
    for (int i = 0; i <= nproc; ++i)
    {
        check += fscanf(fp, "%*[^ ] %lu %lu %lu %lu %lu %lu %lu%*[^\n]\n",
            clocks[i]+0, clocks[i]+1, clocks[i]+2, clocks[i]+3, clocks[i]+4, clocks[i]+5, clocks[i]+6);
    }

    if (check != (nproc + 1) * 7)
    {
        perror("Read data from `/proc/stat` (unexpected format)");
        exit(EXIT_FAILURE);
    }

    if (EOF == fclose(fp))
    {
        perror("Close `/proc/stat`");
        exit(EXIT_FAILURE);
    }
}

void printCPU(const int nproc, const float delay)
{
    static uint64_t clocks[2][128][7];
    static uint64_t totals[7];
    static uint64_t idles[7];

    probeCPU(nproc, clocks[0]);
    usleep(delay * 1e6f);
    probeCPU(nproc, clocks[1]);

    for (int i = 0; i <= nproc; ++i)
    {
        totals[i] =
            +clocks[1][i][0] - clocks[0][i][0]
            +clocks[1][i][1] - clocks[0][i][1]
            +clocks[1][i][2] - clocks[0][i][2]
            +clocks[1][i][3] - clocks[0][i][3]
            +clocks[1][i][4] - clocks[0][i][4]
            +clocks[1][i][5] - clocks[0][i][5]
            +clocks[1][i][6] - clocks[0][i][6];
    }

    for (int i = 0; i <= nproc; ++i)
    {
        idles[i] = clocks[1][i][3] - clocks[0][i][3];
    }

    const float usage = 1.f - (float) idles[0]/totals[0];
    printf("CPU ");
    printHbar(usage, nproc * BASE_WIDTH);
    printf(CSI"m" "%lc", L'▏');
    risk2style(usage);
    printf("%4.1f%%  \n    ", 100*usage);
    // printf(CSI"m" " [");
    for (int i = 1; i <= nproc; ++i)
    {
        const float usage = 1.f - (float) idles[i]/totals[i];
        printHbar(usage, BASE_WIDTH);
        // risk2style(usage);
        // printf("%4.1f%% ", 100*usage);
    }
    printf(CSI"m" "%lc", L'▏');

/*
    const float usage = 1.f - (float) idles[0]/totals[0];
    printf("    ");
    printVbar(usage, 4);
    printVbar(usage, 4);
    printf(" ");
    for (int i = 1; i <= nproc; ++i)
    {
        const float usage = 1.f - (float) idles[i]/totals[i];
        printVbar(usage, 4);
        // risk2style(usage);
        // printf("%4.1f%% ", 100*usage);
    }
    printf(CSI"m");
*/
}

void printRAM(const int nproc)
{
    int MemTotal;
    int MemFree;
    int MemAvailable;
    int Buffers;
    int Cached;
    int SwapTotal;
    int SwapFree;

    FILE *fp;
    if (NULL == (fp = fopen("/proc/meminfo", "r")))
    {
        perror("Open `/proc/meminfo`");
        exit(EXIT_FAILURE);
    }

    const int check = fscanf(fp,
        "MemTotal:"	        "%*[ ]%d"	" kB\n"
        "MemFree:"	        "%*[ ]%d"	" kB\n"
        "MemAvailable:"	    "%*[ ]%d"	" kB\n"
        "Buffers:"	        "%*[ ]%d"	" kB\n"
        "Cached:"	        "%*[ ]%d"	" kB\n"
        "SwapCached:"	    "%*[ ]%*d"	" kB\n"
        "Active:"	        "%*[ ]%*d"	" kB\n"
        "Inactive:"	        "%*[ ]%*d"	" kB\n"
        "Active(anon):"	    "%*[ ]%*d"	" kB\n"
        "Inactive(anon):"	"%*[ ]%*d"	" kB\n"
        "Active(file):"	    "%*[ ]%*d"	" kB\n"
        "Inactive(file):"	"%*[ ]%*d"	" kB\n"
        "Unevictable:"	    "%*[ ]%*d"	" kB\n"
        "Mlocked:"	        "%*[ ]%*d"	" kB\n"
        "SwapTotal:"	    "%*[ ]%d"	" kB\n"
        "SwapFree:"	        "%*[ ]%d"	" kB\n"
        , &MemTotal
        , &MemFree
        , &MemAvailable
        , &Buffers
        , &Cached
        , &SwapTotal
        , &SwapFree
    );

    if (check != 7)
    {
        perror("Read data from `/proc/meminfo` (unexpected format)");
        exit(EXIT_FAILURE);
    }

    if (EOF == fclose(fp))
    {
        perror("Close `/proc/meminfo`");
        exit(EXIT_FAILURE);
    }

    const int MemUsed = MemTotal - MemAvailable;
    const float usage = (float) MemUsed/MemTotal;
    printf("RAM ");
    printHbar(usage, nproc * BASE_WIDTH);
    printf(CSI"m" "%lc", L'▏');
    risk2style(usage);
    printf("%4.1f%%  %.1f" CSI"m" "/%.1fGi", 100.f*MemUsed/MemTotal, MemUsed*0x1p-20f, MemTotal*0x1p-20f);
    printf(CSI"2;90m" "  [%.1f = %.1f + %.1f(f) + %.1f(b) + %.1f(c)]", MemTotal*0x1p-20f, MemUsed*0x1p-20f, MemFree*0x1p-20f, Buffers*0x1p-20f, Cached*0x1p-20f);
    // const float swapMb = (SwapTotal - SwapFree) * 0x1p-10f;
    // if (swapMb > 100)
    // {
    //     printf(CSI"0;1;5;91m" "\tSwap: %.0fMi", swapMb);
    // }
    puts(CSI"m" "   ");
}

void restore_terminal_attributes()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_attributes);

    struct termios check_attributes;
    memset(&check_attributes, 0, sizeof(check_attributes));
    if (tcgetattr(STDIN_FILENO, &check_attributes) == -1)
    {
        perror("Get terminal attributes");
    }
    if (memcmp(&check_attributes, &original_terminal_attributes, sizeof(check_attributes)))
    {
        perror("Restore terminal attibutes");
    }
}

void set_terminal_to_raw()
{
    memset(&original_terminal_attributes, 0, sizeof(original_terminal_attributes));
    if (tcgetattr(STDIN_FILENO, &original_terminal_attributes) == -1)
    {
        perror("Get original terminal attributes");
        exit(EXIT_FAILURE);
    }

    struct termios raw = original_terminal_attributes;

    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    struct termios check_attributes;
    memset(&check_attributes, 0, sizeof(check_attributes));
    if (tcgetattr(STDIN_FILENO, &check_attributes) == -1)
    {
        perror("Get terminal attributes");
        exit(EXIT_FAILURE);
    }
    if (memcmp(&check_attributes, &raw, sizeof(check_attributes)))
    {
        perror("Set raw terminal attibutes");
        exit(EXIT_FAILURE);
    }
    atexit(restore_terminal_attributes);
}

bool check_for_key(const char key)
{
    char ch;
    while (read(STDIN_FILENO, &ch, 1) == 1)
    {
        if (ch == key)
        {
            return true;
        }
    }

    if (errno && errno != EAGAIN)
    {
        perror("Read from `STDIN`");
        exit(EXIT_FAILURE);
    }

    return false;
}

int main(const int argc, char * const argv[])
{
    float interval;
    if (argc == 1)
    {
        interval = 1;
    }
    else if ((argc != 2) || (sscanf(argv[1], "%f", &interval) != 1))
    {
        puts("Usage: " __FILE__ " <interval (secs)>");
        return EXIT_FAILURE;
    }

    const int nproc = get_nprocs_conf();
    assert(nproc > 0);

    setlocale(LC_CTYPE, "");

    set_terminal_to_raw();
    printf(CSI"?25l");  // Hide cursor
    // printf(CSI"2J");    // Clear terminal
    printf(CSI"s");     // Save cursor position

    while (!check_for_key('q'))
    {
        // printf(CSI";H");    // Move cursor to up left corner
        
        printf(CSI"u"); // Restore cursor position
        printRAM(nproc);

        printf(CSI"u"); // Restore cursor position
        printf(CUD);    // Move cursor one line down
        printCPU(nproc, interval);

        fflush(stdin);
    }
    puts(CSI"?25h");    // Unhide cursor

    return EXIT_SUCCESS;
}