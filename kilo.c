#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

struct editorConfig {
    int rows;
    int columns;
struct termios orig_termios;
};

struct editorConfig config;

void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1B[H", 3);

    perror(s);
    exit(1);
}

void drawTildes(void){
    for(int x = 0; x <= config.rows; x++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

int getWindowSize(int *rows, int *columns){
    struct winsize window;

    if(ioctl(STDIN_FILENO, TIOCGWINSZ, &window) == -1 || window.ws_col == 0){
        return -1;
    } else {
        *rows = window.ws_col;
        *columns = window.ws_row;
        return 0;
    }

}

void clearScreen(void){
    write(STDOUT_FILENO, "\x1B[2J", 4);
    write(STDOUT_FILENO, "\x1B[H", 3);
    drawTildes();
    write(STDOUT_FILENO, "\x1B[H", 3);
}

void disableRawMode(void) {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &config.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode(void){
    // Get the terminal attributes
    if(tcgetattr(STDIN_FILENO, &config.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
   
    // Modify the struct to turn on raw mode
    struct termios raw = config.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_oflag &= ~(OPOST);
    raw.c_iflag &= ~(ICRNL | IXON | INPCK | ISTRIP | BRKINT);
    raw.c_cflag &= ~(CS8);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // Set the terminal attributes
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void initalizeEditor(void){
    if(getWindowSize(&config.rows, &config.columns) == -1) die("getWindowSize");
}

void sig_handler(int sig){
    if(SIGWINCH == sig){
        initalizeEditor();
        clearScreen();
    }
}

int main(void){
    enableRawMode();
    initalizeEditor();
    clearScreen();
    
    signal(SIGWINCH, sig_handler);

    while(1){
        char c = '\0';
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        if(iscntrl(c)){
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }

        if(c == 'q'){
            write(STDOUT_FILENO, "\x1B[2J", 4);
            write(STDOUT_FILENO, "\x1B[H", 3);
            break;
        }
    }

    return 0;
}
