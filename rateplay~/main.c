#include "m_pd.h"
#include <math.h>

static t_class *rateplay_tilde_class;

typedef struct _rateplay_tilde
{
    t_object x_obj;
    t_outlet *x_bangout;
    float x_phase;
    int x_nsampsintab;
    int x_limit;
    float x_sampincr;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_clock *x_clock;
} t_rateplay_tilde;

void rateplay_tilde_tick(t_rateplay_tilde *x);

void *rateplay_tilde_new(t_symbol *s, t_float f)
{
    t_rateplay_tilde *x = (t_rateplay_tilde *)pd_new(rateplay_tilde_class);
    x->x_clock = clock_new(x, (t_method)rateplay_tilde_tick);
    x->x_phase = 0x4f000000;
    x->x_limit = 0;
    x->x_arrayname = s;
    x->x_sampincr = f;
    outlet_new(&x->x_obj, &s_signal);
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    return (x);
}

 t_int *rateplay_tilde_perform(t_int *w)
{
    t_rateplay_tilde *x = (t_rateplay_tilde *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_word *wp = x->x_vec;
    float phase = x->x_phase;
    float sampincr = x->x_sampincr;
    int n = (int)(w[3]),
        endphase = (x->x_nsampsintab < x->x_limit ?
            x->x_nsampsintab : x->x_limit);
    if (!x->x_vec || phase >= endphase)
        goto zero;


    for (int i = 0; i < n; i++)
    {
        if (phase < endphase)
        {
            *out++ = (wp + (int)floor(phase))->w_float;
            phase += sampincr;
        }
        else
        {
            *out++ = 0;
        }
    }
    x->x_phase = phase;

    return (w+4);
zero:
    while (n--) *out++ = 0;
    return (w+4);
}

void rateplay_tilde_set(t_rateplay_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name) pd_error(x, "rateplay~: %s: no such array",
            x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_nsampsintab, &x->x_vec))
    {
        pd_error(x, "%s: bad template for rateplay~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

void rateplay_tilde_dsp(t_rateplay_tilde *x, t_signal **sp)
{
    rateplay_tilde_set(x, x->x_arrayname);
    dsp_add(rateplay_tilde_perform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
}

void rateplay_tilde_list(t_rateplay_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    long start = atom_getfloatarg(0, argc, argv);
    float length = atom_getfloatarg(1, argc, argv);
    if (start < 0) start = 0;
    if (length <= 0)
        x->x_limit = 0x4f000000;
    else
        x->x_limit = (int)(start + length);
    x->x_phase = (float)start;
}

void rateplay_tilde_stop(t_rateplay_tilde *x)
{
    x->x_phase = 0x4f000000;
}

void rateplay_tilde_tick(t_rateplay_tilde *x)
{
    outlet_bang(x->x_bangout);
}

 void rateplay_tilde_free(t_rateplay_tilde *x)
{
    clock_free(x->x_clock);
}

void rateplay_ft1(t_rateplay_tilde *x, t_float f)
{
    if (f > 0)
    {
        x->x_sampincr = f;
    }

}

void rateplay_tilde_setup(void)
{
    rateplay_tilde_class = class_new(gensym("rateplay~"),
        (t_newmethod)rateplay_tilde_new, (t_method)rateplay_tilde_free,
        sizeof(t_rateplay_tilde), 0, A_DEFSYM, A_DEFFLOAT, 0);
    class_addmethod(rateplay_tilde_class, (t_method)rateplay_tilde_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(rateplay_tilde_class, (t_method)rateplay_tilde_stop,
        gensym("stop"), 0);
    class_addmethod(rateplay_tilde_class, (t_method)rateplay_tilde_set,
        gensym("set"), A_DEFSYM, 0);
    class_addlist(rateplay_tilde_class, rateplay_tilde_list);
    class_addmethod(rateplay_tilde_class, (t_method)rateplay_ft1, gensym("ft1"), A_FLOAT, 0);
}
