/*
**! module Image
**! note
**!	$Id: layers.c,v 1.5 1999/04/19 18:45:09 mirar Exp $
**! class Layer
*/

#include "global.h"
#include <config.h>

RCSID("$Id: layers.c,v 1.5 1999/04/19 18:45:09 mirar Exp $");

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "threads.h"
#include "builtin_functions.h"
#include "dmalloc.h"
#include "operators.h"
#include "module_support.h"
#include "opcodes.h"

#include "image.h"

extern struct program *image_program;
struct program *image_layer_program;

static struct mapping *colors=NULL;
static struct object *colortable=NULL;
static struct array *colornames=NULL;

struct program *image_layer_program=NULL;
extern struct program *image_colortable_program;

static const rgb_group black={0,0,0};
static const rgb_group white={COLORMAX,COLORMAX,COLORMAX};

typedef void lm_row_func(rgb_group *s,
			 rgb_group *l,
			 rgb_group *d,
			 rgb_group *sa,
			 rgb_group *la, /* may be NULL */
			 rgb_group *da,
			 int len,
			 float alpha);
                    

#define SNUMPIXS 64 /* pixels in short-stroke buffer */

struct layer
{
   int xsize;            /* underlaying image size */
   int ysize;	         
		         
   int xoffs,yoffs;      /* clip offset */

   struct object *image; /* image object */
   struct object *alpha; /* alpha object or null */

   struct image *img;    /* image object storage */
   struct image *alp;    /* alpha object storage */

   float alpha_value;    /* overall alpha value (1.0=opaque) */

   rgb_group fill;       /* fill color ("outside" the layer) */
   rgb_group fill_alpha; /* fill alpha */ 

   rgb_group sfill[SNUMPIXS];       /* pre-calculated rows */
   rgb_group sfill_alpha[SNUMPIXS]; 

   int tiled;            /* true if tiled */

   lm_row_func *row_func;/* layer mode */
   int optimize_alpha;
};

#define THIS ((struct layer *)(fp->current_storage))
#define THISOBJ (fp->current_object)

static void lm_normal(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha);
static void lm_dissolve(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,float alpha);
static void lm_behind(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha);
static void lm_multiply(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,float alpha);
static void lm_screen(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha);
static void lm_overlay(rgb_group *s,rgb_group *l,rgb_group *d,
		       rgb_group *sa,rgb_group *la,rgb_group *da,
		       int len,float alpha);
static void lm_difference(rgb_group *s,rgb_group *l,rgb_group *d,
			  rgb_group *sa,rgb_group *la,rgb_group *da,
			  int len,float alpha);
static void lm_add(rgb_group *s,rgb_group *l,rgb_group *d,
		   rgb_group *sa,rgb_group *la,rgb_group *da,
		   int len,float alpha);
static void lm_subtract(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,float alpha);
static void lm_darken(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha);
static void lm_lighten(rgb_group *s,rgb_group *l,rgb_group *d,
		       rgb_group *sa,rgb_group *la,rgb_group *da,
		       int len,float alpha);
static void lm_hue(rgb_group *s,rgb_group *l,rgb_group *d,
		   rgb_group *sa,rgb_group *la,rgb_group *da,
		   int len,float alpha);
static void lm_saturation(rgb_group *s,rgb_group *l,rgb_group *d,
			  rgb_group *sa,rgb_group *la,rgb_group *da,
			  int len,float alpha);
static void lm_color(rgb_group *s,rgb_group *l,rgb_group *d,
		     rgb_group *sa,rgb_group *la,rgb_group *da,
		     int len,float alpha);
static void lm_value(rgb_group *s,rgb_group *l,rgb_group *d,
		     rgb_group *sa,rgb_group *la,rgb_group *da,
		     int len,float alpha);
static void lm_divide(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha);
static void lm_erase(rgb_group *s,rgb_group *l,rgb_group *d,
		     rgb_group *sa,rgb_group *la,rgb_group *da,
		     int len,float alpha);
static void lm_replace(rgb_group *s,rgb_group *l,rgb_group *d,
		       rgb_group *sa,rgb_group *la,rgb_group *da,
		       int len,float alpha);

struct layer_mode_desc
{
   char *name;
   lm_row_func *func;
   int optimize_alpha; /* alpha 0 -> skip layer */
   struct pike_string *ps;
} layer_mode[]=
{
   {"normal",        lm_normal,        1, NULL            },
   {"add",           lm_add,           1, NULL            }, 
   {"subtract",      lm_subtract,      1, NULL            }, 
   {"dissolve",      lm_dissolve,      1, NULL            }, 
/* {"behind",        lm_behind,        1, NULL            }, */
/* {"multiply",      lm_multiply,      1, NULL            }, */
/* {"screen",        lm_screen,        1, NULL            }, */
/* {"overlay",       lm_overlay,       1, NULL            }, */
/* {"difference",    lm_difference,    1, NULL            }, */
/* {"darken",        lm_darken,        1, NULL            }, */
/* {"lighten",       lm_lighten,       1, NULL            }, */
/* {"hue",           lm_hue,           1, NULL            }, */
/* {"saturation",    lm_saturation,    1, NULL            }, */
/* {"color",         lm_color,         1, NULL            }, */
/* {"value",         lm_value,         1, NULL            }, */
/* {"divide",        lm_divide,        1, NULL            }, */
/* {"erase",         lm_erase,         1, NULL            }, */
/* {"replace",       lm_replace,       1, NULL            }, */
} ;

#define LAYER_MODES ((int)NELEM(layer_mode))

/*

Px=pixel, Ax=alpha [0-1], Rx,Gx,Bx, Hx,Sx,Vx = kanaler
Xs=k�llbild
Xl=aktuellt lager
Xd=m�lbild

normal 		Pd=(Pl*Al+Ps*(1-Al)*As)/(Al+(1-Al)*As)
		Ad=(Al+(1-Al)*As)

add             Pd=Pl*Al+Ps*As
		Ad=(Al+(1-Al)*As)

subtract        Pd=Ps-Pl*Al
		Ad=(Al+(1-Al)*As)

multiply        Pd=Ps*Pl
		Ad=(Al+(1-Al)*As)

divide          Pd=Ps/Pl
		Ad=As

dissolve        i=round(random(Al)) typ
                Pd=Al*i+As*(1-i)
		Ad=i+As

behind          
screen          
overlay         
difference      
darken          
lighten         
hue             
saturation      
color           
value           
erase           
replace         

*/



