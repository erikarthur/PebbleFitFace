#include <pebble.h>
//#include <ctype.h>

static Window *s_main_window;
static char s_fit_steps[10];
static char s_full_steps[10];
static char s_day_buffer[10];

static GFont s_res_roboto_bold_subset_49;
static GFont s_res_roboto_condensed_21;
static GFont s_res_bitham_34_medium_numbers;
//static BitmapLayer *s_bitmaplayer_1;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_fit_layer;

static int num_steps = 0;
static double full_steps = 6000.0;
  
static Layer *s_pb_layer;

// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	STEP_KEY = 1,
  GOAL_KEY = 2
};

int isspace(int c)
{
    if(((char)c)==' ')
        return 1;
    return 0;    
}

int isdigit(int c)
{
    char cc = (char)c;
    if(cc>='0' && cc<='9')
        return 1;    
    return 0;    
}

double atof(const char *nptr)
{
    return (strtod(nptr, (char **)NULL));
}
 
double strtod(const char *nptr, char **endptr)
{
    double x = 0.0;
    double xs= 1.0;
    double es = 1.0;
    double xf = 0.0;
    double xd = 1.0;
    while( isspace( (unsigned char)*nptr ) ) ++nptr;
    if(*nptr == '-')
    {
        xs = -1;
        nptr++;
    }
    else if(*nptr == '+')
    {
        nptr++;
    }
 
    while (1)
    {
        if (isdigit((unsigned char)*nptr))
        {
            x = x * 10 + (*nptr - '0');
            nptr++;
        }
        else
        {
            x = x * xs;
            break;
        }
    }
    if (*nptr == '.')
    {
        nptr++;
        while (1)
        {
            if (isdigit((unsigned char)*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                xd = xd * 10;
            }
            else
            {
                x = x + xs * (xf / xd);
                break;
            }
            nptr++;
        }
    }
    if ((*nptr == 'e') || (*nptr == 'E'))
    {
        nptr++;
        if (*nptr == '-')
        {
            es = -1;
            nptr++;
        }
        xd = 1;
        xf = 0;
        while (1)
        {
            if (isdigit((unsigned char)*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                nptr++;
            }
            else
            {
                while (xf > 0)
                {
                    xd *= 10;
                    xf--;
                }
                if (es < 0.0)
                {
                    x = x / xd;
                }
                else
                {
                    x = x * xd;
                }
                break;
            }
        }
    }
    if (endptr != NULL)
    {
        *endptr = (char *)nptr;
    }
    return (x);
}

static void pb_update_proc(Layer *layer, GContext *ctx) {
  
  // dot in the middle
  double perComplete = num_steps / full_steps;
  if (perComplete > 1.0f)
    perComplete = 1.0f;
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Percent complete %f", perComplete);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Percent complete %d", num_steps);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Percent complete %f", full_steps);
  
  int offset = (int)(perComplete * 120);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(12, 123, 120, 4), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(12 + offset, 120, 9, 9), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  
  #ifdef PBL_COLOR
    window_set_background_color(s_main_window, GColorOxfordBlue);
  #else
    window_set_background_color(s_main_window, GColorBlack);
  #endif
  
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_main_window, true);
  #endif
  
  s_res_roboto_bold_subset_49 = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_res_roboto_condensed_21 = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
  s_res_bitham_34_medium_numbers = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  
  
  s_time_layer = text_layer_create(GRect(0, 44, 144, 55));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "0:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, s_res_roboto_bold_subset_49);
  layer_add_child(window_get_root_layer(s_main_window), (Layer *)s_time_layer);
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(s_day_buffer, sizeof(s_day_buffer), "%a, %d", t);
  s_date_layer = text_layer_create(GRect(6, 31, 100, 24));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, s_day_buffer);
  text_layer_set_font(s_date_layer, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_main_window), (Layer *)s_date_layer);
  
  s_fit_layer = text_layer_create(GRect(0, 127, 144, 49));
  text_layer_set_background_color(s_fit_layer, GColorClear);
  text_layer_set_text_color(s_fit_layer, GColorWhite);
  text_layer_set_text(s_fit_layer, "0");
  text_layer_set_text_alignment(s_fit_layer, GTextAlignmentCenter);
  text_layer_set_font(s_fit_layer, s_res_bitham_34_medium_numbers);
  layer_add_child(window_get_root_layer(s_main_window), (Layer *)s_fit_layer);
  
  
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  s_pb_layer = layer_create(bounds);
  layer_set_update_proc(s_pb_layer, pb_update_proc);
  layer_add_child(window_layer, s_pb_layer);

}

static void main_window_unload(Window *window) {
  layer_destroy(s_pb_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_fit_layer);
  text_layer_destroy(s_date_layer);
  
  
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  
  strftime(s_day_buffer, sizeof(s_day_buffer), "%a, %d", tick_time);
  text_layer_set_text(s_date_layer, s_day_buffer);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "buffer time is: %s", buffer);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  //text_layer_set_text(s_time_layer, buffer);
  //text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STEP_KEY);
	if(tuple) {
    strncpy(s_fit_steps, tuple->value->cstring, 10);
    num_steps = atoi(s_fit_steps);
    text_layer_set_text(s_fit_layer, s_fit_steps);
	}
  tuple = dict_find(received, GOAL_KEY);
	if(tuple) {
    strncpy(s_full_steps, tuple->value->cstring, 10);
    full_steps = atof(s_full_steps);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Full Steps %d", full_steps);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "SFull Steps %s", s_full_steps);
	}
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Full Steps %d", full_steps);
}



// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

static void init() {
// Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time();
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