/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* select routines
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "data.h"
#include "draw.h"
#include "error.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "rats.h"
#include "misc.h"
#include "find.h"

#include <sys/types.h>
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

RCSID ("$Id$");

/* ---------------------------------------------------------------------------
 * toggles the selection of any kind of object
 * the different types are defined by search.h
 */
bool
SelectObject (void)
{
  void *ptr1, *ptr2, *ptr3;
  LayerTypePtr layer;
  int type;

  bool changed = true;

  type = SearchScreen (Crosshair.X, Crosshair.Y, SELECT_TYPES,
		       &ptr1, &ptr2, &ptr3);
  if (type == NO_TYPE || TEST_FLAG (LOCKFLAG, (PinTypePtr) ptr2))
    return (false);
  switch (type)
    {
    case VIA_TYPE:
      AddObjectToFlagUndoList (VIA_TYPE, ptr1, ptr1, ptr1);
      TOGGLE_FLAG (SELECTEDFLAG, (PinTypePtr) ptr1);
      DrawVia ((PinTypePtr) ptr1);
      break;

    case LINE_TYPE:
      {
	LineType *line = (LineTypePtr) ptr2;

	layer = (LayerTypePtr) ptr1;
	AddObjectToFlagUndoList (LINE_TYPE, ptr1, ptr2, ptr2);
	TOGGLE_FLAG (SELECTEDFLAG, line);
	DrawLine (layer, line);
	break;
      }

    case RATLINE_TYPE:
      {
	RatTypePtr rat = (RatTypePtr) ptr2;

	AddObjectToFlagUndoList (RATLINE_TYPE, ptr1, ptr1, ptr1);
	TOGGLE_FLAG (SELECTEDFLAG, rat);
	DrawRat (rat);
	break;
      }

    case ARC_TYPE:
      {
	ArcType *arc = (ArcTypePtr) ptr2;

	layer = (LayerTypePtr) ptr1;
	AddObjectToFlagUndoList (ARC_TYPE, ptr1, ptr2, ptr2);
	TOGGLE_FLAG (SELECTEDFLAG, arc);
	DrawArc (layer, arc);
	break;
      }

    case TEXT_TYPE:
      {
	TextType *text = (TextTypePtr) ptr2;

	layer = (LayerTypePtr) ptr1;
	AddObjectToFlagUndoList (TEXT_TYPE, ptr1, ptr2, ptr2);
	TOGGLE_FLAG (SELECTEDFLAG, text);
	DrawText (layer, text);
	break;
      }

    case POLYGON_TYPE:
      {
	PolygonType *poly = (PolygonTypePtr) ptr2;

	layer = (LayerTypePtr) ptr1;
	AddObjectToFlagUndoList (POLYGON_TYPE, ptr1, ptr2, ptr2);
	TOGGLE_FLAG (SELECTEDFLAG, poly);
	DrawPolygon (layer, poly);
	/* changing memory order no longer effects draw order */
	break;
      }

    case PIN_TYPE:
      AddObjectToFlagUndoList (PIN_TYPE, ptr1, ptr2, ptr2);
      TOGGLE_FLAG (SELECTEDFLAG, (PinTypePtr) ptr2);
      DrawPin ((PinTypePtr) ptr2);
      break;

    case PAD_TYPE:
      AddObjectToFlagUndoList (PAD_TYPE, ptr1, ptr2, ptr2);
      TOGGLE_FLAG (SELECTEDFLAG, (PadTypePtr) ptr2);
      DrawPad ((PadTypePtr) ptr2);
      break;

    case ELEMENTNAME_TYPE:
      {
	ElementTypePtr element = (ElementTypePtr) ptr1;

	/* select all names of the element */
	ELEMENTTEXT_LOOP (element);
	{
	  AddObjectToFlagUndoList (ELEMENTNAME_TYPE, element, text, text);
	  TOGGLE_FLAG (SELECTEDFLAG, text);
	}
	END_LOOP;
	DrawElementName (element);
	break;
      }

    case ELEMENT_TYPE:
      {
	ElementTypePtr element = (ElementTypePtr) ptr1;

	/* select all pins and names of the element */
	PIN_LOOP (element);
	{
	  AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
	  TOGGLE_FLAG (SELECTEDFLAG, pin);
	}
	END_LOOP;
	PAD_LOOP (element);
	{
	  AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
	  TOGGLE_FLAG (SELECTEDFLAG, pad);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP (element);
	{
	  AddObjectToFlagUndoList (ELEMENTNAME_TYPE, element, text, text);
	  TOGGLE_FLAG (SELECTEDFLAG, text);
	}
	END_LOOP;
	AddObjectToFlagUndoList (ELEMENT_TYPE, element, element, element);
	TOGGLE_FLAG (SELECTEDFLAG, element);
	if (PCB->ElementOn &&
	    ((TEST_FLAG (ONSOLDERFLAG, element) != 0) == SWAP_IDENT ||
	     PCB->InvisibleObjectsOn))
	  if (PCB->ElementOn)
	    {
	      DrawElementName (element);
	      DrawElementPackage (element);
	    }
	if (PCB->PinOn)
	  DrawElementPinsAndPads (element);
	break;
      }
    }
  Draw ();
  IncrementUndoSerialNumber ();
  return (changed);
}

/* ----------------------------------------------------------------------
 * selects/unselects all visible objects within the passed box
 * Flag determines if the block is to be selected or unselected
 * returns true if the state of any object has changed
 */
bool
SelectBlock (BoxTypePtr Box, bool Flag)
{
  bool changed = false;

  if (PCB->RatOn || !Flag)
    RAT_LOOP (PCB->Data);
  {
    if (LINE_IN_BOX ((LineTypePtr) line, Box) &&
	!TEST_FLAG (LOCKFLAG, line) && TEST_FLAG (SELECTEDFLAG, line) != Flag)
      {
	AddObjectToFlagUndoList (RATLINE_TYPE, line, line, line);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, line);
	if (PCB->RatOn)
	  DrawRat (line);
	changed = true;
      }
  }
  END_LOOP;

  /* check layers */
  LAYER_LOOP(PCB->Data, max_copper_layer + 2);
  {
    if (layer == & PCB->Data->SILKLAYER)
      {
	if (! (PCB->ElementOn || !Flag))
	  continue;
      }
    else if (layer == & PCB->Data->BACKSILKLAYER)
      {
	if (! (PCB->InvisibleObjectsOn || !Flag))
	  continue;
      }
    else
      if (! (layer->On || !Flag))
	continue;

    LINE_LOOP (layer);
    {
      if (LINE_IN_BOX (line, Box)
	  && !TEST_FLAG (LOCKFLAG, line)
	  && TEST_FLAG (SELECTEDFLAG, line) != Flag)
	{
	  AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
	  ASSIGN_FLAG (SELECTEDFLAG, Flag, line);
	  if (layer->On)
	    DrawLine (layer, line);
	  changed = true;
	}
    }
    END_LOOP;
    ARC_LOOP (layer);
    {
      if (ARC_IN_BOX (arc, Box)
	  && !TEST_FLAG (LOCKFLAG, arc)
	  && TEST_FLAG (SELECTEDFLAG, arc) != Flag)
	{
	  AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
	  ASSIGN_FLAG (SELECTEDFLAG, Flag, arc);
	  if (layer->On)
	    DrawArc (layer, arc);
	  changed = true;
	}
    }
    END_LOOP;
    TEXT_LOOP (layer);
    {
      if (!Flag || TEXT_IS_VISIBLE(PCB, layer, text))
	{
	  if (TEXT_IN_BOX (text, Box)
	      && !TEST_FLAG (LOCKFLAG, text)
	      && TEST_FLAG (SELECTEDFLAG, text) != Flag)
	    {
	      AddObjectToFlagUndoList (TEXT_TYPE, layer, text, text);
	      ASSIGN_FLAG (SELECTEDFLAG, Flag, text);
	      if (TEXT_IS_VISIBLE(PCB, layer, text))
		DrawText (layer, text);
	      changed = true;
	    }
	}
    }
    END_LOOP;
    POLYGON_LOOP (layer);
    {
      if (POLYGON_IN_BOX (polygon, Box)
	  && !TEST_FLAG (LOCKFLAG, polygon)
	  && TEST_FLAG (SELECTEDFLAG, polygon) != Flag)
	{
	  AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
	  ASSIGN_FLAG (SELECTEDFLAG, Flag, polygon);
	  if (layer->On)
	    DrawPolygon (layer, polygon);
	  changed = true;
	}
    }
    END_LOOP;
  }
  END_LOOP;

  /* elements */
  ELEMENT_LOOP (PCB->Data);
  {
    {
      bool gotElement = false;
      if ((PCB->ElementOn || !Flag)
	  && !TEST_FLAG (LOCKFLAG, element)
	  && ((TEST_FLAG (ONSOLDERFLAG, element) != 0) == SWAP_IDENT
	      || PCB->InvisibleObjectsOn))
	{
	  if (BOX_IN_BOX
	      (&ELEMENT_TEXT (PCB, element).BoundingBox, Box)
	      && !TEST_FLAG (LOCKFLAG, &ELEMENT_TEXT (PCB, element))
	      && TEST_FLAG (SELECTEDFLAG,
			    &ELEMENT_TEXT (PCB, element)) != Flag)
	    {
	      /* select all names of element */
	      ELEMENTTEXT_LOOP (element);
	      {
		AddObjectToFlagUndoList (ELEMENTNAME_TYPE,
					 element, text, text);
		ASSIGN_FLAG (SELECTEDFLAG, Flag, text);
	      }
	      END_LOOP;
	      if (PCB->ElementOn)
		DrawElementName (element);
	      changed = true;
	    }
	  if ((PCB->PinOn || !Flag) && ELEMENT_IN_BOX (element, Box))
	    if (TEST_FLAG (SELECTEDFLAG, element) != Flag)
	      {
		AddObjectToFlagUndoList (ELEMENT_TYPE,
					 element, element, element);
		ASSIGN_FLAG (SELECTEDFLAG, Flag, element);
		PIN_LOOP (element);
		{
		  if (TEST_FLAG (SELECTEDFLAG, pin) != Flag)
		    {
		      AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
		      ASSIGN_FLAG (SELECTEDFLAG, Flag, pin);
		      if (PCB->PinOn)
			DrawPin (pin);
		      changed = true;
		    }
		}
		END_LOOP;
		PAD_LOOP (element);
		{
		  if (TEST_FLAG (SELECTEDFLAG, pad) != Flag)
		    {
		      AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
		      ASSIGN_FLAG (SELECTEDFLAG, Flag, pad);
		      if (PCB->PinOn)
			DrawPad (pad);
		      changed = true;
		    }
		}
		END_LOOP;
		if (PCB->PinOn)
		  DrawElement (element);
		changed = true;
		gotElement = true;
	      }
	}
      if ((PCB->PinOn || !Flag) && !TEST_FLAG (LOCKFLAG, element) && !gotElement)
	{
	  PIN_LOOP (element);
	  {
	    if ((VIA_OR_PIN_IN_BOX (pin, Box)
		 && TEST_FLAG (SELECTEDFLAG, pin) != Flag))
	      {
		AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
		ASSIGN_FLAG (SELECTEDFLAG, Flag, pin);
		if (PCB->PinOn)
		  DrawPin (pin);
		changed = true;
	      }
	  }
	  END_LOOP;
	  PAD_LOOP (element);
	  {
	    if (PAD_IN_BOX (pad, Box)
		&& TEST_FLAG (SELECTEDFLAG, pad) != Flag
		&& (TEST_FLAG (ONSOLDERFLAG, pad) == SWAP_IDENT
		    || PCB->InvisibleObjectsOn
		    || !Flag))
	      {
		AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
		ASSIGN_FLAG (SELECTEDFLAG, Flag, pad);
		if (PCB->PinOn)
		  DrawPad (pad);
		changed = true;
	      }
	  }
	  END_LOOP;
	}
    }
  }
  END_LOOP;
  /* end with vias */
  if (PCB->ViaOn || !Flag)
    VIA_LOOP (PCB->Data);
  {
    if (VIA_OR_PIN_IN_BOX (via, Box)
	&& !TEST_FLAG (LOCKFLAG, via)
	&& TEST_FLAG (SELECTEDFLAG, via) != Flag)
      {
	AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, via);
	if (PCB->ViaOn)
	  DrawVia (via);
	changed = true;
      }
  }
  END_LOOP;
  if (changed)
    {
      Draw ();
      IncrementUndoSerialNumber ();
    }
  return (changed);
}

