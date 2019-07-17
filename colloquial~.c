/**
 @file
 colloquial~ - interpolated sample rate reduction of some kind

 @ingroup	examples
 */

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

void *colloquial_class;

typedef struct _colloquial {
	t_pxobject	x_obj;
	float		x_rate;
    double lastframes[1024];
} t_colloquial;


void *colloquial_new(double val);
void colloquial_float(t_colloquial *x, double f);
void colloquial_int(t_colloquial *x, long n);
void colloquial_dsp64(t_colloquial *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void colloquial_assist(t_colloquial *x, void *b, long m, long a, char *s);


void ext_main(void *r)
{
	t_class *c = class_new("colloquial~", (method)colloquial_new, (method)dsp_free, sizeof(t_colloquial), NULL, A_DEFFLOAT, 0);

	class_addmethod(c, (method)colloquial_dsp64, "dsp64", A_CANT, 0); 	// respond to the dsp message
	class_addmethod(c, (method)colloquial_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)colloquial_int, "int", A_LONG, 0);
	class_addmethod(c, (method)colloquial_assist,"assist",A_CANT,0);
	class_dspinit(c);												// must call this function for MSP object classes

	class_register(CLASS_BOX, c);
	colloquial_class = c;
}


void *colloquial_new(double rate)
{
	t_colloquial *x = object_alloc(colloquial_class);
	dsp_setup((t_pxobject *)x,1);					// set up DSP for the instance and create signal inlets
	outlet_new((t_pxobject *)x, "signal"); // signal outlets are created like this
    
    
	x->x_rate = rate + 0.5;
    if(x->x_rate < 1) x->x_rate = 1;
    
    for(int i = 0; i < 1024; i++) {
        x->lastframes[i] = 0.0f;
    }
    
	return (x);
}


void colloquial_assist(t_colloquial *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) {
		switch (a) {
		case 0:
			sprintf(s,"(Signal) Input");
			break;
		}
	}
	else
		sprintf(s,"(Signal) Ouput");
}


// the float and int routines cover both inlets.  It doesn't matter which one is involved
void colloquial_float(t_colloquial *x, double f)
{
	x->x_rate = f + 0.5;
    if(x->x_rate < 1) x->x_rate = 1;
}


void colloquial_int(t_colloquial *x, long n)
{
	x->x_rate = n;
    if(x->x_rate < 1) x->x_rate = 1;
}

double hermite(double x, double y0, double y1, double y2, double y3) {
    // 4-point, 3rd-order Hermite (x-form)

    return (((0.5 * (y3 - y0) + 1.5 * (y1 - y2)) * x + (y0 - 2.5 * y1 + 2. * y2 - 0.5 * y3)) * x + 0.5 * (y2 - y0)) * x + y1;
}

double get(int index, t_double* in, int sampleframes, t_colloquial *x) {
    if(index < 0) {
        return x->lastframes[sampleframes + index];
    }
    else return in[index];
}

void colloquial_perform64(t_colloquial *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_double	*in = ins[0];
	t_double	*out = outs[0];
    int		rate = x->x_rate;
    
    for(int i = 0; i < sampleframes; i++) {
        int index = i - (i % rate);
        
        //out[i] = get(index, in, sampleframes, x);
        
        int phase3 = index;
        int phase2 = phase3 - (1 * rate);
        int phase1 = phase3 - (2 * rate);
        int phase0 = phase3 - (3 * rate);
        
        float y0 = get(phase0, in, sampleframes, x);
        float y1 = get(phase1, in, sampleframes, x);
        float y3 = get(phase3, in, sampleframes, x);
        float y2 = get(phase2, in, sampleframes, x);
        
        int xx = i - index;
        
        out[i] = hermite(xx, y0, y1, y2, y3);
    }
    
    for(int j=0; j<sampleframes; j++) {
        x->lastframes[j] = in[j];
    }
}


// method called when dsp is turned on
void colloquial_dsp64(t_colloquial *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
		dsp_add64(dsp64, (t_object *)x, (t_perfroutine64)colloquial_perform64, 0, NULL);
}