/*** layer helpers ****************************************/

static INLINE void smear_color(rgb_group *d,rgb_group s,int len)
{
   while (len--)
      *(d++)=s;
}

/*** layer object : init and exit *************************/

static void init_layer(struct object *dummy)
{
   THIS->xsize=0;
   THIS->ysize=0;
   THIS->xoffs=0;
   THIS->yoffs=0;
   THIS->image=NULL;
   THIS->alpha=NULL;
   THIS->img=NULL;
   THIS->alp=NULL;
   THIS->fill=black;
   THIS->fill_alpha=black;
   THIS->tiled=0;
   THIS->alpha_value=1.0;
   THIS->row_func=lm_normal;
   THIS->optimize_alpha=1;

   smear_color(THIS->sfill,THIS->fill,SNUMPIXS);
   smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);
}

static void free_layer(struct layer *l)
{
   if (THIS->image) free_object(THIS->image);
   if (THIS->alpha) free_object(THIS->alpha);
   THIS->image=NULL;
   THIS->alpha=NULL;
   THIS->img=NULL;
   THIS->alp=NULL;
}

static void exit_layer(struct object *dummy)
{
   free_layer(THIS);
}

/*
**! method object set_image(object(Image.Image)	image)
**! method object set_image(object(Image.Image)	image,object(Image.Image) alpha_channel)
**! method object|int(0) image()
**! method object|int(0) alpha()
**!	Set/change/get image and alpha channel for the layer.
**!	You could also cancel the channels giving 0 
**!	instead of an image object.
**! note:
**!	image and alpha channel must be of the same size,
**!	or canceled.
*/

static void image_layer_set_image(INT32 args)
{
   struct image *img;

   if (THIS->image)
      free_object(THIS->image);
   THIS->image=NULL;
   THIS->img=NULL;

   if (THIS->alpha)
      free_object(THIS->alpha);
   THIS->alpha=NULL;
   THIS->alp=NULL;

   if (args>=1)
      if ( sp[-args].type!=T_OBJECT )
      {
	 if (sp[-args].type!=T_INT || 
	     sp[-args].u.integer!=0)
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",1,
				 "object(Image)|int(0)");
      }
      else if ((img=(struct image*)
		get_storage(sp[-args].u.object,image_program)))
      {
	 THIS->image=sp[-args].u.object;
	 add_ref(THIS->image);
	 THIS->img=img;
	 THIS->xsize=img->xsize;
	 THIS->ysize=img->ysize;
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",1,
			      "object(Image)|int(0)");

   if (args>=2)
      if ( sp[1-args].type!=T_OBJECT )
      {
	 if (sp[1-args].type!=T_INT || 
	     sp[1-args].u.integer!=0)
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
				 "object(Image)|int(0)");
      }
      else if ((img=(struct image*)
		get_storage(sp[1-args].u.object,image_program)))
      {
	 if (THIS->img &&
	     (img->xsize!=THIS->xsize ||
	      img->ysize!=THIS->ysize))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
				 "image of same size");
	 if (!THIS->img)
	 {
	    THIS->xsize=img->xsize;
	    THIS->ysize=img->ysize;
	 }
	 THIS->alpha=sp[1-args].u.object;
	 add_ref(THIS->alpha);
	 THIS->alp=img;
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
			      "object(Image)|int(0)");

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_image(INT32 args)
{
   pop_n_elems(args);
   if (THIS->image)
      ref_push_object(THIS->image);
   else
      push_int(0);
}


static void image_layer_alpha(INT32 args)
{
   pop_n_elems(args);
   if (THIS->alpha)
      ref_push_object(THIS->alpha);
   else
      push_int(0);
}

/*
**! method object set_alpha_value(float value)
**! method float alpha_value()
**!	Set/get the general alpha value of this layer.
**!	This is a float value between 0 and 1,
**!	and is multiplied with the alpha channel.
*/

static void image_layer_set_alpha_value(INT32 args)
{
   float f;
   get_all_args("Image.Layer->set_alpha_value",args,"%F",&f);
   if (f<0.0 || f>=1.0)
      SIMPLE_BAD_ARG_ERROR("Image.Layer->set_alpha_value",1,"float(0..1)");
   THIS->alpha_value=f;
}

static void image_layer_alpha_value(INT32 args)
{
   pop_n_elems(args);
   push_float(THIS->alpha_value);
}

/*
**! method object set_mode(string mode)
**! method string mode()
**! method array(string) available_modes()
**!	Set/get layer mode. Mode is one of these:
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>normal</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>addition</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>behind</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>color</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>darken</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>difference</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>dissolve</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>divide</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>erase</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>hue</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>lighten</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>multiply</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>overlay</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>replace</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>saturation</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>screen</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>subtract</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!
**!	<tr><td valign=top align=left>
**!	<b><tt>value</tt><b></br>
**!	?
**!	</td><td align=left valign=top>
**!	<illustration> return lena(); </illustration>
**!	<td></tr>
**!	
**!	<ref>available_modes</ref>() simply gives an array 
**!	containing the names of these modes.
**!
**! note:
**!	image and alpha channel must be of the same size,
**!	or canceled.
*/

