/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "global.h"
#include "hid.h"
#include "data.h"
#include "misc.h"
#include "hid.h"
#include "../hidint.h"
#include "pcb-printf.h"

#include "hid/common/draw_helpers.h"
#include "hid/common/hidnogui.h"
#include "hid/common/actions.h"
#include "hid/common/hidinit.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* This is a text-line "batch" HID, which exists for scripting and
   non-GUI needs.  */

typedef struct hid_gc_struct
{
  int nothing_interesting_here;
} hid_gc_struct;

static HID_Attribute *
batch_get_export_options (int *n_ret)
{
  return 0;
}

/* ----------------------------------------------------------------------------- */

static char *prompt = "pcb";

static int
nop (int argc, char **argv, Coord x, Coord y)
{
  return 0;
}

static int
PCBChanged (int argc, char **argv, Coord x, Coord y)
{
  if (PCB && PCB->Filename)
    {
      prompt = strrchr(PCB->Filename, '/');
      if (prompt)
	prompt ++;
      else
	prompt = PCB->Filename;
    }
  else
    prompt = "no-board";
  return 0;
}

static int
help (int argc, char **argv, Coord x, Coord y)
{
  print_actions ();
  return 0;
}

static int
info (int argc, char **argv, Coord x, Coord y)
{
  int i, j;
  int cg, sg;
  if (!PCB || !PCB->Data || !PCB->Filename)
    {
      printf("No PCB loaded.\n");
      return 0;
    }
  printf("Filename: %s\n", PCB->Filename);
  pcb_printf("Size: %ml x %ml mils, %mm x %mm mm\n",
	 PCB->MaxWidth, PCB->MaxHeight,
	 PCB->MaxWidth, PCB->MaxHeight);
  cg = GetLayerGroupNumberByNumber (component_silk_layer);
  sg = GetLayerGroupNumberByNumber (solder_silk_layer);
  for (i=0; i<MAX_LAYER; i++)
    {
      
      int lg = GetLayerGroupNumberByNumber (i);
      for (j=0; j<MAX_LAYER; j++)
	putchar(j==lg ? '#' : '-');
      printf(" %c %s\n", lg==cg ? 'c' : lg==sg ? 's' : '-',
	     PCB->Data->Layer[i].Name);
    }
  return 0;
}


HID_Action batch_action_list[] = {
  {"PCBChanged", 0, PCBChanged },
  {"RouteStylesChanged", 0, nop },
  {"NetlistChanged", 0, nop },
  {"LayersChanged", 0, nop },
  {"LibraryChanged", 0, nop },
  {"Busy", 0, nop },
  {"Help", 0, help },
  {"Info", 0, info }
};

REGISTER_ACTIONS (batch_action_list)


/* ----------------------------------------------------------------------------- */

static void
batch_do_export (HID_Attr_Val * options)
{
  int interactive;
  char line[1000];

  if (isatty (0))
    interactive = 1;
  else
    interactive = 0;

  if (interactive)
    {
      printf("Entering %s version %s batch mode.\n", PACKAGE, VERSION);
      printf("See http://pcb.gpleda.org for project information\n");
    }
  while (1)
    {
      if (interactive)
	{
	  printf("%s> ", prompt);
	  fflush(stdout);
	}
      if (fgets(line, sizeof(line)-1, stdin) == NULL)
	return;
      hid_parse_command (line);
    }
}

static void
batch_parse_arguments (int *argc, char ***argv)
{
  hid_parse_command_line (argc, argv);
}

static void
batch_invalidate_lr (int l, int r, int t, int b)
{
}

static void
batch_invalidate_all (void)
{
}

static int
batch_set_layer (const char *name, int idx, int empty)
{
  return 0;
}

static hidGC
batch_make_gc (void)
{
  return 0;
}

static void
batch_destroy_gc (hidGC gc)
{
}

static void
batch_use_mask (int use_it)
{
}

static void
batch_set_color (hidGC gc, const char *name)
{
}

static void
batch_set_line_cap (hidGC gc, EndCapStyle style)
{
}

static void
batch_set_line_width (hidGC gc, Coord width)
{
}

static void
batch_set_draw_xor (hidGC gc, int xor_set)
{
}

