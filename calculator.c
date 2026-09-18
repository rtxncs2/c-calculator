#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

static GtkWidget *entry;
static double first_operand = 0.0;
static char pending_op = '\0';
static bool clear_on_next_input = false;

// Apply dark styling scaled up by 50%
static void apply_dark_theme(void) {
    const char *css =
        "window {"
        "   background-color: #18181b;"
        "}"
        "entry {"
        "   background-color: #09090b;"
        "   color: #ffffff;"
        "   font-size: 36px;"           /* Scaled from 24px */
        "   font-weight: bold;"
        "   border: 2px solid #27272a;"
        "   border-radius: 12px;"       /* Scaled from 8px */
        "   padding: 16px;"             /* Scaled from 10px */
        "   margin-bottom: 10px;"
        "}"
        "button {"
        "   background-color: #27272a;"
        "   background-image: none;"
        "   border: 1px solid #3f3f46;"
        "   border-radius: 12px;"       /* Scaled from 8px */
        "   box-shadow: none;"
        "   min-height: 58px;"          /* Added to make buttons physically taller */
        "}"
        "button label {"
        "   color: #ffffff;"
        "   font-size: 27px;"           /* Scaled from 18px */
        "   font-weight: bold;"
        "}"
        "button:hover {"
        "   background-color: #3f3f46;"
        "}"
        "button:active {"
        "   background-color: #18181b;"
        "}"
        "button.op-btn {"
        "   background-color: #f97316;"
        "   border: none;"
        "}"
        "button.op-btn label {"
        "   color: #ffffff;"
        "   font-weight: bold;"
        "}"
        "button.op-btn:hover {"
        "   background-color: #ea580c;"
        "}"
        "button.clear-btn {"
        "   background-color: #ef4444;"
        "   border: none;"
        "}"
        "button.clear-btn label {"
        "   color: #ffffff;"
        "   font-weight: bold;"
        "}"
        "button.clear-btn:hover {"
        "   background-color: #dc2626;"
        "}";

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}

// Helper to extract the active second number from a formatted expression string like "12 + 5"
static double extract_second_operand(const char *str) {
    const char *last_space = strrchr(str, ' ');
    if (last_space != NULL && *(last_space + 1) != '\0') {
        return atof(last_space + 1);
    }
    return atof(str);
}

static void process_action(const char *text) {
    const char *current = gtk_entry_get_text(GTK_ENTRY(entry));

    // Clear everything
    if (strcmp(text, "C") == 0) {
        gtk_entry_set_text(GTK_ENTRY(entry), "0");
        first_operand = 0.0;
        pending_op = '\0';
        clear_on_next_input = false;
        return;
    }

    // Backspace handling
    if (strcmp(text, "BACKSPACE") == 0) {
        if (clear_on_next_input) return;
        int len = strlen(current);
        if (len <= 1 || strcmp(current, "Error") == 0) {
            gtk_entry_set_text(GTK_ENTRY(entry), "0");
        } else {
            char buf[256];
            if (len >= 3 && current[len - 1] == ' ' && current[len - 3] == ' ') {
                strncpy(buf, current, len - 3);
                buf[len - 3] = '\0';
                pending_op = '\0';
            } else {
                strncpy(buf, current, len - 1);
                buf[len - 1] = '\0';
            }
            gtk_entry_set_text(GTK_ENTRY(entry), buf);
        }
        return;
    }

    // Operators: +, -, *, /
    if (strcmp(text, "+") == 0 || strcmp(text, "-") == 0 ||
        strcmp(text, "*") == 0 || strcmp(text, "/") == 0) {

        if (strcmp(current, "Error") == 0) return;

        char buf[256];

        // Swap operator if pressed immediately after
        if (pending_op != '\0') {
            int len = strlen(current);
            if (len >= 3 && current[len - 1] == ' ') {
                pending_op = text[0];
                snprintf(buf, sizeof(buf), "%.*s%c ", len - 2, current, pending_op);
                gtk_entry_set_text(GTK_ENTRY(entry), buf);
                return;
            } else {
                double second_operand = extract_second_operand(current);
                double intermediate = 0.0;
                switch (pending_op) {
                    case '+': intermediate = first_operand + second_operand; break;
                    case '-': intermediate = first_operand - second_operand; break;
                    case '*': intermediate = first_operand * second_operand; break;
                    case '/':
                        if (second_operand == 0.0) {
                            gtk_entry_set_text(GTK_ENTRY(entry), "Error");
                            pending_op = '\0';
                            clear_on_next_input = true;
                            return;
                        }
                        intermediate = first_operand / second_operand;
                        break;
                }
                first_operand = intermediate;
            }
        } else {
            first_operand = atof(current);
        }

        pending_op = text[0];
        char num_buf[64];
        snprintf(num_buf, sizeof(num_buf), "%.8g", first_operand);
        snprintf(buf, sizeof(buf), "%s %c ", num_buf, pending_op);
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        clear_on_next_input = false;
        return;
    }

    // Evaluate (=)
    if (strcmp(text, "=") == 0) {
        if (pending_op != '\0') {
            int len = strlen(current);
            if (len >= 3 && current[len - 1] == ' ') return;

            double second_operand = extract_second_operand(current);
            double result = 0.0;

            switch (pending_op) {
                case '+': result = first_operand + second_operand; break;
                case '-': result = first_operand - second_operand; break;
                case '*': result = first_operand * second_operand; break;
                case '/':
                    if (second_operand == 0.0) {
                        gtk_entry_set_text(GTK_ENTRY(entry), "Error");
                        pending_op = '\0';
                        clear_on_next_input = true;
                        return;
                    }
                    result = first_operand / second_operand;
                    break;
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "%.8g", result);
            gtk_entry_set_text(GTK_ENTRY(entry), buf);
            pending_op = '\0';
            clear_on_next_input = true;
        }
        return;
    }

    // Digits and decimal point
    char buf[256];
    if (clear_on_next_input || strcmp(current, "0") == 0 || strcmp(current, "Error") == 0) {
        if (strcmp(text, ".") == 0) {
            snprintf(buf, sizeof(buf), "0.");
        } else {
            snprintf(buf, sizeof(buf), "%s", text);
        }
        clear_on_next_input = false;
    } else {
        if (strcmp(text, ".") == 0) {
            const char *last_space = strrchr(current, ' ');
            const char *check_segment = (last_space != NULL) ? last_space : current;
            if (strchr(check_segment, '.') != NULL) return;
        }
        snprintf(buf, sizeof(buf), "%s%s", current, text);
    }
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
}

