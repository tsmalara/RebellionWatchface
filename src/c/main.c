#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static BitmapLayer *rebel_image_layer;
static GBitmap *rebel_image;
static TextLayer *s_time_layer, *s_step_layer;

static char s_current_time_buffer[8], s_current_steps_buffer[16];
static int s_step_count = 0, s_step_goal = 0, s_step_average = 0;

// Health Data Start
bool step_data_is_available() {
  return HealthServiceAccessibilityMaskAvailable &
    health_service_metric_accessible(HealthMetricStepCount,
      time_start_of_today(), time(NULL));
}

// Daily step goal
static void get_step_goal() {
  const time_t start = time_start_of_today();
  const time_t end = start + SECONDS_PER_DAY;
  s_step_goal = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

// Todays current step count
static void get_step_count() {
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
}

// Average daily step count for this time of day
static void get_step_average() {
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  s_step_average = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

static void display_step_count() {
  int thousands = s_step_count / 1000;
  int decimalplaces = s_step_count % 1000 / 100;
  int hundreds = s_step_count % 1000;
  static char s_emoji[5];
  
  if(s_step_count >= s_step_average) {
    text_layer_set_text_color(s_step_layer, GColorIslamicGreen);
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F60C");
  } else {
    text_layer_set_text_color(s_step_layer, GColorVividCerulean);
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F4A9");
  }
  
  if(thousands > 0) {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d.%01dk", thousands, decimalplaces);
  } else {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d", hundreds);   
  }

  text_layer_set_text(s_step_layer, s_current_steps_buffer);
}

static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventSignificantUpdate) {
    get_step_goal();
  }
  if(event != HealthEventSleepUpdate) {
    get_step_count();
    get_step_average();
    display_step_count();
  }
}
// Health Data End

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
    
  // Write the current date into a buffer
  static char s_buffer_date[8]; 
  strftime(s_buffer_date, sizeof("dd/MMM/yy"), PBL_IF_ROUND_ELSE("%d", "%b %d"), tick_time);
  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_buffer_date);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set Window background color
  window_set_background_color(s_main_window, GColorBlack);

  //START IMAGE
  rebel_image_layer = bitmap_layer_create(bounds);
  rebel_image = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_IMAGE);
	
  bitmap_layer_set_compositing_mode(rebel_image_layer, GCompOpAssign);
  bitmap_layer_set_bitmap(rebel_image_layer, rebel_image);
  bitmap_layer_set_alignment(rebel_image_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(rebel_image_layer));
  //END IMAGE
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(95, 90), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create date DateLayer
  s_date_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(-30,0), PBL_IF_ROUND_ELSE(60, 0), bounds.size.w, 50));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorLightGray);
  
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter,GTextAlignmentLeft));
    
  //Add date as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  // Create a layer to hold the current step count
  s_step_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(28,0), PBL_IF_ROUND_ELSE(60, 138), bounds.size.w, 38));
  text_layer_set_background_color(s_step_layer, GColorClear);
  text_layer_set_font(s_step_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_step_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter,GTextAlignmentRight));
  layer_add_child(window_layer, text_layer_get_layer(s_step_layer));

  // Subscribe to health events if we can
  if(step_data_is_available()) {
    health_service_events_subscribe(health_handler, NULL);
  }
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  //Destroy DateLayer
  text_layer_destroy(s_date_layer);
  //Destroy ImageLayer
  gbitmap_destroy(rebel_image);
  bitmap_layer_destroy(rebel_image_layer);
  //Destroy Step Layer
  layer_destroy(text_layer_get_layer(s_step_layer));
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}