static void image_layer_set_mode(INT32 args)
{
   int i;
   if (args!=1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->set_mode",1);
   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("Image.Layer->set_mode",1,"string");
      
   for (i=0; i<LAYER_MODES; i++)
      if (sp[-args].u.string==layer_mode[i].ps)
      {
	 THIS->row_func=layer_mode[i].func;
	 THIS->optimize_alpha=layer_mode[i].optimize_alpha;
	 pop_n_elems(args);
	 ref_push_object(THISOBJ);
	 return;
      }

   SIMPLE_BAD_ARG_ERROR("Image.Layer->set_mode",1,"existing mode");
}

static void image_layer_mode(INT32 args)
{
   int i;
   pop_n_elems(args);
      
   for (i=0; i<LAYER_MODES; i++)
      if (THIS->row_func==layer_mode[i].func)
      {
	 ref_push_string(layer_mode[i].ps);
	 return;
      }

   fatal("illegal mode: %p\n",layer_mode[i].func);
}

static void image_layer_available_modes(INT32 args)
{
   int i;
   pop_n_elems(args);
      
   for (i=0; i<LAYER_MODES; i++)
      ref_push_string(layer_mode[i].ps);

   f_aggregate(LAYER_MODES);
}

/*
**! method object set_fill(Color color)
**! method object set_fill(Color color,Color alpha)
**! method object fill()
**! method object fill_alpha()
**!	Set/query fill color and alpha, ie the color used "outside" the 
**!	image. This is mostly useful if you want to "frame"
**!	a layer.
*/

static void image_layer_set_fill(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->set_fill",1);

   if (sp[-args].type==T_INT && !sp[-args].u.integer)
      THIS->fill=black;
   else
      if (!image_color_arg(-args,&(THIS->fill)))
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_fill",1,"color");

   smear_color(THIS->sfill,THIS->fill,SNUMPIXS);

   THIS->fill_alpha=white;
   if (args>1)
      if (sp[1-args].type==T_INT && !sp[1-args].u.integer)
	 ; /* white is good */
      else
	 if (!image_color_arg(1-args,&(THIS->fill_alpha)))
	 {
	    smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_fill",2,"color");
	 }
   smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);

#if 0
   {
      int i;
      for (i=0; i<SNUMPIXS; i++)
	 fprintf(stderr,"#%02x%02x%02x ",THIS->sfill_alpha[i].r,THIS->sfill_alpha[i].g,THIS->sfill_alpha[i].b);
      fprintf(stderr,"\n");
      for (i=0; i<SNUMPIXS; i++)
	 fprintf("stderr,#%02x%02x%02x ",THIS->sfill[i].r,THIS->sfill[i].g,THIS->sfill[i].b);
      fprintf(stderr,"\n");
   }
#endif

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_fill(INT32 args)
{
   pop_n_elems(args);
   _image_make_rgb_color(THIS->fill.r,THIS->fill.g,THIS->fill.b);
}

static void image_layer_fill_alpha(INT32 args)
{
   pop_n_elems(args);
   _image_make_rgb_color(THIS->fill_alpha.r,
			 THIS->fill_alpha.g,
			 THIS->fill_alpha.b);
}

/*
**! method object set_offset(int x,int y)
**! method int xoffset()
**! method int yoffset()
**!	Set/query layer offset.
*/

static void image_layer_set_offset(INT32 args)
{
   get_all_args("Image.Layer->set_offset",args,"%i%i",
		&(THIS->xoffs),&(THIS->yoffs));
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_xoffset(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->xoffs);
}

static void image_layer_yoffset(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->yoffs);
}

/*
**! method object set_tiled(int yes)
**! method int tiled()
**!	Set/query <i>tiled</i> flag. If set, the
**!	image and alpha channel will be tiled rather
**!	then framed by the <ref>fill</ref> values.
*/

static void image_layer_set_tiled(INT32 args)
{
   get_all_args("Image.Layer->set_offset",args,"%i",
		&(THIS->tiled));
   THIS->tiled=!!THIS->tiled;
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_tiled(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->tiled);
}

/*
**! method void create(object image,object alpha,string mode)
**! method void create(mapping info)
**! method void create()
**!
**! method void create(int xsize,int ysize,object color)
**! method void create(object color)
**!	The Layer construct either three arguments,
**!	the image object, alpha channel and mode, or
**!	a mapping with optional elements:
**!	<pre>
**!	"image":image,            
**!		// default: black
**!
**!	"alpha":alpha,            
**!		// alpha channel object
**!		// default: full opaque
**!
**!	"mode":string mode,
**!		// layer mode, see <ref>mode</ref>.
**!		// default: "normal"
**!
**!	"alpha_value":float(0.0-1.0),
**!		// layer general alpha value
**!		// default is 1.0; this is multiplied
**!		// with the alpha channel.
**!
**!	"xoffset":int,
**!	"yoffset":int,
**!		// offset of this layer
**!
**!	"fill":Color,		  
**!	"fill_alpha":Color,
**!		// fill color, ie what color is used
**!		// "outside" the image. default: black
**!		// and black (full transparency).
**!
**!	"tiled":int(0|1),
**!		// select tiling; if 1, the image
**!		// will be tiled. deafult: 0, off
**!	</pre>	
**!	The layer can also be created "empty",
**!	either giving a size and color -
**!	this will give a filled opaque square,
**!	or a color, which will set the "fill"
**!	values and fill the whole layer with an
**!	opaque color.
**!
**!	All values can be modified after object creation.
**!
**! note:
**!	image and alpha channel must be of the same size.
**!	
*/

static INLINE void try_parameter(char *a,void (*f)(INT32))
{
   stack_dup();
   push_text(a);
   f_index(2);

   if (!IS_UNDEFINED(sp-1))
      f(1);
   pop_stack();
}

static INLINE void try_parameter_pair(char *a,char *b,void (*f)(INT32))
{
   stack_dup();  /* map map */
   stack_dup();  /* map map map */
   push_text(a); /* map map map a */
   f_index(2);   /* map map map[a] */
   stack_swap(); /* map map[a] map */
   push_text(b); /* map map[a] map b */
   f_index(2);   /* map map[a] map[b] */

   if (!IS_UNDEFINED(sp-2) ||
       !IS_UNDEFINED(sp-1))
   {
      f(2);
      pop_stack();
   }
   else 
      pop_n_elems(2);
}

static void image_layer_create(INT32 args)
{
   if (!args)
      return;
   if (sp[-args].type==T_MAPPING)
   {
      pop_n_elems(args-1);
      try_parameter_pair("image","alpha",image_layer_set_image);

      try_parameter("mode",image_layer_set_mode);
      try_parameter("alpha_value",image_layer_set_alpha_value);

      try_parameter_pair("xoffset","yoffset",image_layer_set_offset);
      try_parameter_pair("fill","fill_alpha",image_layer_set_fill);
      try_parameter("tiled",image_layer_set_tiled);
      pop_stack();
      return;
   }
   else if (sp[-args].type==T_INT)
   {
      rgb_group col=black,alpha=white;

      get_all_args("Image.Layer",args,"%i%i",&(THIS->xsize),&(THIS->ysize));
      if (args>2)
	 if (!image_color_arg(2-args,&col))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer",3,"Image.Color");
	    
      if (args>3)
	 if (!image_color_arg(3-args,&alpha))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer",4,"Image.Color");
      
      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_int(col.r);
      push_int(col.g);
      push_int(col.b);
      push_object(clone_object(image_program,5));

      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_int(alpha.r);
      push_int(alpha.g);
      push_int(alpha.b);
      push_object(clone_object(image_program,5));
      
      image_layer_set_image(2);

      pop_n_elems(args);
   }
   else if (sp[-args].type==T_OBJECT)
   {
      if (args>2)
      {
	 image_layer_set_mode(args-2);
	 pop_stack();
	 args=2;
      }
      image_layer_set_image(args);
      pop_stack();
   }
   else
      SIMPLE_BAD_ARG_ERROR("Image.Layer",1,"mapping|int|Image.Image");
}