/* ----------------------------------------------------------------------
 * performs several operations on the passed object
 */
void *
ObjectOperation (ObjectFunctionTypePtr F,
		 int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
  switch (Type)
    {
    case LINE_TYPE:
      if (F->Line)
	return (F->Line ((LayerTypePtr) Ptr1, (LineTypePtr) Ptr2));
      break;

    case ARC_TYPE:
      if (F->Arc)
	return (F->Arc ((LayerTypePtr) Ptr1, (ArcTypePtr) Ptr2));
      break;

    case LINEPOINT_TYPE:
      if (F->LinePoint)
	return (F->LinePoint ((LayerTypePtr) Ptr1, (LineTypePtr) Ptr2,
			      (PointTypePtr) Ptr3));
      break;

    case TEXT_TYPE:
      if (F->Text)
	return (F->Text ((LayerTypePtr) Ptr1, (TextTypePtr) Ptr2));
      break;

    case POLYGON_TYPE:
      if (F->Polygon)
	return (F->Polygon ((LayerTypePtr) Ptr1, (PolygonTypePtr) Ptr2));
      break;

    case POLYGONPOINT_TYPE:
      if (F->Point)
	return (F->Point ((LayerTypePtr) Ptr1, (PolygonTypePtr) Ptr2,
			  (PointTypePtr) Ptr3));
      break;

    case VIA_TYPE:
      if (F->Via)
	return (F->Via ((PinTypePtr) Ptr1));
      break;

    case ELEMENT_TYPE:
      if (F->Element)
	return (F->Element ((ElementTypePtr) Ptr1));
      break;

    case PIN_TYPE:
      if (F->Pin)
	return (F->Pin ((ElementTypePtr) Ptr1, (PinTypePtr) Ptr2));
      break;

    case PAD_TYPE:
      if (F->Pad)
	return (F->Pad ((ElementTypePtr) Ptr1, (PadTypePtr) Ptr2));
      break;

    case ELEMENTNAME_TYPE:
      if (F->ElementName)
	return (F->ElementName ((ElementTypePtr) Ptr1));
      break;

    case RATLINE_TYPE:
      if (F->Rat)
	return (F->Rat ((RatTypePtr) Ptr1));
      break;
    }
  return (NULL);
}