static void on_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    process_action((const char *)data);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    switch (event->keyval) {
        case GDK_KEY_0: case GDK_KEY_KP_0: process_action("0"); return TRUE;
        case GDK_KEY_1: case GDK_KEY_KP_1: process_action("1"); return TRUE;
        case GDK_KEY_2: case GDK_KEY_KP_2: process_action("2"); return TRUE;
        case GDK_KEY_3: case GDK_KEY_KP_3: process_action("3"); return TRUE;
        case GDK_KEY_4: case GDK_KEY_KP_4: process_action("4"); return TRUE;
        case GDK_KEY_5: case GDK_KEY_KP_5: process_action("5"); return TRUE;
        case GDK_KEY_6: case GDK_KEY_KP_6: process_action("6"); return TRUE;
        case GDK_KEY_7: case GDK_KEY_KP_7: process_action("7"); return TRUE;
        case GDK_KEY_8: case GDK_KEY_KP_8: process_action("8"); return TRUE;
        case GDK_KEY_9: case GDK_KEY_KP_9: process_action("9"); return TRUE;

        case GDK_KEY_period: case GDK_KEY_KP_Decimal:
            process_action("."); return TRUE;

        case GDK_KEY_plus: case GDK_KEY_KP_Add:
            process_action("+"); return TRUE;
        case GDK_KEY_minus: case GDK_KEY_KP_Subtract:
            process_action("-"); return TRUE;
        case GDK_KEY_asterisk: case GDK_KEY_KP_Multiply:
            process_action("*"); return TRUE;
        case GDK_KEY_slash: case GDK_KEY_KP_Divide:
            process_action("/"); return TRUE;

        case GDK_KEY_Return: case GDK_KEY_KP_Enter: case GDK_KEY_equal:
            process_action("="); return TRUE;

        case GDK_KEY_Escape: case GDK_KEY_c: case GDK_KEY_C:
            process_action("C"); return TRUE;

        case GDK_KEY_BackSpace:
            process_action("BACKSPACE"); return TRUE;

        default:
            break;
    }

    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    apply_dark_theme();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Calculator");
    
    // Scaled dimensions by 50% (from 300x380 -> 450x570)
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 570);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(window), 16);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
    gtk_entry_set_alignment(GTK_ENTRY(entry), 1.0);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);

    const char *buttons[5][4] = {
        {"C", "", "", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "=", ""}
    };

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            const char *label = buttons[r][c];
            if (strlen(label) == 0) continue;

            GtkWidget *btn = gtk_button_new_with_label(label);
            gtk_widget_set_can_focus(btn, FALSE);
            g_signal_connect(btn, "clicked", G_CALLBACK(on_button_clicked), (gpointer)label);

            GtkStyleContext *ctx = gtk_widget_get_style_context(btn);
            if (strcmp(label, "C") == 0) {
                gtk_style_context_add_class(ctx, "clear-btn");
                gtk_grid_attach(GTK_GRID(grid), btn, 0, 0, 3, 1);
            } else if (strcmp(label, "=") == 0) {
                gtk_style_context_add_class(ctx, "op-btn");
                gtk_grid_attach(GTK_GRID(grid), btn, 2, 4, 2, 1);
            } else {
                if (strchr("+-*/", label[0]) != NULL) {
                    gtk_style_context_add_class(ctx, "op-btn");
                }
                gtk_grid_attach(GTK_GRID(grid), btn, c, r, 1, 1);
            }
        }
    }

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