static void
batch_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
batch_draw_arc (hidGC gc, Coord cx, Coord cy, Coord width, Coord height,
		Angle start_angle, Angle end_angle)
{
}

static void
batch_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
batch_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
}

static void
batch_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
}

static void
batch_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void
batch_calibrate (double xval, double yval)
{
}

static int
batch_shift_is_pressed (void)
{
  return 0;
}

static int
batch_control_is_pressed (void)
{
  return 0;
}

static int
batch_mod1_is_pressed (void)
{
  return 0;
}

static void
batch_get_coords (const char *msg, Coord *x, Coord *y)
{
}

static void
batch_set_crosshair (int x, int y, int action)
{
}

static hidval
batch_add_timer (void (*func) (hidval user_data),
		 unsigned long milliseconds, hidval user_data)
{
  hidval rv;
  rv.lval = 0;
  return rv;
}

static void
batch_stop_timer (hidval timer)
{
}

hidval
batch_watch_file (int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
    hidval user_data)
{
  hidval ret;
  ret.ptr = NULL;
  return ret;
}

void
batch_unwatch_file (hidval data)
{
}

static hidval
batch_add_block_hook (void (*func) (hidval data), hidval user_data )
{
  hidval ret;
  ret.ptr = NULL;
  return ret;
}

static void
batch_stop_block_hook (hidval mlpoll)
{
}

static int
batch_attribute_dialog (HID_Attribute * attrs_,
			int n_attrs_, HID_Attr_Val * results_,
			const char *title_, const char *descr_)
{
  return 0;
}

static void
batch_show_item (void *item)
{
}

#include "dolists.h"

static HID batch_hid;

void
hid_batch_init ()
{
  memset (&batch_hid, 0, sizeof (HID));

  common_nogui_init (&batch_hid);
  common_draw_helpers_init (&batch_hid);

  batch_hid.struct_size           = sizeof (HID);
  batch_hid.name                  = "batch";
  batch_hid.description           = "Batch-mode GUI for non-interactive use.";
  batch_hid.gui                   = 1;

  batch_hid.get_export_options    = batch_get_export_options;
  batch_hid.do_export             = batch_do_export;
  batch_hid.parse_arguments       = batch_parse_arguments;
  batch_hid.invalidate_lr         = batch_invalidate_lr;
  batch_hid.invalidate_all        = batch_invalidate_all;
  batch_hid.set_layer             = batch_set_layer;
  batch_hid.make_gc               = batch_make_gc;
  batch_hid.destroy_gc            = batch_destroy_gc;
  batch_hid.use_mask              = batch_use_mask;
  batch_hid.set_color             = batch_set_color;
  batch_hid.set_line_cap          = batch_set_line_cap;
  batch_hid.set_line_width        = batch_set_line_width;
  batch_hid.set_draw_xor          = batch_set_draw_xor;
  batch_hid.draw_line             = batch_draw_line;
  batch_hid.draw_arc              = batch_draw_arc;
  batch_hid.draw_rect             = batch_draw_rect;
  batch_hid.fill_circle           = batch_fill_circle;
  batch_hid.fill_polygon          = batch_fill_polygon;
  batch_hid.fill_rect             = batch_fill_rect;
  batch_hid.calibrate             = batch_calibrate;
  batch_hid.shift_is_pressed      = batch_shift_is_pressed;
  batch_hid.control_is_pressed    = batch_control_is_pressed;
  batch_hid.mod1_is_pressed       = batch_mod1_is_pressed;
  batch_hid.get_coords            = batch_get_coords;
  batch_hid.set_crosshair         = batch_set_crosshair;
  batch_hid.add_timer             = batch_add_timer;
  batch_hid.stop_timer            = batch_stop_timer;
  batch_hid.watch_file            = batch_watch_file;
  batch_hid.unwatch_file          = batch_unwatch_file;
  batch_hid.add_block_hook        = batch_add_block_hook;
  batch_hid.stop_block_hook       = batch_stop_block_hook;
  batch_hid.attribute_dialog      = batch_attribute_dialog;
  batch_hid.show_item             = batch_show_item;

  hid_register_hid (&batch_hid);
#include "batch_lists.h"
}
