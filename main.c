#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <leif/leif.h>
#include <string.h>


typedef enum {
    TAB_DASHBOARD = 0,
    TAB_NEW_TASK
} gui_tab;

typedef enum {
    FILTER_ALL = 0,
    FILTER_IN_PROGRESS,
    FILTER_COMPLETED,
    FILTER_LOW,
    FILTER_MEDIUM,
    FILTER_HIGH
} entry_filter;

typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH
} entry_priority;

typedef struct {
    bool completed;
    char* desc, *date;
    entry_priority priority;
} task_entry;

#define WIN_MARGIN 20.0f

static int winw = 1280, winh = 720;
static LfFont titleFont, smallFont;
static entry_filter current_filter;
static gui_tab current_tab;

static task_entry* entries[1024];
static uint32_t num_entries = 0;

static LfTexture remove_texture, back_texture;

static LfInputField new_task_input;
static char new_task_input_buff[512];

char* get_command_output(const char* cmd) {
    FILE *fp;
    char buffer[1024];
    char *result = NULL;
    size_t result_size = 0;

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t buffer_len = strlen(buffer);
        char *temp = realloc(result, result_size + buffer_len + 1);
        if (temp == NULL) {
            printf("Memory allocation failed\n");
            free(result);
            pclose(fp);
            return  NULL;
        }
        result = temp;
        strcpy(result + result_size, buffer);
        result_size += buffer_len;
    }
    pclose(fp);
    return result;
}

static void render_top_bar() {
    lf_push_font(&titleFont);
    lf_text("Your To Do");
    lf_pop_font();

    {
        const float width = 160.0f;

        lf_set_ptr_x_absolute(winw - width - WIN_MARGIN * 2.0f);
        LfUIElementProps props = lf_get_theme().button_props;
        props.margin_left = 0.0f; props.margin_right = 0.0f;
        props.color = (LfColor){65, 167, 204, 255};
        props.border_width = 0.0f; props.corner_radius = 4.0f;
        lf_push_style_props(props);
        if (lf_button_fixed("New Task", width, -1) == LF_CLICKED) {
            current_tab = TAB_NEW_TASK;
        }
        lf_pop_style_props();
    }
}

static int compare_entry_priority(const void* a, const void* b) {
    task_entry* entry_a = *(task_entry**)a;
    task_entry* entry_b = *(task_entry**)b;
    return entry_b->priority - entry_a->priority;
}

static void sort_entries_by_priority() {
    qsort(entries, num_entries, sizeof(task_entry*), compare_entry_priority);
}

static void render_filters() {
    uint32_t num_filters = 6;
    static const char* filters[] = {"ALL", "IN PROGRESS", "COMPLETED", "LOW", "MEDIUM", "HIGH"};

    LfUIElementProps props = lf_get_theme().text_props;
    props.margin_top = 10.0f;
    props.margin_right = 10.0f;
    props.margin_right = 10.0f;
    props.padding = 10.0f;
    props.border_width = 0.0f;
    props.corner_radius = 8.0f;
    props.color = LF_NO_COLOR;
    props.text_color = LF_WHITE;

    float width = 0.0f;
    float ptr_x_before = lf_get_ptr_x();
    float ptr_y_before = lf_get_ptr_y();
    lf_push_style_props(props);
    lf_set_no_render(true);
    lf_set_ptr_y_absolute(lf_get_ptr_y() + 50.0f);
    for (uint32_t i = 0; i < num_filters; i++) {
        lf_button(filters[i]);
    }
    lf_set_no_render(false);
    lf_set_ptr_y_absolute(ptr_y_before);
    lf_pop_style_props();

    width = lf_get_ptr_x() - ptr_x_before - props.margin_right - props.padding;

    lf_set_ptr_x_absolute(winw - width - WIN_MARGIN * 2.0f);

    lf_set_line_should_overflow(false);
    for (uint32_t i = 0; i < num_filters; i++) {
        props.color = (current_filter == (entry_filter)i) ? (LfColor){250, 250, 250, 50}: LF_NO_COLOR;
        lf_push_style_props(props);
        if (lf_button(filters[i]) == LF_CLICKED) {
            current_filter = (entry_filter)i;
        }
        lf_pop_style_props();
    }
    lf_set_line_should_overflow(true);
}

