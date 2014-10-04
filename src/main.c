#include "pebble.h"

typedef struct {
 char *name;
 uint32_t res;
} Meme;

Meme meme_list[] = {
  { .name = "Cage", .res = RESOURCE_ID_MEME_YOU_DONT_SAY },
  { .name = "Reasons", .res = RESOURCE_ID_MEME_REASONS },
  { .name = "Cuteness", .res = RESOURCE_ID_MEME_CUTENESS },
  { .name = "Deal with it", .res = RESOURCE_ID_MEME_DEAL },
  { .name = "Doge", .res = RESOURCE_ID_MEME_DOGE }
};
#define NUM_MEMES sizeof(meme_list) / sizeof(Meme)

/*
 * Global and static UI elements.
 *
 * This app uses two window:
 *  - the main_window which uses a menu_layer to display a list of memes;
 *  - the meme_window.
 *
 * The meme window uses a Bitmaplayer
 */
static Window *main_window;
static Window *meme_window;
static MenuLayer *menu_layer;
static BitmapLayer *meme_layer;
static GBitmap *meme_bitmap;

/* Store the index of the currently selected meme */
static int current_meme = 0;

/** Menu layer callbacks **/

/*
 * Return the number of rows in the menu: one for each meme.
 */
static uint16_t get_num_rows(struct MenuLayer* menu_layer, uint16_t section_index, void *callback_context) {
  return NUM_MEMES;
}

/*
 * Draw one row (ie: one meme) using the convenience function menu_cell_basic_draw()
 */
static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  Meme *memes = (Meme*) callback_context;
  Meme *meme = &memes[cell_index->row];

  menu_cell_title_draw(ctx, cell_layer, meme->name);
}

/*
 * When a row is selected, update the currently selected row and push the meme window to the front.
 */
static void select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  current_meme = cell_index->row;
  window_set_fullscreen(meme_window, 1);
  window_stack_push(meme_window, true);
}

/* Group our callbacks in a MenuLayerCallbacks struct that we will pass to menu_layer_set_callbacks() */
MenuLayerCallbacks menu_callbacks = {
  .get_num_rows = get_num_rows,
  .draw_row = draw_row,
  .select_click = select_click
};

/*
 * Main window load callback
 */
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  menu_layer = menu_layer_create(layer_get_bounds(window_layer));
  menu_layer_set_callbacks(menu_layer, meme_list, menu_callbacks);
  menu_layer_set_click_config_onto_window(menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

/*
 * Called when the main window is unloaded.
 */
static void main_window_unload(Window *window) {
  menu_layer_destroy(menu_layer);
}

/*
 * Update the meme window with the currently selected meme.
 *
 * This is called:
 *  - When the meme_window is initialized
 *  - When the user presses up/down/select in the meme window
 */
static void show_selected_meme() {
  Meme *meme = &meme_list[current_meme];
  meme_bitmap = gbitmap_create_with_resource(meme->res);
  bitmap_layer_set_bitmap(meme_layer, meme_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(meme_layer));
}

static void select_previous_meme_handler(ClickRecognizerRef recognizer, void *context) {
  current_meme--;
  if (current_meme < 0) {
    current_meme = NUM_MEMES - 1;
  }
  MenuIndex idx = menu_layer_get_selected_index(menu_layer);
  idx.row = current_meme;
  menu_layer_set_selected_index(menu_layer, idx, MenuRowAlignCenter, false);
  show_selected_meme();
}

static void select_next_meme_handler(ClickRecognizerRef recognizer, void *context) {
  current_meme++;
  if ((unsigned)current_meme >= NUM_MEMES) {
    current_meme = 0;
  }
  MenuIndex idx = menu_layer_get_selected_index(menu_layer);
  idx.row = current_meme;
  menu_layer_set_selected_index(menu_layer, idx, MenuRowAlignCenter, false);
  show_selected_meme();
}

/*
 * Provides the click configuration for the meme_window
 */
static void meme_window_config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)cycle_text_handler);
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)select_previous_meme_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)select_next_meme_handler);
}

/*
 * Called every time the meme window is loaded (pushed to front) to setup its layers.
 */
static void meme_window_load(Window *window) {
  //window_set_fullscreen(window, 1);
  Layer *window_layer = window_get_root_layer(window);
  meme_layer = bitmap_layer_create(layer_get_bounds(window_layer));
  window_set_click_config_provider(window, (ClickConfigProvider) meme_window_config_provider);
  layer_add_child(window_layer, bitmap_layer_get_layer(meme_layer));
  // Finally, update the bitmap in the layers
   show_selected_meme();
}

/*
 * Called when the meme window is unloaded.
 *
 * We need to remove the child layers here, or we would add them twice when
 * the window is reloaded (that would be bad!).
 */
static void meme_window_unload(Window *window) {
  gbitmap_destroy(meme_bitmap);
  bitmap_layer_destroy(meme_layer);
}

/*
 * Declare our handlers for both window
 */
static WindowHandlers main_window_handlers = {
  .load = main_window_load,
  .unload = main_window_unload
};
static WindowHandlers meme_window_handlers = {
  .load = meme_window_load,
  .unload = meme_window_unload
};

/*
 * Initialize our app and both windows
 */
static void init() {
  current_meme = 0;
//   current_message = 0;

  main_window = window_create();
  window_set_window_handlers(main_window, main_window_handlers);

  meme_window = window_create();
  window_set_window_handlers(meme_window, meme_window_handlers);

  window_stack_push(main_window, true /* Animated */);
}

/*
 * Deinitialize both windows
 */
static void deinit() {
  window_destroy(meme_window);
  window_destroy(main_window);
}

/*
 * Our main function just calls initializers and the event loop.
 */
int main(void) {
  init();
  app_event_loop();
  deinit();
}
