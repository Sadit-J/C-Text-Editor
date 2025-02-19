//Developer: Sadit Joarder
//Description:


#include <stdio.h>
#include <termio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

//Terminal Cursor properties

struct terminal_cursor {
    int x;
    int y;
};

//Each row of the text editor

struct editor_row {
    int row_size;
    int used_space;
    char* row_text;
};

static struct termios orig_termios, current_termios;
static struct terminal_cursor cursor;
static int term_width, term_height, current_selection;
static int selected_row, total_rows;
struct editor_row *row_list;


//--------------- Row in Text Editor ---------------//

int extendBuffer(struct editor_row *current_row) {
    //Doubles the size of a row if limit is reached
    char *temp = realloc(current_row->row_text, current_row->row_size * 2 * sizeof(char));
    if (temp == NULL) {
        perror("realloc");
        return -1;
    }

    current_row->row_text = temp;
    current_row->row_size = current_row->row_size * 2;

    return 0;
}

//Default assignment of a row
void createRows(struct editor_row *row, int row_size) {
    row->row_size = row_size;

    //Two used space to account for new line and carriage return characters
    row->used_space = 2;
    row->row_text = (char*) malloc((row_size * sizeof(char)));
}

int extendRows() {
    struct editor_row *temp;

    temp = realloc(row_list, total_rows * sizeof(struct editor_row));
    write(STDOUT_FILENO, "Tab", 3);
    if (temp == NULL) {
        write(STDOUT_FILENO, "Del", 3);
        perror("realloc");
        return -1;
    }
    row_list = temp;

    createRows(&row_list[total_rows - 1], row_list[total_rows - 2].row_size);

    //free(temp);

    return 0;
}

int insertCharacter(char input_character, int (position), struct editor_row *row) {

    //Position adjusted for the row
    position -= 1;
    cursor.x += 1;

    //Checks if row is full
    if (row->row_size == row->used_space) {
        extendBuffer(row);
    }

    //Adds a character in the middle of the string
    if (position < row->used_space) {
        memmove(row->row_text + (position + 1), row->row_text + (position), (row->row_size - (position - 1)) * sizeof(char));
        row->row_text[position] = input_character;
    }

    //If cursor is at the end of the string
    else {
        row->row_text[row->used_space - 2] = input_character;
    }

    row->used_space += 1;
    return 0;
}

int deleteCharacter(struct editor_row *row, int position) {

    //Adjust position to account for carriage return and new line character
    position = position - 2;

    //Memmove overlaps the character in between to delete the character
    //Conditions ensures there are characters to delete
    if (row->used_space > 2 && cursor.x > 1) {
        memmove(row->row_text + (position), row->row_text + (position + 1), (row->row_size - (position+1)) * sizeof(char));
        row->used_space -= 1;
        cursor.x -= 1;

        return 0;
    }

    return 1;
}



//--------------- Change Terminal Settings (Raw/Canonical) ---------------//

//Termios library to set terminal to original settings
void makeTerminalCanon(){
    tcsetattr(0, TCSANOW, &orig_termios);
}

//Set terminal to raw which takes in a continouous stream of input
//from the user
void makeTerminalRaw(){
    if (tcgetattr(0, &orig_termios) < 0) {
        printf("Error in tcsetattr()");
    }

    current_termios = orig_termios;
    cfmakeraw(&current_termios);

    tcsetattr(0, TCSANOW, &current_termios);
    atexit(makeTerminalCanon);
}

//--------------- Terminal Cursor ---------------//