/* ----------------------------------------------------------------------
 * performs several operations on selected objects which are also visible
 * The lowlevel procedures are passed together with additional information
 * resets the selected flag if requested
 * returns true if anything has changed
 */
bool
SelectedOperation (ObjectFunctionTypePtr F, bool Reset, int type)
{
  bool changed = false;

  /* check lines */
  if (type & LINE_TYPE && F->Line)
    VISIBLELINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, line))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
	    CLEAR_FLAG (SELECTEDFLAG, line);
	  }
	F->Line (layer, line);
	changed = true;
      }
  }
  ENDALL_LOOP;

  /* check arcs */
  if (type & ARC_TYPE && F->Arc)
    VISIBLEARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, arc))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
	    CLEAR_FLAG (SELECTEDFLAG, arc);
	  }
	F->Arc (layer, arc);
	changed = true;
      }
  }
  ENDALL_LOOP;

  /* check text */
  if (type & TEXT_TYPE && F->Text)
    ALLTEXT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, text) && TEXT_IS_VISIBLE (PCB, layer, text))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (TEXT_TYPE, layer, text, text);
	    CLEAR_FLAG (SELECTEDFLAG, text);
	  }
	F->Text (layer, text);
	changed = true;
      }
  }
  ENDALL_LOOP;

  /* check polygons */
  if (type & POLYGON_TYPE && F->Polygon)
    VISIBLEPOLYGON_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, polygon))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
	    CLEAR_FLAG (SELECTEDFLAG, polygon);
	  }
	F->Polygon (layer, polygon);
	changed = true;
      }
  }
  ENDALL_LOOP;

  /* elements silkscreen */
  if (type & ELEMENT_TYPE && PCB->ElementOn && F->Element)
    ELEMENT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, element))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (ELEMENT_TYPE, element, element, element);
	    CLEAR_FLAG (SELECTEDFLAG, element);
	  }
	F->Element (element);
	changed = true;
      }
  }
  END_LOOP;
  if (type & ELEMENTNAME_TYPE && PCB->ElementOn && F->ElementName)
    ELEMENT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, &ELEMENT_TEXT (PCB, element)))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (ELEMENTNAME_TYPE,
				     element,
				     &ELEMENT_TEXT (PCB, element),
				     &ELEMENT_TEXT (PCB, element));
	    CLEAR_FLAG (SELECTEDFLAG, &ELEMENT_TEXT (PCB, element));
	  }
	F->ElementName (element);
	changed = true;
      }
  }
  END_LOOP;

  if (type & PIN_TYPE && PCB->PinOn && F->Pin)
    ELEMENT_LOOP (PCB->Data);
  {
    PIN_LOOP (element);
    {
      if (TEST_FLAG (SELECTEDFLAG, pin))
	{
	  if (Reset)
	    {
	      AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
	      CLEAR_FLAG (SELECTEDFLAG, pin);
	    }
	  F->Pin (element, pin);
	  changed = true;
	}
    }
    END_LOOP;
  }
  END_LOOP;

  if (type & PAD_TYPE && PCB->PinOn && F->Pad)
    ELEMENT_LOOP (PCB->Data);
  {
    PAD_LOOP (element);
    {
      if (TEST_FLAG (SELECTEDFLAG, pad))
	{
	  if (Reset)
	    {
	      AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
	      CLEAR_FLAG (SELECTEDFLAG, pad);
	    }
	  F->Pad (element, pad);
	  changed = true;
	}
    }
    END_LOOP;
  }
  END_LOOP;

  /* process vias */
  if (type & VIA_TYPE && PCB->ViaOn && F->Via)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, via))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
	    CLEAR_FLAG (SELECTEDFLAG, via);
	  }
	F->Via (via);
	changed = true;
      }
  }
  END_LOOP;
  /* and rat-lines */
  if (type & RATLINE_TYPE && PCB->RatOn && F->Rat)
    RAT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (SELECTEDFLAG, line))
      {
	if (Reset)
	  {
	    AddObjectToFlagUndoList (RATLINE_TYPE, line, line, line);
	    CLEAR_FLAG (SELECTEDFLAG, line);
	  }
	F->Rat (line);
	changed = true;
      }
  }
  END_LOOP;
  if (Reset && changed)
    IncrementUndoSerialNumber ();
  return (changed);
}