static void render_entries() {
    lf_div_begin(((vec2s){lf_get_ptr_x(), lf_get_ptr_y()}),
                 ((vec2s){winw - lf_get_ptr_x() - WIN_MARGIN, (winh - lf_get_ptr_y() - WIN_MARGIN)}),
                 true);

    uint32_t rendered_count = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        task_entry *entry = entries[i];
        if (current_filter == FILTER_LOW && entry->priority != PRIORITY_LOW) continue;
        if (current_filter == FILTER_MEDIUM && entry->priority != PRIORITY_MEDIUM) continue;
        if (current_filter == FILTER_HIGH && entry->priority != PRIORITY_HIGH) continue;
        if (current_filter == FILTER_COMPLETED && !entry->completed) continue;
        if (current_filter == FILTER_IN_PROGRESS && entry->completed) continue;

        float priority_size = 15.0f;
        float ptr_y_before = lf_get_ptr_y();
        lf_set_ptr_y_absolute(lf_get_ptr_y() + 5.0f);
        lf_set_ptr_x_absolute(lf_get_ptr_x() + 5.0f);

        bool clicked_priority = lf_hovered((vec2s){lf_get_ptr_x(), lf_get_ptr_y()}, (vec2s){priority_size, priority_size})
                && lf_mouse_button_went_down(GLFW_MOUSE_BUTTON_LEFT);
        if (clicked_priority) {
            if (entry->priority + 1 >= PRIORITY_HIGH + 1) {
                entry->priority = 0;
            } else {
                entry->priority++;
            }
            sort_entries_by_priority();
        }
        switch (entry->priority) {
            case PRIORITY_LOW:
                lf_rect(priority_size, priority_size, (LfColor){76, 175, 80, 255}, 4.0f);
                break;
            case PRIORITY_MEDIUM:
                lf_rect(priority_size, priority_size, (LfColor){255, 235, 59, 255}, 4.0f);
                break;
            case PRIORITY_HIGH:
                lf_rect(priority_size, priority_size, (LfColor){244, 67, 54, 255}, 4.0f);
                break;
        }
        lf_set_ptr_y_absolute(ptr_y_before);
        {
            LfUIElementProps props = lf_get_theme().button_props;
            props.color = LF_NO_COLOR;
            props.border_width = 0.0f; props.padding = 0.0f; props.margin_top = 3.0f; props.margin_left = 10.0f;
            lf_push_style_props(props);
            if (lf_image_button(((LfTexture){.id = remove_texture.id, .width = 20.0f, .height = 20.0f})) == LF_CLICKED) {
                for (uint32_t j = i; j < num_entries - 1; j++) {
                    entries[j] = entries[j + 1];
                }
                num_entries--;
            }
            lf_pop_style_props();
        }
        {
            LfUIElementProps props = lf_get_theme().checkbox_props;
            props.border_width = 1.0f; props.corner_radius = 0.0f, props.margin_top = 0.0f; props.padding = 5.0f;
            props.margin_left = 5.0f;
            props.color = lf_color_from_zto((vec4s){0.05f, 0.05f, 0.05f, 1.0f});
            lf_push_style_props(props);
            if (lf_checkbox("", &entry->completed, LF_NO_COLOR, ((LfColor){65, 167, 204, 255})) == LF_CLICKED) {

            }
            lf_pop_style_props();
        }
        lf_push_font(&smallFont);
        LfUIElementProps props = lf_get_theme().text_props;
        props.margin_top = 0.0f;
        props.margin_left = 5.0f;
        lf_push_style_props(props);

        float desc_ptr_x = lf_get_ptr_x();
        lf_text(entry->desc);

        lf_set_ptr_x_absolute(desc_ptr_x);
        lf_set_ptr_y_absolute(lf_get_ptr_y() + smallFont.font_size);
        lf_pop_style_props();
        props.text_color = (LfColor){150, 150, 150, 150};

        lf_push_style_props(props);
        lf_text(entry->date);
        lf_pop_style_props();
        lf_pop_font();

        lf_next_line();

        rendered_count++;
    }
    if (!rendered_count) {
        lf_text("There is not task here");
    }
    lf_div_end();
}