int incrementCursor(const char KEY, struct editor_row *row) {


    //Increments the cursor to move it in the terminal
    switch (KEY) {
        case 65:
            cursor.y -= 1;
            if (cursor.y < 1) {
                cursor.y = 0;
            }

        break;
        case 66: //Down Arrow Key
            cursor.y += 1;
        break;
        case 67: //Right Arrow Key
            cursor.x += 1;
        break;
        default:
            cursor.x -= 1;
            if (cursor.x < 2) {
            cursor.x = 1;
            }
        break;
    }
    //Prevents the buffer from extending beyond the terminal height
    if (cursor.y == (term_height)) {
        current_selection += 1;
        cursor.y = term_height - 1;

    }
    else if (cursor.y < 1 && current_selection > term_height) {
        cursor.y = 0;
        current_selection -= 1;
    }

    selected_row = cursor.y + current_selection - term_height;

    //Ensures cursor does not go past how many characters are available in
    //the selected row
    if (cursor.x > row[selected_row].used_space - 1) {
        cursor.x = row[selected_row].used_space - 1;
    }

    if (selected_row >= (total_rows - 2)) {
        write(STDOUT_FILENO, "Tab", 3);
        total_rows += 1;
        extendRows();

    }
    return 0;





}


//--------------- Process KeyBoard Input ---------------//

void keyboardPressChecker(const char KEY, struct editor_row *row_list) {
    fflush(stdout);
    system("clear");

    switch (KEY) {
        case 127: //DEL produced by Backspace
            deleteCharacter(&row_list[selected_row], cursor.x);
        break;
        case 9: //Tab
            write(STDOUT_FILENO, "Tab", 3);
        break;
        case 13: //Enter
            write(STDOUT_FILENO, "Enter", 5);
        break;
        case 8: //DEL produced by Backspace


        break;
        case 65 ... 68: //Up Arrow Key
            incrementCursor(KEY, row_list);
        break;
        default:
            insertCharacter(KEY, cursor.x, &row_list[selected_row]);
        break;
    }

}

//--------------- Screen ---------------//
void screenAppend(struct editor_row *row, char *buffer) {
    if (row->row_size == row->used_space) {
    }

}

int screenBuffer(struct editor_row *row_list, struct winsize window_size) {

    int capacity = 0;
    int count = 40;
    int ptr = 0;
    char* test;
    FILE *fptr;

    // Open a file in writing mode
    fptr = fopen("filename.txt", "w");


    char* screen_buffer = (char*)malloc(capacity*sizeof(int));
    for (int i = (current_selection - term_height); i < current_selection; i++)
    {

        capacity += row_list[i].used_space;
        row_list[i].row_text[row_list[i].used_space - 2] = '\r';
        row_list[i].row_text[row_list[i].used_space - 1] = '\n';
        test = (char*) realloc(screen_buffer, capacity*sizeof(int)); //Create test buffer in case of failure
        if (test == NULL) {
            perror("Error in realloc()");
            return -1;
        }
        screen_buffer = test;
        memmove(screen_buffer + ptr, row_list[i].row_text, row_list[i].used_space + 1);

        ptr += row_list[i].used_space;
    }

    screen_buffer[ptr] = '\0';
    // Write some text to the file
    fprintf(fptr, screen_buffer);

    write(STDOUT_FILENO, screen_buffer, strlen(screen_buffer) * sizeof(char));
    free(screen_buffer);


    // Close the file
    fclose(fptr);

    return 0;

}


void createScreen(char *buf, struct winsize window, struct editor_row row_list[]) {

}



int main(void) {

    //struct editor_row row_list[10000];
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    fflush(stdout);
    system("clear");
    makeTerminalRaw();

    row_list = malloc(sizeof(struct editor_row) * w.ws_row);

    for (int row = 0; row < w.ws_row; row++) {
        createRows(&row_list[row], w.ws_col);
    }

    term_height = w.ws_row - 2;
    total_rows = w.ws_row;
    current_selection = term_height;

    cursor.x = 1;
    cursor.y = 0;
    selected_row = cursor.y + current_selection - term_height;

    while (1) {
        char buf[32] = {'\0'};
        char c[4] = {'\0'};

        read(STDIN_FILENO, &c, 3);
        if (c[0] == 'q') break;

        if (c[0] != '\0') {
            if ((c[1] == '[')) {
                keyboardPressChecker(c[2], row_list);
            }
            else {
                keyboardPressChecker(c[0], row_list);
            }
            snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (cursor.y + 1), (cursor.x));
            screenBuffer(row_list, w);
            write(STDOUT_FILENO, buf, strlen(buf));
        }
    }

    makeTerminalCanon();

    return 0;
}