/* ----------------------------------------------------------------------
 * selects/unselects all objects which were found during a connection scan
 * Flag determines if they are to be selected or unselected
 * returns true if the state of any object has changed
 *
 * text objects and elements cannot be selected by this routine
 */
bool
SelectConnection (bool Flag)
{
  bool changed = false;

  if (PCB->RatOn)
    RAT_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line))
      {
	AddObjectToFlagUndoList (RATLINE_TYPE, line, line, line);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, line);
	DrawRat (line);
	changed = true;
      }
  }
  END_LOOP;

  VISIBLELINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line) && !TEST_FLAG (LOCKFLAG, line))
      {
	AddObjectToFlagUndoList (LINE_TYPE, layer, line, line);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, line);
	DrawLine (layer, line);
	changed = true;
      }
  }
  ENDALL_LOOP;
  VISIBLEARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, arc) && !TEST_FLAG (LOCKFLAG, arc))
      {
	AddObjectToFlagUndoList (ARC_TYPE, layer, arc, arc);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, arc);
	DrawArc (layer, arc);
	changed = true;
      }
  }
  ENDALL_LOOP;
  VISIBLEPOLYGON_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, polygon) && !TEST_FLAG (LOCKFLAG, polygon))
      {
	AddObjectToFlagUndoList (POLYGON_TYPE, layer, polygon, polygon);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, polygon);
	DrawPolygon (layer, polygon);
	changed = true;
      }
  }
  ENDALL_LOOP;

  if (PCB->PinOn && PCB->ElementOn)
    {
      ALLPIN_LOOP (PCB->Data);
      {
	if (!TEST_FLAG (LOCKFLAG, element) && TEST_FLAG (FOUNDFLAG, pin))
	  {
	    AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
	    ASSIGN_FLAG (SELECTEDFLAG, Flag, pin);
	    DrawPin (pin);
	    changed = true;
	  }
      }
      ENDALL_LOOP;
      ALLPAD_LOOP (PCB->Data);
      {
	if (!TEST_FLAG (LOCKFLAG, element) && TEST_FLAG (FOUNDFLAG, pad))
	  {
	    AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
	    ASSIGN_FLAG (SELECTEDFLAG, Flag, pad);
	    DrawPad (pad);
	    changed = true;
	  }
      }
      ENDALL_LOOP;
    }

  if (PCB->ViaOn)
    VIA_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, via) && !TEST_FLAG (LOCKFLAG, via))
      {
	AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, via);
	DrawVia (via);
	changed = true;
      }
  }
  END_LOOP;
  return (changed);
}