/*** layer object *****************************************/

static void image_layer_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->cast",1);
   if (sp[-args].type==T_STRING||sp[-args].u.string->size_shift)
   {
      if (strncmp(sp[-args].u.string->str,"mapping",7)==0)
      {
	 int n=0;
	 pop_n_elems(args);
	 
	 push_text("xsize");        push_int(THIS->xsize);         n++;
	 push_text("ysize");        push_int(THIS->ysize);         n++;
	 push_text("image");        image_layer_image(0);          n++;
	 push_text("alpha");        image_layer_image(0);          n++;
	 push_text("xoffset");      push_int(THIS->xoffs);         n++;
	 push_text("yoffset");      push_int(THIS->yoffs);         n++;
	 push_text("alpha_value");  push_float(THIS->alpha_value); n++;
	 push_text("fill");         image_layer_fill(0);           n++;
	 push_text("fill_alpha");   image_layer_fill_alpha(0);     n++;
	 push_text("tiled");        push_int(THIS->tiled);         n++;
	 push_text("mode");         image_layer_mode(0);           n++;

	 f_aggregate_mapping(n*2);

	 return;
      }
   }
   SIMPLE_BAD_ARG_ERROR("Image.Colortable->cast",1,
			"string(\"mapping\"|\"array\"|\"string\")");

}

/*** layer helpers ************************************/

#define COMBINE_METHOD_INT
#define CCUT_METHOD_INT

#ifdef CCUT_METHOD_FLOAT
#define qMAX (1.0/COLORMAX)
#define CCUT(Z) ((COLORTYPE)(qMAX*Z))
#else /* CCUT_METHOD_INT */
#define CCUT(Z) ((COLORTYPE)((Z)/COLORMAX))
#endif

#ifdef COMBINE_METHOD_INT


#define COMBINE_ALPHA_SUM(aS,aL) \
    CCUT((COLORMAX*(int)(aL))+(COLORMAX-(int)(aL))*(aS))
#define COMBINE_ALPHA_SUM_V(aS,aL,V) \
    COMBINE_ALPHA_SUM(aS,(aL)*(V))

#define COMBINE_ALPHA(S,L,aS,aL) \
    ( (COLORTYPE)((((S)*((int)(COLORMAX-(aL)))*(aS))+ \
	           ((L)*((int)(aL))*COLORMAX))/ \
    	          (((COLORMAX*(int)(aL))+(COLORMAX-(int)(aL))*(aS))) ) )

#define COMBINE_ALPHA_V(S,L,aS,aL,V) \
    COMBINE_ALPHA(S,(int)((L)*(V)),aS,aL)

#else 
#ifdef COMBINE_METHOD_FLOAT

#define C2F(Z) (qMAX*(Z))

#define COMBINE_ALPHA(S,L,aS,aL) \
    ( (COLORTYPE)( ( (S)*(1.0-C2F(aL))*C2F(aS) + (L)*C2F(aL) ) / \
	       ( (C2F(aL)+(1-C2F(aL))*C2F(aS))) ) )

#define COMBINE_ALPHA_V(S,L,aS,aL,V) \
    COMBINE_ALPHA(S,(L)*(V),aS,aL)

#define COMBINE_ALPHA_SUM(aS,aL) \
    ((COLORTYPE)(COLORMAX*(C2F(aL)+(1.0-C2F(aL))*C2F(aS))))
#define COMBINE_ALPHA_SUM_V(aS,aL,V) \
    COMBINE_ALPHA_SUM(aS,(aL)*(V))

#else /* unknown COMBINE_METHOD */
#error unknown COMBINE_METHOD
#endif /* COMBINE_METHOD_FLOAT  */

#endif 

#define COMBINE(P,A) CCUT(((int)(P))*(A))
#define COMBINE_A(P,A) ((COLORTYPE)((P)*(A)))
#define COMBINE_V(P,V,A) CCUT((V)*(P)*(A))

/*** layer mode definitions ***************************/

static void lm_normal(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,float alpha)
{
   /* la may be NULL, no other */

   if (alpha==0.0) /* optimized */
   {
      MEMCPY(s,d,sizeof(rgb_group)*len);
      MEMCPY(sa,da,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
      {
	 MEMCPY(d,l,sizeof(rgb_group)*len);
	 smear_color(da,white,len);
      }
      else
	 while (len--)
	 {
#define ALPHA_ADD(S,L,D,SA,LA,DA,C)					\
	    if (!LA->C) d->C=S->C,DA->C=SA->C;				\
	    else if (!SA->C) D->C=l->C,DA->C=LA->C;			\
	    else if (LA->C==COLORMAX) D->C=l->C,DA->C=LA->C;		\
	    else							\
	       D->C=COMBINE_ALPHA(S->C,l->C,SA->C,LA->C),		\
		  DA->C=COMBINE_ALPHA_SUM(SA->C,LA->C);

	    if (la->r==COLORMAX && la->g==COLORMAX && la->b==COLORMAX)
	    {
	       *d=*l;
	       *da=*la;
	    }
	    else if (la->r==0 && la->g==0 && la->b==0)
	    {
	       *d=*s;
	       *da=*sa;
	    }
	    else
	    {
	       ALPHA_ADD(s,l,d,sa,la,da,r);
	       ALPHA_ADD(s,l,d,sa,la,da,g);
	       ALPHA_ADD(s,l,d,sa,la,da,b);
	    }
#undef ALPHA_ADD
	    l++; s++; la++; sa++; d++; da++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => alpha value opaque */
	 while (len--)
	 {
#define ALPHA_ADD_V_NOLA(L,S,D,SA,DA,V,C)				\
            do {							\
               if (!SA->C) D->C=l->C,DA->C=0;				\
               else							\
               {							\
                 if (SA->C==COLORMAX)					\
		 	D->C=COMBINE_ALPHA_V(S->C,l->C,COLORMAX,255,V);	\
  	         else D->C=COMBINE_ALPHA_V(S->C,l->C,SA->C,255,V);	\
     	         DA->C=COMBINE_ALPHA_SUM_V(SA->C,255,V);		\
               }							\
	    } while(0)

	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,r);
	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,g);
	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,b);

#undef ALPHA_ADD_V_NOLA
	    l++; s++; la++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
#define ALPHA_ADD_V(L,S,D,LA,SA,DA,V,C) 				\
            do { 							\
	       if (!LA->C) 						\
	       {							\
		  D->C=COMBINE_ALPHA_V(S->C,l->C,SA->C,0,V); 		\
		  DA->C=COMBINE_ALPHA_SUM_V(0,SA->C,V);  		\
	       }							\
	       else if (!SA->C) 					\
	       {							\
		  D->C=COMBINE_ALPHA_V(S->C,l->C,0,LA->C,V); 		\
		  DA->C=COMBINE_ALPHA_SUM_V(LA->C,0,V);  		\
	       }							\
	       else 							\
	       {							\
		  D->C=COMBINE_ALPHA_V(S->C,l->C,SA->C,LA->C,V); 	\
		  DA->C=COMBINE_ALPHA_SUM_V(LA->C,SA->C,V);  		\
	       }							\
	    } while (0) 

	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,r);
	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,g);
	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,b);

