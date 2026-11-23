#include "system.h"

#include <algorithm>
#include <new>

#include "ncurses/ncurses.h"
#include "ncurses/menu.h"
#include "ncurses/form.h"

namespace os{
TerminalWindow::TerminalWindow(void)
{
    
}

TerminalWindow::~TerminalWindow(void)
{
    
}

std::pair<std::string, std::string> 
TerminalWindow::display_menu(std::vector<std::string> proj_name, std::vector<std::string> proj_path)
{
    std::pair<std::string, std::string> choose_value;
    if (proj_path.size() == 0 || proj_path.size() != proj_name.size()) {
        return choose_value;
    }
    if (initscr() == nullptr) {  // 以 curses 模式初始化终端
        return choose_value;
    }
    start_color();
    cbreak();   // 当缓存中有数据可读时直接返回而不是等到换行符或是缓冲满了才返回
    noecho();   // 输入字符不回显
    keypad(stdscr, TRUE); // 支持读取功能键像 F1, F2, arrow keys etc
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);

    std::vector<std::string> display_paths;
    for (std::size_t i = 0; i < proj_path.size(); ++i) {
        int32_t path_start = static_cast<int32_t>(proj_path[i].length()) - 40;
        if (path_start > 0) { // 路径过长只显示后面一部分
            std::string path = "...";
            path.append(proj_path[i].c_str() + path_start);
            display_paths.push_back(path);
        } else {
            display_paths.push_back(proj_path[i]);
        }
    }

    std::size_t display_num = display_paths.size();
    ITEM** items = new (std::nothrow) ITEM*[display_num + 1];
    if (items == nullptr) {
        endwin();
        return choose_value;
    }

    std::size_t created_items = 0;
    for (std::size_t i = 0; i < display_num; ++i) {
        items[i] = new_item(proj_name[i].c_str(), display_paths[i].c_str());
        if (items[i] == nullptr) {
            break;
        }
        ++created_items;
    }
    if (created_items != display_num) {
        for (std::size_t i = 0; i < created_items; ++i) {
            free_item(items[i]);
        }
        delete[] items;
        endwin();
        return choose_value;
    }
    items[display_num] = nullptr;

    MENU *project_menu = new_menu(items);
    if (project_menu == nullptr) {
        for (std::size_t i = 0; i < display_num; ++i) {
            free_item(items[i]);
        }
        delete[] items;
        endwin();
        return choose_value;
    }

    WINDOW *menu_window = newwin(23, 79, 0, 0);
    if (menu_window == nullptr) {
        free_menu(project_menu);
        for (std::size_t i = 0; i < display_num; ++i) {
            free_item(items[i]);
        }
        delete[] items;
        endwin();
        return choose_value;
    }
    keypad(menu_window, TRUE);

    WINDOW *sub_window = derwin(menu_window, 20, 74, 3, 1);
    if (sub_window == nullptr) {
        delwin(menu_window);
        free_menu(project_menu);
        for (std::size_t i = 0; i < display_num; ++i) {
            free_item(items[i]);
        }
        delete[] items;
        endwin();
        return choose_value;
    }

    /* Set main window and sub window */
    set_menu_win(project_menu, menu_window);
    set_menu_sub(project_menu, sub_window);
    set_menu_format(project_menu, 15, 1);

    /* Set menu mark to the std::string " * " */
    set_menu_mark(project_menu, " > ");

    /* Print a border around the main window and print a title */
    box(menu_window, 0, 0);
    this->print_in_middle(menu_window, 1, 0, 79, "Open Project(press 'q or Q' to quit)", COLOR_PAIR(1));
    mvwaddch(menu_window, 2, 0, ACS_LTEE);
    mvwhline(menu_window, 2, 1, ACS_HLINE, 77);
    mvwaddch(menu_window, 2, 78, ACS_RTEE);

    post_menu(project_menu);
    wrefresh(menu_window);

    while(true) // 回车键
    {   
        int ch = wgetch(menu_window);
        if (ch == Keyboard_Enter) { // 回车键退出
            if (project_menu->curitem == nullptr || project_menu->curitem->index < 0
                || static_cast<std::size_t>(project_menu->curitem->index) >= proj_path.size()) {
                break;
            }
            choose_value.second = proj_path[project_menu->curitem->index]; // 返回项目路径
            choose_value.first = proj_name[project_menu->curitem->index];
            break;
        } else if (ch == 'q' || ch == 'Q') {
            choose_value.second = "";
            choose_value.first = "";
            break;
        }

        switch(ch)
        {   
            case KEY_DOWN: {
                menu_driver(project_menu, REQ_DOWN_ITEM);
            } break;
            case KEY_UP: {
                menu_driver(project_menu, REQ_UP_ITEM);
            } break;
            case KEY_NPAGE: {
                menu_driver(project_menu, REQ_SCR_DPAGE);
            } break;
            case KEY_PPAGE: {
                menu_driver(project_menu, REQ_SCR_UPAGE);
            } break;
        }
        wrefresh(menu_window);
    }