static void render_new_task() {
    lf_push_font(&titleFont);
    {
        LfUIElementProps props = lf_get_theme().text_props;
        props.margin_bottom = 15.0f;
        lf_push_style_props(props);
        lf_text("Add a new Task");
        lf_pop_font();
        lf_pop_style_props();
    }

    lf_next_line();

    {
        lf_push_font(&smallFont);
        lf_text("Description");
        lf_pop_font();

        lf_next_line();
        LfUIElementProps props = lf_get_theme().inputfield_props;
        props.padding = 15.0f;
        props.color = lf_color_from_zto((vec4s){0.05f, 0.05f, 0.05f, 1.0f});
        props.corner_radius = 11.0f;
        props.text_color = LF_WHITE;
        props.border_width = 1.0f;
        props.border_color = new_task_input.selected ? LF_WHITE : (LfColor){170, 170, 170, 255};
        props.corner_radius = 2.5f;
        props.margin_bottom = 10.0f;
        lf_push_style_props(props);
        lf_input_text(&new_task_input);
        lf_pop_style_props();
    }

    lf_next_line();

    static int32_t selected_priority = -1;
    {
        lf_push_font(&smallFont);
        lf_text("Priority");
        lf_pop_font();

        lf_next_line();
        static const char* items[3] = {
                "Low",
                "Medium",
                "High"
        };
        static bool opened = false;
        LfUIElementProps props = lf_get_theme().button_props;
        props.color= (LfColor){70, 70, 70, 255};
        props.text_color = LF_WHITE;
        props.border_width = 0.0f; props.corner_radius = 5.0f;
        lf_push_style_props(props);
        lf_dropdown_menu(items, "Priority", 3, 200.0f, 80.0f, &selected_priority, &opened);
        lf_pop_style_props();
        lf_pop_style_props();
    }

    lf_next_line();

    {
        bool form_complete = (strlen(new_task_input_buff) && selected_priority != -1);
        const char* text = "Add";
        const float width = 150.0f;

        LfUIElementProps props = lf_get_theme().button_props;
        props.margin_left = 0.0f; props.margin_right = 0.0f;
        props.corner_radius = 5.0f; props.border_width = 0.0f;
        props.color = !form_complete ? (LfColor){80, 80, 80, 255} : (LfColor){65, 167, 204, 255};
        lf_push_style_props(props);
        lf_set_line_should_overflow(false);
        lf_set_ptr_x_absolute(winw - (width + props.padding * 2.0f + WIN_MARGIN));
        lf_set_ptr_y_absolute(winh - (lf_button_dimension(text).y + props.padding * 2.0f + WIN_MARGIN));
        if (lf_button_fixed(text, width, -1) == LF_CLICKED && form_complete) {
            task_entry *entry = (task_entry*) malloc(sizeof (*entry));
            entry->priority = selected_priority;
            entry->completed = false;
            entries[num_entries++] = entry;
            char* new_desc = malloc(strlen(new_task_input_buff));
            strcpy(new_desc, new_task_input_buff);
            entry->desc = new_desc;
            entry->date = get_command_output("date +\"%d.%m.%y, %H:%M\"");

            memset(new_task_input_buff, 0, 512);
            selected_priority = -1;
            sort_entries_by_priority();
        }
        lf_set_line_should_overflow(true);
        lf_pop_style_props();
    }

    lf_next_line();

    {
        LfUIElementProps props = lf_get_theme().button_props;
        props.color = LF_NO_COLOR; props.border_width = 0.0f;
        props.padding = 0.0f; props.margin_left = 0.0f; props.margin_top = 0.0f;
        props.margin_right = 0.0f; props.margin_bottom = 0.0f;
        lf_push_style_props(props);
        lf_set_line_should_overflow(false);
        LfTexture back_button = (LfTexture){.id = back_texture.id, .width = 20, .height = 40};
        lf_set_ptr_y_absolute(winh - back_button.height - WIN_MARGIN * 2.0f);
        lf_set_ptr_x_absolute(WIN_MARGIN);

        if (lf_image_button(back_button) == LF_CLICKED) {
            current_tab = TAB_DASHBOARD;
        }
        lf_set_line_should_overflow(true);
        lf_pop_style_props();
    }
}

int main(void) {
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(winw, winh, "Task Management", NULL, NULL);

    glfwMakeContextCurrent(window);

    lf_init_glfw(winw, winh, window);

    LfTheme theme = lf_get_theme();
    theme.div_props.color = LF_NO_COLOR;
    lf_set_theme(theme);

    titleFont = lf_load_font("../fonts/inter-bold.ttf", 40);
    smallFont = lf_load_font("../fonts/inter.ttf", 20);

    remove_texture = lf_load_texture("../icons/remove.png", true, LF_TEX_FILTER_LINEAR);
    back_texture = lf_load_texture("../icons/back.png", true, LF_TEX_FILTER_LINEAR);

    memset(new_task_input_buff, 0, 512);
    new_task_input = (LfInputField){
        .width = 400,
        .buf = new_task_input_buff,
        .buf_size = 512,
        .placeholder = "What is there to do?"
    };

    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

        lf_begin();

        lf_div_begin(((vec2s){WIN_MARGIN, WIN_MARGIN}),
                     ((vec2s){winw - WIN_MARGIN * 2.0f, winh - WIN_MARGIN * 2.0f}),
                     true);

        switch (current_tab) {
            case TAB_DASHBOARD:
                render_top_bar();
                lf_next_line();
                render_filters();
                lf_next_line();
                render_entries();
                break;
            case TAB_NEW_TASK:
                render_new_task();
                break;
            default:
                break;
        }

        lf_div_end();

        lf_end();

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    lf_free_font(&titleFont);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}