#undef ALPHA_ADD_V
	    l++; s++; la++; sa++; da++; d++;
	 }
      return;
   }
}


static void lm_add(rgb_group *s,rgb_group *l,rgb_group *d,
		   rgb_group *sa,rgb_group *la,rgb_group *da,
		   int len,float alpha)
{
   if (alpha==0.0)
   {
      MEMCPY(s,d,sizeof(rgb_group)*len);
      MEMCPY(sa,da,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=MINIMUM(COLORMAX,COMBINE(s->r,sa->r)+(int)l->r);
	    d->g=MINIMUM(COLORMAX,COMBINE(s->r,sa->g)+(int)l->g);
	    d->b=MINIMUM(COLORMAX,COMBINE(s->r,sa->b)+(int)l->b);
	    *da=white;
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=MINIMUM(COLORMAX,COMBINE(s->r,sa->r)+
			 (int)COMBINE(l->r,la->r));
	    d->g=MINIMUM(COLORMAX,COMBINE(s->g,sa->g)+
			 (int)COMBINE(l->g,la->g));
	    d->b=MINIMUM(COLORMAX,COMBINE(s->b,sa->b)+
			 (int)COMBINE(l->b,la->b));
	    da->r=COMBINE_ALPHA_SUM(la->r,sa->r);
	    da->g=COMBINE_ALPHA_SUM(la->g,sa->g);
	    da->b=COMBINE_ALPHA_SUM(la->b,sa->b);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=MINIMUM(COLORMAX,COMBINE(s->r,sa->r)+
			 (int)COMBINE_A(l->r,alpha));
	    d->g=MINIMUM(COLORMAX,COMBINE(s->g,sa->g)+
			 (int)COMBINE_A(l->g,alpha));
	    d->b=MINIMUM(COLORMAX,COMBINE(s->b,sa->b)+
			 (int)COMBINE_A(l->b,alpha));
	    da->r=COMBINE_ALPHA_SUM_V(COLORMAX,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(COLORMAX,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(COLORMAX,sa->b,alpha);
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=MINIMUM(COLORMAX,
			 COMBINE(s->r,sa->r)+(int)COMBINE_V(l->r,la->r,alpha));
	    d->g=MINIMUM(COLORMAX,
			 COMBINE(s->g,sa->g)+(int)COMBINE_V(l->g,la->g,alpha));
	    d->b=MINIMUM(COLORMAX,
			 COMBINE(s->b,sa->b)+(int)COMBINE_V(l->b,la->b,alpha));
	    da->r=COMBINE_ALPHA_SUM_V(la->r,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(la->g,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(la->b,sa->b,alpha);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
}

static void lm_subtract(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,float alpha)
{
   if (alpha==0.0)
   {
      MEMCPY(s,d,sizeof(rgb_group)*len);
      MEMCPY(sa,da,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=MAXIMUM(0,COMBINE(s->r,sa->r)-(int)l->r);
	    d->g=MAXIMUM(0,COMBINE(s->r,sa->g)-(int)l->g);
	    d->b=MAXIMUM(0,COMBINE(s->r,sa->b)-(int)l->b);
	    *da=white;
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=MAXIMUM(0,COMBINE(s->r,sa->r)-
			 (int)COMBINE(l->r,la->r));
	    d->g=MAXIMUM(0,COMBINE(s->g,sa->g)-
			 (int)COMBINE(l->g,la->g));
	    d->b=MAXIMUM(0,COMBINE(s->b,sa->b)-
			 (int)COMBINE(l->b,la->b));
	    da->r=COMBINE_ALPHA_SUM(la->r,sa->r);
	    da->g=COMBINE_ALPHA_SUM(la->g,sa->g);
	    da->b=COMBINE_ALPHA_SUM(la->b,sa->b);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=MAXIMUM(0,COMBINE(s->r,sa->r)-
			 (int)COMBINE_A(l->r,alpha));
	    d->g=MAXIMUM(0,COMBINE(s->g,sa->g)-
			 (int)COMBINE_A(l->g,alpha));
	    d->b=MAXIMUM(0,COMBINE(s->b,sa->b)-
			 (int)COMBINE_A(l->b,alpha));
	    da->r=COMBINE_ALPHA_SUM_V(COLORMAX,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(COLORMAX,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(COLORMAX,sa->b,alpha);
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=MAXIMUM(0,COMBINE(s->r,sa->r)-
			 (int)COMBINE_V(l->r,la->r,alpha));
	    d->g=MAXIMUM(0,COMBINE(s->g,sa->g)-
			 (int)COMBINE_V(l->g,la->g,alpha));
	    d->b=MAXIMUM(0,COMBINE(s->b,sa->b)-
			 (int)COMBINE_V(l->b,la->b,alpha));
	    da->r=COMBINE_ALPHA_SUM_V(la->r,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(la->g,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(la->b,sa->b,alpha);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
}

static void lm_dissolve(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,float alpha)
{
   if (alpha==0.0)
   {
      MEMCPY(s,d,sizeof(rgb_group)*len);
      MEMCPY(sa,da,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--) /* 50 % */
	 {
	    if (my_rand()&1) *d=*l,*da=white;
	    else *d=*s,*da=*sa;
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    if ((my_rand()&65535) < (la->r*87 + la->g*127 + la->b*41)) 
	       *d=*l,*da=white;
	    else 
	       *d=*s,*da=*sa;
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      int v=(int)(COLORMAX*alpha);
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    if ((my_rand()&255) < v) 
	       *d=*l,*da=white;
	    else 
	       *d=*s,*da=*sa;
	    l++; s++; sa++; da++; d++;
	 }
      else
      {
	 while (len--)
	 {
	    if ((my_rand()&65535) < ((la->r*87 + la->g*127 + la->b*41)>>8)*v)
	       *d=*l,*da=white;
	    else 
	       *d=*s,*da=*sa;
	    l++; s++; la++; sa++; da++; d++;
	 }
      }
   }
}

/*** the add-layer function ***************************/

static void INLINE img_lay_first_line(struct layer *l,
				      int xoffs,int xsize,
				      int y, /* in _this_ layer */
				      rgb_group *d,rgb_group *da)
{
   if (!l->tiled)
   {
      rgb_group *s,*sa;
      int len;

      if (y<0 ||
	  y>=l->ysize ||
	  l->xoffs+l->xsize<xoffs ||
	  l->xoffs>xoffs+xsize) /* outside */
      {
	 smear_color(d,l->fill,xsize);
	 smear_color(da,l->fill_alpha,xsize);
	 return;
      }

      if (l->img) s=l->img->img+y*l->xsize; else s=NULL;
      if (l->alp) sa=l->alp->img+y*l->xsize; else sa=NULL;
      len=l->xsize;
      
      if (l->xoffs>xoffs) 
      {
	 /* fill to the left */
	 smear_color(d,l->fill,l->xoffs-xoffs);
	 smear_color(da,l->fill_alpha,l->xoffs-xoffs);

	 xsize-=l->xoffs-xoffs;
	 d+=l->xoffs-xoffs;
	 da+=l->xoffs-xoffs;
      }
      else
      {
	 if (s) s+=xoffs-l->xoffs;
	 if (sa) sa+=xoffs-l->xoffs;
	 len-=xoffs-l->xoffs;
      }
      if (len<xsize) /* copy bit, fill right */
      {
	 if (s) MEMCPY(d,s,len*sizeof(rgb_group)); 
	 else smear_color(d,l->fill,len);
	 if (sa) MEMCPY(da,sa,len*sizeof(rgb_group)); 
	 else smear_color(da,white,len);

	 smear_color(d+len,l->fill,xsize-len);
	 smear_color(da+len,l->fill_alpha,xsize-len);
      }
      else /* copy rest */
      {
	 if (s) MEMCPY(d,s,xsize*sizeof(rgb_group)); 
	 else smear_color(d,l->fill,xsize);
	 if (sa) MEMCPY(da,sa,xsize*sizeof(rgb_group)); 
	 else smear_color(da,white,xsize);
      }
      return;
   }
   else
   {
      rgb_group *s,*sa;

      y%=l->ysize;
      if (y<0) y+=l->ysize;

      if (l->img) s=l->img->img+y*l->xsize; 
      else smear_color(d,l->fill,xsize),s=NULL;

      if (l->alp) sa=l->alp->img+y*l->xsize; 
      else smear_color(da,white,xsize),sa=NULL;

      xoffs-=l->xoffs; /* position in source */
      xoffs%=l->xsize;
      if (xoffs<0) xoffs+=l->xsize;
      if (xoffs)
      {
	 int len=l->xsize-xoffs;
	 if (len>l->xsize) len=l->xsize;
	 if (s) MEMCPY(d,s+xoffs,len*sizeof(rgb_group));
	 if (sa) MEMCPY(da,sa+xoffs,len*sizeof(rgb_group));
	 da+=len;
	 d+=len;
	 xsize-=len;
      }
      while (xsize>l->xsize)
      {
	 if (s) MEMCPY(d,s,l->xsize*sizeof(rgb_group));
	 if (sa) MEMCPY(d,sa,l->xsize*sizeof(rgb_group));
	 da+=l->xsize;
	 d+=l->xsize;
	 xsize-=l->xsize;
      }
      if (s) MEMCPY(d,s,xsize*sizeof(rgb_group));
      if (sa) MEMCPY(d,sa,xsize*sizeof(rgb_group));
   }
}

static INLINE void img_lay_stroke(struct layer *ly,
				  rgb_group *l,
				  rgb_group *la,
				  rgb_group *s,
				  rgb_group *sa,
				  rgb_group *d,
				  rgb_group *da,
				  int len)
{
   if (l) 
   {
      (ly->row_func)(s,l,d,sa,la,da,len,ly->alpha_value);
      return;
   }

   if (!la &&
       ly->fill_alpha.r==0 && 
       ly->fill_alpha.g==0 && 
       ly->fill_alpha.b==0 && 
       ly->optimize_alpha)
   {
      MEMCPY(d,s,len*sizeof(rgb_group));
      MEMCPY(da,sa,len*sizeof(rgb_group));
      return;
   }

   if (!la && 
       ly->fill_alpha.r==COLORMAX && 
       ly->fill_alpha.g==COLORMAX && 
       ly->fill_alpha.b==COLORMAX)
   {
      while (len>SNUMPIXS)
      {
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,NULL,da,
			SNUMPIXS,ly->alpha_value);
	 s+=SNUMPIXS; d+=SNUMPIXS;
	 sa+=SNUMPIXS;da+=SNUMPIXS;
	 if (l) l+=SNUMPIXS; 
	 len-=SNUMPIXS;
      }
      if (len)
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,NULL,da,len,ly->alpha_value);
   }
   else
   {
      int i;

      while (len>SNUMPIXS)
      {
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,la?la:ly->sfill_alpha,da,
			SNUMPIXS,ly->alpha_value);
	 s+=SNUMPIXS; d+=SNUMPIXS;
	 sa+=SNUMPIXS; da+=SNUMPIXS;
	 if (l) l+=SNUMPIXS; 
	 if (la) la+=SNUMPIXS; 
	 len-=SNUMPIXS;
      }
      if (len)
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,la?la:ly->sfill_alpha,
			da,len,ly->alpha_value);
   }
}

static INLINE void img_lay_line(struct layer *ly,
				rgb_group *s,rgb_group *sa,
				int xoffs,int xsize,
				int y, /* y in ly layer */
				rgb_group *d,rgb_group *da)
{
   if (!ly->tiled)
   {
      int len;
      rgb_group *l,*la;

      if (y<0 ||
	  y>=ly->ysize ||
	  ly->xoffs+ly->xsize<xoffs ||
	  ly->xoffs>xoffs+xsize) /* outside */
      {
	 img_lay_stroke(ly,NULL,NULL,s,sa,d,da,xsize);
	 return;
      }

      if (ly->img) l=ly->img->img+y*ly->xsize; else l=NULL;
      if (ly->alp) la=ly->alp->img+y*ly->xsize; else la=NULL;
      len=ly->xsize;
      
      if (ly->xoffs>xoffs) 
      {
	 /* fill to the left */
	 img_lay_stroke(ly,NULL,NULL,s,sa,d,da,ly->xoffs-xoffs);

	 xsize-=ly->xoffs-xoffs;
	 d+=ly->xoffs-xoffs;
	 da+=ly->xoffs-xoffs;
	 s+=ly->xoffs-xoffs;
	 sa+=ly->xoffs-xoffs;
      }
      else
      {
	 if (l) l+=xoffs-ly->xoffs;
	 if (la) la+=xoffs-ly->xoffs;
	 len-=xoffs-ly->xoffs;
      }
      if (len<xsize) /* copy stroke, fill right */
      {
	 img_lay_stroke(ly,l,la,s,sa,d,da,len);

	 img_lay_stroke(ly,NULL,NULL,s+len,sa+len,d+len,da+len,xsize-len);
      }
      else /* copy rest */
      {
	 img_lay_stroke(ly,l,la,s,sa,d,da,xsize);
      }
      return;
   }
   else
   {
      rgb_group *l,*la;

      y%=ly->ysize;
      if (y<0) y+=ly->ysize;

      if (ly->img) l=ly->img->img+y*ly->xsize; else l=NULL;
      if (ly->alp) la=ly->alp->img+y*ly->xsize; else la=NULL;

      xoffs-=ly->xoffs; /* position in source */
      if ((xoffs=xoffs%ly->xsize))
      {
	 int len;
	 if (xoffs<0) xoffs+=ly->xsize;
	 len=ly->xsize-xoffs;

	 img_lay_stroke(ly,l?l+xoffs:NULL,
			la?la+(xoffs%ly->xsize):NULL,
			s,sa,d,da,len);
	 da+=len;
	 d+=len;
	 sa+=len;
	 s+=len;
	 xsize-=len;
      }
      while (xsize>ly->xsize)
      {
	 img_lay_stroke(ly,l,la,s,sa,d,da,ly->xsize);
	 da+=ly->xsize;
	 d+=ly->xsize;
	 sa+=ly->xsize;
	 s+=ly->xsize;
	 xsize-=ly->xsize;
      }
      if (xsize)
	 img_lay_stroke(ly,l,la,s,sa,d,da,xsize);
   }
}


void img_lay(struct layer **layer,
	     int layers,
	     struct layer *dest)
{
   rgb_group *line1,*line2,*aline1,*aline2,*tmp;
   rgb_group *d,*da;
   int width=dest->xsize;
   int y,z;
   int xoffs=dest->xoffs,xsize=dest->xsize;

   line1=malloc(sizeof(rgb_group)*width);
   line2=malloc(sizeof(rgb_group)*width);
   aline1=malloc(sizeof(rgb_group)*width);
   aline2=malloc(sizeof(rgb_group)*width);
   if (!line1 || !line2 || !aline1 || !aline2)
   {
      if (line1) free(line1);
      if (line2) free(line2);
      if (aline1) free(aline1);
      if (aline2) free(aline2);
      resource_error(NULL,0,0,"memory",sizeof(rgb_group)*4*width,
		     "Out of memory.\n");
   }

   da=dest->alp->img;
   d=dest->img->img;

   /* loop over lines */
   for (y=0; y<dest->ysize; y++)
   {
      if (layers>1 || layer[0]->row_func!=lm_normal)
      {
	 /* add the bottom layer first */
	 if (layer[0]->row_func==lm_normal) /* cheat */
	 {
	    img_lay_first_line(layer[0],xoffs,xsize,
			       y+dest->yoffs-layer[0]->yoffs,
			       line1,aline1),z=1;
	    z=1;
	 }
	 else
	 {
	    smear_color(line1,black,xsize);
	    smear_color(aline1,black,xsize);
	    z=0;
	 }

	 /* loop over the rest of the layers, except the last */
	 for (; z<layers-1; z++)
	 {
	    img_lay_line(layer[z],line1,aline1,
			 xoffs,xsize,y-layer[z]->yoffs,line2,aline2);
	    /* swap buffers */
	    tmp=line1; line1=line2; line2=tmp;
	    tmp=aline1; aline1=aline2; aline2=tmp;
	 }

	 /* make the last layer on the destionation */
	 img_lay_line(layer[layers-1],line1,aline1,
		      xoffs,xsize,y+dest->yoffs-layer[layers-1]->yoffs,d,da);
      }
      else 
      {
	 /* make the layer to destination*/
	 img_lay_first_line(layer[0],xoffs,xsize,
			    y+dest->yoffs-layer[0]->yoffs,d,da);
      }
      d+=dest->xsize;
      da+=dest->xsize;
   }

   free(line1);
   free(aline1);
   free(line2);
   free(aline2);
}

/*
**! module Image
**! method Image.Layer lay(array(Image.Layer|mapping))
**! method Image.Layer lay(array(Image.Layer|mapping),int xoffset,int yoffset,int xsize,int ysize)
**!	Combine layers. 
**! returns a new layer object.
**!
**! see also: Image.Layer
*/

void image_lay(INT32 args)
{
   int layers,i;
   struct layer **l;
   struct object *o;
   struct layer *dest;
   struct array *a;
   int gotoffs;
   int xoffset=0,yoffset=0,xsize=0,ysize=0;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.lay",1);
      
   if (sp[-args].type!=T_ARRAY)
      SIMPLE_BAD_ARG_ERROR("Image.lay",1,
			   "array(Image.Layer|mapping)");

   if (args>1)
   {
      get_all_args("Image.lay",args-1,"%i%i%i%i",
		   &xoffset,&yoffset,&xsize,&ysize);
      if (xsize<1)
	 SIMPLE_BAD_ARG_ERROR("Image.lay",4,"int(1..)");
      if (ysize<1)
	 SIMPLE_BAD_ARG_ERROR("Image.lay",5,"int(1..)");
   }
   
   layers=(a=sp[-args].u.array)->size;

   if (!layers) /* dummy return empty layer */
   {
      pop_n_elems(args);
      push_object(clone_object(image_layer_program,0));
      return;
   }
      
   l=(struct layer**)xalloc(sizeof(struct layer)*layers);

   for (i=0; i<layers; i++)
   {
      if (a->item[i].type==T_OBJECT)
      {
	 if (!(l[i]=(struct layer*)get_storage(a->item[i].u.object,
					       image_layer_program)))
	    SIMPLE_BAD_ARG_ERROR("Image.lay",1,
				 "array(Image.Layer|mapping)");
      }
      else if (a->item[i].type==T_MAPPING)
      {
	 push_svalue(a->item+i);
	 push_object(o=clone_object(image_layer_program,1));
	 args++;
	 l[i]=(struct layer*)get_storage(o,image_layer_program);
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.lay",1,
			      "array(Image.Layer|mapping)");
   }

   if (xsize==0) /* figure offset and size */
   {
      xoffset=l[0]->xoffs;
      yoffset=l[0]->yoffs;
      xsize=l[0]->xsize;
      ysize=l[0]->ysize;
      for (i=1; i<layers; i++)
      {
	 int t;
	 if (l[i]->xoffs<xoffset) 
	    t=xoffset-l[i]->xoffs,xoffset-=t,xsize+=t;
	 if (l[i]->yoffs<yoffset) 
	    t=yoffset-l[i]->yoffs,yoffset-=t,ysize+=t;
	 if (l[i]->xsize+l[i]->xoffs-xoffset>xsize)
	    xsize=l[i]->xsize+l[i]->xoffs-xoffset;
	 if (l[i]->ysize+l[i]->yoffs-yoffset>ysize)
	    ysize=l[i]->ysize+l[i]->yoffs-yoffset;
      }
   }

   /* get destination layer */
   push_int(xsize);
   push_int(ysize);
   push_object(o=clone_object(image_layer_program,2));

   dest=(struct layer*)get_storage(o,image_layer_program);
   dest->xoffs=xoffset;
   dest->yoffs=yoffset;

   /* ok, do it! */
   img_lay(l,layers,dest);
   
   sp--;
   pop_n_elems(args);
   push_object(o);
}

/******************************************************/

void init_image_layers(void)
{
   char buf[100];
   char buf2[sizeof(INT32)];
   int i;
   
   for (i=0; i<LAYER_MODES; i++)
      layer_mode[i].ps=make_shared_string(layer_mode[i].name);

   start_new_program();

   ADD_STORAGE(struct layer);
   set_init_callback(init_layer);
   set_exit_callback(exit_layer);

#define tLayerMap tMap(tString,tOr4(tString,tColor,tFloat,tInt))

   ADD_FUNCTION("create",image_layer_create,
		tOr4(tFunc(,tVoid),
		     tFunc(tObj tOr(tObj,tVoid) tOr(tString,tVoid),tVoid),
		     tFunc(tLayerMap,tVoid),
		     tFunc(tInt tInt 
			   tOr(tColor,tVoid) tOr(tColor,tVoid),tVoid)),0);

   ADD_FUNCTION("cast",image_layer_cast,
		tFunc(tString,tMapping),0);

   /* set */

   ADD_FUNCTION("set_offset",image_layer_set_offset,tFunc(tInt tInt,tObj),0);
   ADD_FUNCTION("set_image",image_layer_set_image,
		tFunc(tOr(tObj,tVoid) tOr(tObj,tVoid),tObj),0);
   ADD_FUNCTION("set_fill",image_layer_set_fill,
		tFunc(tOr(tObj,tVoid) tOr(tObj,tVoid),tObj),0);
   ADD_FUNCTION("set_mode",image_layer_set_mode,tFunc(tStr,tObj),0);
   ADD_FUNCTION("set_alpha_value",image_layer_set_mode,tFunc(tFloat,tObj),0);
   ADD_FUNCTION("set_tiled",image_layer_set_tiled,tFunc(tInt,tObj),0);

   /* query */

   ADD_FUNCTION("image",image_layer_image,tFunc(,tObj),0);
   ADD_FUNCTION("alpha",image_layer_alpha,tFunc(,tObj),0);
   ADD_FUNCTION("mode",image_layer_mode,tFunc(,tStr),0);

   ADD_FUNCTION("available_modes",image_layer_available_modes,
		tFunc(,tArr(tStr)),0);

   ADD_FUNCTION("xoffset",image_layer_xoffset,tFunc(,tInt),0);
   ADD_FUNCTION("yoffset",image_layer_yoffset,tFunc(,tInt),0);

   ADD_FUNCTION("alpha_value",image_layer_alpha_value,tFunc(,tFloat),0);
   ADD_FUNCTION("fill",image_layer_fill,tFunc(,tObj),0);
   ADD_FUNCTION("fill_alpha",image_layer_fill_alpha,tFunc(,tObj),0);

   ADD_FUNCTION("tiled",image_layer_tiled,tFunc(,tInt01),0);

   image_layer_program=end_program();

   add_program_constant("Layer",image_layer_program,0);

   ADD_FUNCTION("lay",image_lay,
		tOr(tFunc(tArr(tOr(tObj,tLayerMap)),tObj),
		    tFunc(tArr(tOr(tObj,tLayerMap))
			  tInt tInt tInt tInt,tObj)),0);
}

void exit_image_layers(void)
{
   int i;
   
   for (i=0; i<LAYER_MODES; i++)
      free_string(layer_mode[i].ps);

   if (image_layer_program)
   {
      free_program(image_layer_program);
      image_layer_program=NULL;
   }
}