    free_menu(project_menu);
    for (std::size_t i = 0; i < display_num; ++i) {
        free_item(items[i]);
    }
    delwin(sub_window);
    delwin(menu_window);
    endwin();
    delete[] items;
    return choose_value;
}

int 
TerminalWindow::get_input(std::string &input, std::string title, std::string default_value)
{
    FIELD *field[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    FORM  *my_form = nullptr;

    /* Initialize curses */
    if (initscr() == nullptr) {
        return -1;
    }
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    init_pair(1, COLOR_RED, COLOR_BLACK);
    /* Initialize the fields */
    field[0] = new_field(1, 70, 8, 7, 0, 0);
    field[1] = new_field(1, 70, 10, 7, 0, 0);
    field[2] = new_field(1, 6, 12, 24, 0, 0);
    field[3] = new_field(1, 10, 12, 40, 0, 0);
    if (field[0] == nullptr || field[1] == nullptr || field[2] == nullptr || field[3] == nullptr) {
        for (std::size_t i = 0; i < 4; ++i) {
            if (field[i] != nullptr) {
                free_field(field[i]);
            }
        }
        endwin();
        return -1;
    }

    /* Create the form and post it */
    my_form = new_form(field);
    if (my_form == nullptr) {
        for (std::size_t i = 0; i < 4; ++i) {
            free_field(field[i]);
        }
        endwin();
        return -1;
    }
    post_form(my_form);
    refresh();

    /* Set field options */
    field_opts_off(field[0], O_ACTIVE); /* This field is a static label */
    set_field_just(field[0], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[0], 0, title.c_str()); 

    set_field_back(field[1], A_UNDERLINE);
    field_opts_off(field[1], O_AUTOSKIP); /* Don't go to next field when this Field is filled up */
    set_field_buffer(field[1], 0, default_value.c_str());

    set_field_just(field[2], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[2], 0, "[ OK ]"); 
    field_opts_off(field[2], O_AUTOSKIP);
    field_opts_off(field[2], O_EDIT);

    set_field_just(field[3], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[3], 0, "[ Cancel ]"); 
    field_opts_off(field[3], O_AUTOSKIP);
    field_opts_off(field[3], O_EDIT);
    
    mvprintw(8, 0, "Title: ");
    mvprintw(10, 0, "Input: ");
    refresh();

    /* Loop through to get user requests */
    int ret_status = 0;
    std::size_t input_char_count = std::min(default_value.size(), static_cast<std::size_t>(Form_MaxInput));
    std::size_t current_cursor_pos = input_char_count;
    for (std::size_t i = 0; i < input_char_count; ++i) { // 移到默认字符串尾部
        form_driver(my_form, REQ_NEXT_CHAR);
    }
    while(true)
    {
        int ch = getch();
        if (ch == Keyboard_Enter ){ 
            if (my_form->current == field[2]) { // 回车键退出
                form_driver(my_form, REQ_VALIDATION);
                char *field_value = field_buffer(field[1], 0);
                if (field_value != nullptr) {
                    input.assign(field_value, input_char_count);
                }
                break;
            } else if (my_form->current == field[3]) {
                ret_status = -1;
                break;
            }
        } else if (ch == Keyboard_Esc) {
            ret_status = -1;
            break;
        }

        switch(ch)
        {
            case KEY_LEFT: {
                /* 移到上一个字符 */
                if (current_cursor_pos > 0) {
                    --current_cursor_pos;
                    form_driver(my_form, REQ_PREV_CHAR);
                }
            } break;
            case KEY_RIGHT: {
                /* 移到下一个字符 */
                if (current_cursor_pos < input_char_count) {
                    ++current_cursor_pos;
                    form_driver(my_form, REQ_NEXT_CHAR);
                }
            } break;
            case KEY_BACKSPACE: {
                if (input_char_count > 0) {
                    form_driver(my_form, REQ_DEL_PREV);
                    --input_char_count;
                    if (current_cursor_pos > 0) {
                        --current_cursor_pos;
                    }
                }
            } break;
            case Keyboard_Tab: {
                form_driver(my_form, REQ_NEXT_FIELD);
                form_driver(my_form, REQ_END_LINE);
            } break;
            default: {
                bool isPrintChar = false;
                if (ch >= '0' && ch <= '9') {
                    isPrintChar = true;
                } else if (ch >= 'a' && ch <= 'z') {
                    isPrintChar = true;
                } else if (ch >= 'A' && ch <= 'Z') {
                    isPrintChar = true;
                } else {
                    for (size_t i = 0; i < strlen(PrintChars); ++i) {
                        if (ch == PrintChars[i]) {
                            isPrintChar = true;
                            break;
                        }
                    }
                }

                if (isPrintChar && input_char_count < Form_MaxInput) {
                    form_driver(my_form, ch);
                    ++input_char_count;
                    ++current_cursor_pos;
                }
            }
        }
    }

    /* Un post form and free the memory */
    unpost_form(my_form);
    free_form(my_form);
    for (std::size_t i = 0; i < 4; ++i) {
        free_field(field[i]);
    }

    endwin();
    return ret_status;
}

// 消息提示
bool 
TerminalWindow::message(std::string title)
{
    FIELD *field[4] = {nullptr, nullptr, nullptr, nullptr};
    FORM  *my_form = nullptr;

    /* Initialize curses */
    if (initscr() == nullptr) {
        return false;
    }
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    /* Initialize the fields */
    field[0] = new_field(1, 70, 10, 7, 0, 0);
    field[1] = new_field(1, 6, 12, 28, 0, 0);
    field[2] = new_field(1, 10, 12, 42, 0, 0);
    if (field[0] == nullptr || field[1] == nullptr || field[2] == nullptr) {
        for (std::size_t i = 0; i < 3; ++i) {
            if (field[i] != nullptr) {
                free_field(field[i]);
            }
        }
        endwin();
        return false;
    }

    /* Create the form and post it */
    my_form = new_form(field);
    if (my_form == nullptr) {
        for (std::size_t i = 0; i < 3; ++i) {
            free_field(field[i]);
        }
        endwin();
        return false;
    }
    post_form(my_form);
    refresh();

    /* Set field options */
    field_opts_off(field[0], O_ACTIVE); /* This field is a static label */
    set_field_just(field[0], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[0], 0, title.c_str()); 

    set_field_just(field[1], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[1], 0, "[ OK ]"); 
    field_opts_off(field[1], O_AUTOSKIP);
    field_opts_off(field[1], O_EDIT);

    set_field_just(field[2], JUSTIFY_CENTER); /* Center Justification */
    set_field_buffer(field[2], 0, "[ Cancel ]"); 
    field_opts_off(field[2], O_AUTOSKIP);
    field_opts_off(field[2], O_EDIT);
    refresh();

    /* Loop through to get user requests */
    bool choose = false;
    while(true)
    {
        int ch = getch();
        if (ch == Keyboard_Enter ){ 
            if (my_form->current == field[1]) { // 回车键退出
                choose = true;
                break;
            } else if (my_form->current == field[2]) {
                choose = false;
                break;
            }
        } else if (ch == Keyboard_Esc) {
            choose = false;
            break;
        }
        
        switch (ch)
        {
            case Keyboard_Tab: {
                form_driver(my_form, REQ_NEXT_FIELD);
                form_driver(my_form, REQ_END_LINE);
            } break;
        }
    }
    
    /* Un post form and free the memory */
    unpost_form(my_form);
    free_form(my_form);
    for (std::size_t i = 0; i < 3; ++i) {
        free_field(field[i]);
    }

    endwin();
    return choose;
}

void 
TerminalWindow::print_in_middle(void *win, int starty, int startx, int width, const char *string, uint32_t color)
{       
    if (string == nullptr) {
        return;
    }
    if (win == nullptr) {
        win = stdscr;
    }

    WINDOW* win_ptr = reinterpret_cast<WINDOW*>(win);
    if (win_ptr == nullptr) {
        return;
    }

    int x = 0, y = 0;
    getyx(win_ptr, y, x);
    if (startx != 0) {
        x = startx;
    }
    if (starty != 0) {
        y = starty;
    }
    if (width <= 0) {
        width = 80;
    }

    int length = static_cast<int>(strlen(string));
    int padding = width - length;
    if (padding < 0) {
        padding = 0;
    }
    x = startx + padding / 2;
    wattron(win_ptr, color);
    mvwprintw(win_ptr, y, x, "%s", string);
    wattroff(win_ptr, color);
    wrefresh(win_ptr);
}

}