#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
/* ---------------------------------------------------------------------------
 * selects objects as defined by Type by name;
 * it's a case insensitive match
 * returns true if any object has been selected
 */

#if defined (HAVE_REGCOMP)
static int
regexec_match_all (const  regex_t  *preg,  const  char  *string)
{
  regmatch_t match;

  if (regexec (preg, string, 1, &match, 0) != 0)
    return 0;
  if (match.rm_so != 0)
    return 0;
  if (match.rm_eo != strlen(string))
    return 0;
  return 1;
}
#endif

bool
SelectObjectByName (int Type, char *Pattern, bool Flag)
{
  bool changed = false;

#if defined(HAVE_REGCOMP)
#define	REGEXEC(arg)	(regexec_match_all(&compiled, (arg)))

  int result;
  regex_t compiled;

  /* compile the regular expression */
  result = regcomp (&compiled, Pattern, REG_EXTENDED | REG_ICASE);
  if (result)
    {
      char errorstring[128];

      regerror (result, &compiled, errorstring, 128);
      Message (_("regexp error: %s\n"), errorstring);
      regfree (&compiled);
      return (false);
    }
#else
#define	REGEXEC(arg)	(re_exec((arg)) == 1)

  char *compiled;

  /* compile the regular expression */
  if ((compiled = re_comp (Pattern)) != NULL)
    {
      Message (_("re_comp error: %s\n"), compiled);
      return (false);
    }
#endif

  /* loop over all visible objects with names */
  if (Type & TEXT_TYPE)
    ALLTEXT_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, text)
	&& TEXT_IS_VISIBLE (PCB, layer, text)
	&& text->TextString
	&& REGEXEC (text->TextString)
	&& TEST_FLAG (SELECTEDFLAG, text) != Flag)
      {
	AddObjectToFlagUndoList (TEXT_TYPE, layer, text, text);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, text);
	DrawText (layer, text);
	changed = true;
      }
  }
  ENDALL_LOOP;

  if (PCB->ElementOn && (Type & ELEMENT_TYPE))
    ELEMENT_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, element)
	&& ((TEST_FLAG (ONSOLDERFLAG, element) != 0) == SWAP_IDENT
	    || PCB->InvisibleObjectsOn)
	&& TEST_FLAG (SELECTEDFLAG, element) != Flag)
      {
	String name = ELEMENT_NAME (PCB, element);
	if (name && REGEXEC (name))
	  {
	    AddObjectToFlagUndoList (ELEMENT_TYPE, element, element, element);
	    ASSIGN_FLAG (SELECTEDFLAG, Flag, element);
	    PIN_LOOP (element);
	    {
	      AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
	      ASSIGN_FLAG (SELECTEDFLAG, Flag, pin);
	    }
	    END_LOOP;
	    PAD_LOOP (element);
	    {
	      AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
	      ASSIGN_FLAG (SELECTEDFLAG, Flag, pad);
	    }
	    END_LOOP;
	    ELEMENTTEXT_LOOP (element);
	    {
	      AddObjectToFlagUndoList (ELEMENTNAME_TYPE, element, text, text);
	      ASSIGN_FLAG (SELECTEDFLAG, Flag, text);
	    }
	    END_LOOP;
	    DrawElementName (element);
	    DrawElement (element);
	    changed = true;
	  }
      }
  }
  END_LOOP;
  if (PCB->PinOn && (Type & PIN_TYPE))
    ALLPIN_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, element)
	&& pin->Name && REGEXEC (pin->Name)
	&& TEST_FLAG (SELECTEDFLAG, pin) != Flag)
      {
	AddObjectToFlagUndoList (PIN_TYPE, element, pin, pin);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, pin);
	DrawPin (pin);
	changed = true;
      }
  }
  ENDALL_LOOP;
  if (PCB->PinOn && (Type & PAD_TYPE))
    ALLPAD_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, element)
	&& ((TEST_FLAG (ONSOLDERFLAG, pad) != 0) == SWAP_IDENT
	    || PCB->InvisibleObjectsOn)
	&& TEST_FLAG (SELECTEDFLAG, pad) != Flag)
      if (pad->Name && REGEXEC (pad->Name))
	{
	  AddObjectToFlagUndoList (PAD_TYPE, element, pad, pad);
	  ASSIGN_FLAG (SELECTEDFLAG, Flag, pad);
	  DrawPad (pad);
	  changed = true;
	}
  }
  ENDALL_LOOP;
  if (PCB->ViaOn && (Type & VIA_TYPE))
    VIA_LOOP (PCB->Data);
  {
    if (!TEST_FLAG (LOCKFLAG, via)
	&& via->Name
	&& REGEXEC (via->Name) && TEST_FLAG (SELECTEDFLAG, via) != Flag)
      {
	AddObjectToFlagUndoList (VIA_TYPE, via, via, via);
	ASSIGN_FLAG (SELECTEDFLAG, Flag, via);
	DrawVia (via);
	changed = true;
      }
  }
  END_LOOP;
  if (Type & NET_TYPE)
    {
      InitConnectionLookup ();
      changed = ResetConnections (true) || changed;

      MENU_LOOP (&PCB->NetlistLib);
      {
        Cardinal i;
        LibraryEntryType *entry;
        ConnectionType conn;

        /* Name[0] and Name[1] are special purpose, not the actual name*/
        if (menu->Name && menu->Name[0] != '\0' && menu->Name[1] != '\0' &&
            REGEXEC (menu->Name + 2))
          {
            for (i = menu->EntryN, entry = menu->Entry; i; i--, entry++)
              if (SeekPad (entry, &conn, false))
                RatFindHook (conn.type, conn.ptr1, conn.ptr2, conn.ptr2,
                             true, true);
          }
      }
      END_LOOP;

      changed = SelectConnection (Flag) || changed;
      changed = ResetConnections (false) || changed;
      FreeConnectionLookupMemory ();
    }

#if defined(HAVE_REGCOMP)
#if !defined(sgi)
  regfree (&compiled);
#endif
#endif

  if (changed)
    {
      IncrementUndoSerialNumber ();
      Draw ();
    }
  return (changed);
}
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */
